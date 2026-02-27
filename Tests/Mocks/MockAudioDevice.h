#pragma once
/// @file MockAudioDevice.h
/// @brief IAudioDeviceのモック実装（テスト用）
///
/// GPUやXAudio2を必要とせず、オーディオ関連の単体テストを可能にする。

#include "Audio/IAudioDevice.h"

namespace gx::Testing {

/// @brief IAudioDeviceのモック実装
class MockAudioDevice : public IAudioDevice
{
public:
    bool Initialize() override { m_initialized = true; return true; }
    void Shutdown() override { m_initialized = false; }
    void SetMasterVolume(float volume) override { m_masterVolume = volume; }
    IXAudio2* GetNativeEngine() const override { return nullptr; }
    IXAudio2MasteringVoice* GetMasterVoice() const override { return nullptr; }
    bool Is3DAudioInitialized() const override { return false; }
    uint32_t GetOutputChannelCount() const override { return 2; }

    // テスト用アクセサ
    bool IsInitialized() const { return m_initialized; }
    float GetMasterVolume() const { return m_masterVolume; }

private:
    bool  m_initialized = false;
    float m_masterVolume = 1.0f;
};

} // namespace gx::Testing
