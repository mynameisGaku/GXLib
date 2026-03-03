/// @file AudioEffect.cpp
/// @brief オーディオDSPエフェクトの実装
#include "pch_audio.h"
#include "Audio/AudioEffect.h"
#include "Core/Logger.h"
#include <xaudio2fx.h>
#include <xapofx.h>

#pragma comment(lib, "xaudio2.lib")

namespace gx
{

// ============================================================================
// ReverbParamsプリセット
// ============================================================================
ReverbParams ReverbParams::FromPreset(ReverbPreset preset)
{
    ReverbParams p;
    switch (preset)
    {
    case ReverbPreset::Default:    p = { 50.0f, 80.0f, 1.5f, 100.0f, 100.0f }; break;
    case ReverbPreset::Room:       p = { 40.0f, 40.0f, 0.8f, 80.0f, 60.0f }; break;
    case ReverbPreset::Hall:       p = { 60.0f, 90.0f, 3.0f, 100.0f, 100.0f }; break;
    case ReverbPreset::Cave:       p = { 70.0f, 100.0f, 5.0f, 50.0f, 40.0f }; break;
    case ReverbPreset::Arena:      p = { 55.0f, 100.0f, 4.0f, 100.0f, 80.0f }; break;
    case ReverbPreset::Bathroom:   p = { 80.0f, 30.0f, 1.2f, 100.0f, 100.0f }; break;
    case ReverbPreset::Forest:     p = { 30.0f, 70.0f, 2.0f, 30.0f, 50.0f }; break;
    case ReverbPreset::Underwater: p = { 90.0f, 100.0f, 8.0f, 100.0f, 20.0f }; break;
    }
    return p;
}

// ============================================================================
// AudioEffectChain
// ============================================================================

AudioEffectChain::~AudioEffectChain()
{
    Shutdown();
}

bool AudioEffectChain::Initialize(IXAudio2SubmixVoice* submixVoice, uint32_t channels, uint32_t sampleRate)
{
    if (!submixVoice) return false;
    m_submixVoice = submixVoice;
    m_channels    = channels;
    m_sampleRate  = sampleRate;
    m_initialized = true;
    return true;
}

void AudioEffectChain::SetReverbParams(const ReverbParams& params)
{
    m_reverbParams = params;
}

void AudioEffectChain::SetReverbPreset(ReverbPreset preset)
{
    m_reverbParams = ReverbParams::FromPreset(preset);
}

void AudioEffectChain::SetEQParams(const EQParams& params)
{
    m_eqParams = params;
}

void AudioEffectChain::Apply()
{
    if (!m_initialized || !m_submixVoice) return;

    // アクティブなエフェクト数をカウント
    gx::Vector<XAUDIO2_EFFECT_DESCRIPTOR> descriptors;
    IUnknown* reverbEffect = nullptr;
    IUnknown* eqEffect = nullptr;

    if (m_reverbEnabled)
    {
        HRESULT hr = CreateFX(__uuidof(FXReverb), &reverbEffect);
        if (SUCCEEDED(hr) && reverbEffect)
        {
            XAUDIO2_EFFECT_DESCRIPTOR desc = {};
            desc.InitialState = TRUE;
            desc.OutputChannels = m_channels;
            desc.pEffect = reverbEffect;
            descriptors.push_back(desc);
        }
    }

    if (m_eqEnabled)
    {
        HRESULT hr = CreateFX(__uuidof(FXEQ), &eqEffect);
        if (SUCCEEDED(hr) && eqEffect)
        {
            XAUDIO2_EFFECT_DESCRIPTOR desc = {};
            desc.InitialState = TRUE;
            desc.OutputChannels = m_channels;
            desc.pEffect = eqEffect;
            descriptors.push_back(desc);
        }
    }

    if (descriptors.empty())
    {
        // 全エフェクトを削除
        m_submixVoice->SetEffectChain(nullptr);
        if (reverbEffect) reverbEffect->Release();
        if (eqEffect) eqEffect->Release();
        return;
    }

    XAUDIO2_EFFECT_CHAIN chain = {};
    chain.EffectCount = static_cast<UINT32>(descriptors.size());
    chain.pEffectDescriptors = descriptors.data();

    HRESULT hr = m_submixVoice->SetEffectChain(&chain);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("AudioEffectChain::Apply - SetEffectChain failed: 0x%08X", hr);
    }
    else
    {
        // リバーブパラメータを設定
        int effectIndex = 0;
        if (m_reverbEnabled && reverbEffect)
        {
            XAUDIO2FX_REVERB_PARAMETERS reverbNative = {};
            reverbNative.WetDryMix      = m_reverbParams.wetDryMix;
            reverbNative.RoomSize       = m_reverbParams.roomSize;
            reverbNative.Density        = m_reverbParams.density;
            reverbNative.DecayTime      = m_reverbParams.decay;
            reverbNative.EarlyDiffusion = static_cast<BYTE>((std::min)(m_reverbParams.diffusion / 100.0f * 15.0f, 15.0f));
            reverbNative.LateDiffusion  = reverbNative.EarlyDiffusion;
            m_submixVoice->SetEffectParameters(effectIndex, &reverbNative, sizeof(reverbNative));
            effectIndex++;
        }

        if (m_eqEnabled && eqEffect)
        {
            FXEQ_PARAMETERS eqNative = {};
            eqNative.FrequencyCenter0 = 100.0f;
            eqNative.Gain0 = m_eqParams.lowGain;
            eqNative.Bandwidth0 = 1.5f;
            eqNative.FrequencyCenter1 = 800.0f;
            eqNative.Gain1 = m_eqParams.midLowGain;
            eqNative.Bandwidth1 = 1.5f;
            eqNative.FrequencyCenter2 = 2500.0f;
            eqNative.Gain2 = m_eqParams.midHighGain;
            eqNative.Bandwidth2 = 1.5f;
            eqNative.FrequencyCenter3 = 8000.0f;
            eqNative.Gain3 = m_eqParams.highGain;
            eqNative.Bandwidth3 = 1.5f;
            m_submixVoice->SetEffectParameters(effectIndex, &eqNative, sizeof(eqNative));
        }
    }

    // COMオブジェクトを解放（SubmixVoiceが参照を保持）
    if (reverbEffect) reverbEffect->Release();
    if (eqEffect) eqEffect->Release();
}

void AudioEffectChain::Shutdown()
{
    if (m_submixVoice && m_initialized)
    {
        m_submixVoice->SetEffectChain(nullptr);
    }
    m_submixVoice = nullptr;
    m_initialized = false;
}

} // namespace gx
