/// @file AudioDevice.cpp
/// @brief XAudio2オーディオデバイス管理の実装
#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Core/Logger.h"

namespace GX
{

AudioDevice::~AudioDevice()
{
    Shutdown();
}

bool AudioDevice::Initialize()
{
    // XAudio2はCOMベースのAPIなので、事前にCoInitializeExが必要。
    // S_FALSE（他所で初期化済み）の場合はCoUninitializeの呼び出し義務がないのでフラグを立てない。
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        m_comInitialized = true;
    }
    else if (hr == S_FALSE)
    {
        m_comInitialized = false;
    }
    else
    {
        GX_LOG_ERROR("CoInitializeEx failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // XAudio2エンジン作成
    hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("XAudio2Create failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // マスタリングボイス作成（最終出力先）
    hr = m_xaudio2->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("CreateMasteringVoice failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // X3DAudio初期化（3D空間音響）
    InitializeX3DAudio();

    GX_LOG_INFO("AudioDevice initialized (XAudio2 + X3DAudio)");
    return true;
}

bool AudioDevice::InitializeX3DAudio()
{
    if (!m_masterVoice) return false;

    // MasteringVoiceのチャンネルマスクを取得（スピーカー配置情報）
    DWORD channelMask = 0;
    HRESULT hr = m_masterVoice->GetChannelMask(&channelMask);
    if (FAILED(hr))
    {
        GX_LOG_WARN("AudioDevice: GetChannelMask failed, X3DAudio disabled");
        return false;
    }

    // 出力チャンネル数を取得
    XAUDIO2_VOICE_DETAILS details;
    m_masterVoice->GetVoiceDetails(&details);
    m_outputChannels = details.InputChannels;

    // X3DAudioの初期化。チャンネルマスクと音速から空間音響計算エンジンを構築する。
    hr = X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, m_x3dAudioHandle);
    if (FAILED(hr))
    {
        GX_LOG_WARN("AudioDevice: X3DAudioInitialize failed: 0x%08X",
                     static_cast<unsigned>(hr));
        return false;
    }

    m_x3dInitialized = true;
    GX_LOG_INFO("AudioDevice: X3DAudio initialized (%u output channels)", m_outputChannels);
    return true;
}

void AudioDevice::Shutdown()
{
    // 破棄順序: MasteringVoice → XAudio2エンジン → COM
    // MasteringVoiceはXAudio2が所有するのでDestroyVoice()で手動解放
    if (m_masterVoice)
    {
        m_masterVoice->DestroyVoice();
        m_masterVoice = nullptr;
    }

    m_xaudio2.Reset();

    if (m_comInitialized)
    {
        CoUninitialize();
        m_comInitialized = false;
    }
}

void AudioDevice::SetMasterVolume(float volume)
{
    if (m_masterVoice)
    {
        m_masterVoice->SetVolume(volume);
    }
}

} // namespace GX
