/// @file AudioBus.cpp
/// @brief オーディオバスの実装
#include "pch_audio.h"
#include "Audio/AudioBus.h"
#include "Audio/AudioEffect.h"   // IAudioEffect full definition (ADR-0017 L2)
#include "Core/Logger.h"

namespace gx
{

AudioBus::~AudioBus()
{
    Shutdown();
}

bool AudioBus::Initialize(IXAudio2* xaudio2, const gx::String& name,
                           uint32_t channels, uint32_t sampleRate)
{
    if (!xaudio2) return false;

    m_name = name;

    // SubmixVoice作成。processingStage=0でMasteringVoiceと同じステージ。
    // 複数バスをチェーンする場合はステージ番号を分ける必要がある。
    HRESULT hr = xaudio2->CreateSubmixVoice(
        &m_submixVoice, channels, sampleRate,
        0,       // flags
        0,       // processingStage（0=MasteringVoiceの直前）
        nullptr, // pSendList（nullptr=MasteringVoiceに直結）
        nullptr  // pEffectChain
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("AudioBus '%s': CreateSubmixVoice failed: 0x%08X",
                     name.c_str(), static_cast<unsigned>(hr));
        return false;
    }

    GX_LOG_INFO("AudioBus '%s' initialized (%u ch, %u Hz)",
                name.c_str(), channels, sampleRate);
    return true;
}

void AudioBus::SetVolume(float volume)
{
    m_volume = volume;
    if (m_submixVoice)
    {
        m_submixVoice->SetVolume(volume);
    }
}

void AudioBus::SetOutputBus(AudioBus* parent)
{
    if (!m_submixVoice) return;

    if (parent && parent->GetSubmixVoice())
    {
        // 出力先を指定バスのSubmixVoiceに変更
        XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
        sendDesc.Flags = 0;
        sendDesc.pOutputVoice = parent->GetSubmixVoice();

        XAUDIO2_VOICE_SENDS sendList = {};
        sendList.SendCount = 1;
        sendList.pSends = &sendDesc;

        m_submixVoice->SetOutputVoices(&sendList);
    }
    else
    {
        // MasteringVoice直結に戻す
        m_submixVoice->SetOutputVoices(nullptr);
    }
}

void AudioBus::Shutdown()
{
    if (m_submixVoice)
    {
        m_submixVoice->DestroyVoice();
        m_submixVoice = nullptr;
    }
    // カスタムエフェクトも破棄
    for (auto& e : m_effects) e.reset();
}

// =========================================================================
// Layer 2: カスタム DSP エフェクトチェーン (ADR-0017 L2, ADR-0010 §8)
// =========================================================================

int AudioBus::AddEffect(std::unique_ptr<IAudioEffect> effect)
{
    if (!effect) {
        GX_LOG_ERROR("AudioBus::AddEffect: effect is null (bus='%s')", m_name.c_str());
        return -1;
    }
    for (size_t i = 0; i < k_MaxEffectsPerBus; ++i)
    {
        if (!m_effects[i]) {
            m_effects[i] = std::move(effect);
            return static_cast<int>(i);
        }
    }
    GX_LOG_ERROR("AudioBus::AddEffect: chain full (4/4) on bus '%s' — per ADR-0010 §8 max is 4 effects per bus",
                 m_name.c_str());
    return -1;
}

void AudioBus::RemoveEffect(int index)
{
    if (index < 0 || index >= static_cast<int>(k_MaxEffectsPerBus)) return;
    m_effects[index].reset();
}

size_t AudioBus::GetEffectCount() const
{
    size_t n = 0;
    for (const auto& e : m_effects) if (e) ++n;
    return n;
}

IAudioEffect* AudioBus::GetEffect(int index)
{
    if (index < 0 || index >= static_cast<int>(k_MaxEffectsPerBus)) return nullptr;
    return m_effects[index].get();
}

} // namespace gx
