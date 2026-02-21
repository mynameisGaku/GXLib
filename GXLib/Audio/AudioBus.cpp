/// @file AudioBus.cpp
/// @brief オーディオバスの実装
#include "pch.h"
#include "Audio/AudioBus.h"
#include "Core/Logger.h"

namespace GX
{

AudioBus::~AudioBus()
{
    Shutdown();
}

bool AudioBus::Initialize(IXAudio2* xaudio2, const std::string& name,
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
}

} // namespace GX
