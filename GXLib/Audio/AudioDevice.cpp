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
    // COMを初期化（XAudio2に必要）
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        m_comInitialized = true;
    }
    else if (hr == S_FALSE)
    {
        // 既に初期化済み — 問題なし
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

    GX_LOG_INFO("AudioDevice initialized (XAudio2)");
    return true;
}

void AudioDevice::Shutdown()
{
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
