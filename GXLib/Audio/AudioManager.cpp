/// @file AudioManager.cpp
/// @brief オーディオマネージャーの実装（3Dサウンド + ミキサー対応）
#include "pch.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioEmitter.h"
#include "Core/Logger.h"

namespace GX
{

bool AudioManager::Initialize()
{
    if (!m_device.Initialize())
    {
        GX_LOG_ERROR("AudioManager: Failed to initialize AudioDevice");
        return false;
    }

    m_soundPlayer.Initialize(&m_device);
    m_musicPlayer.Initialize(&m_device);

    // オーディオミキサー初期化（BGM/SE/Voiceバス）
    if (!m_mixer.Initialize(m_device))
    {
        GX_LOG_WARN("AudioManager: Mixer initialization failed, continuing without mixer");
    }

    m_entries.reserve(k_MaxSounds);
    GX_LOG_INFO("AudioManager initialized (max: %u sounds, X3DAudio: %s)",
                k_MaxSounds, m_device.IsX3DAudioInitialized() ? "enabled" : "disabled");
    return true;
}

int AudioManager::AllocateHandle()
{
    // フリーリストに解放済みハンドルがあれば再利用する
    if (!m_freeHandles.empty())
    {
        int handle = m_freeHandles.back();
        m_freeHandles.pop_back();
        return handle;
    }

    // 新規割り当て。必要に応じてエントリ配列を拡張する
    int handle = m_nextHandle++;
    if (handle >= static_cast<int>(m_entries.size()))
    {
        m_entries.resize(handle + 1);
    }
    return handle;
}

int AudioManager::LoadSound(const std::wstring& filePath)
{
    // 同一パスが既に読み込み済みならキャッシュからハンドルを返す（二重読み込み防止）
    auto it = m_pathCache.find(filePath);
    if (it != m_pathCache.end())
    {
        return it->second;
    }

    int handle = AllocateHandle();

    auto& entry = m_entries[handle];
    entry.sound = std::make_unique<Sound>();
    entry.filePath = filePath;

    if (!entry.sound->LoadFromFile(filePath))
    {
        entry.sound.reset();
        entry.filePath.clear();
        m_freeHandles.push_back(handle);
        return -1;
    }

    m_pathCache[filePath] = handle;
    GX_LOG_INFO("Sound loaded (handle: %d)", handle);
    return handle;
}

void AudioManager::PlaySound(int handle, float volume, float pan)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;

    auto& entry = m_entries[handle];
    if (!entry.sound || !entry.sound->IsValid())
        return;

    m_soundPlayer.Play(*entry.sound, volume, pan);
}

void AudioManager::PlaySoundOnBus(int handle, AudioBus& bus, float volume)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;

    auto& entry = m_entries[handle];
    if (!entry.sound || !entry.sound->IsValid())
        return;

    m_soundPlayer.PlayOnBus(*entry.sound, bus, volume);
}

int AudioManager::PlaySound3D(int handle, AudioEmitter& emitter, float volume)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return -1;

    auto& entry = m_entries[handle];
    if (!entry.sound || !entry.sound->IsValid())
        return -1;

    return m_soundPlayer.Play3D(*entry.sound, emitter, volume);
}

void AudioManager::StopSound3D(int voiceId)
{
    m_soundPlayer.Stop3D(voiceId);
}

void AudioManager::PlayMusic(int handle, bool loop, float volume)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;

    auto& entry = m_entries[handle];
    if (!entry.sound || !entry.sound->IsValid())
        return;

    m_musicPlayer.Play(*entry.sound, loop, volume);
}

void AudioManager::StopMusic()
{
    m_musicPlayer.Stop();
}

void AudioManager::PauseMusic()
{
    m_musicPlayer.Pause();
}

void AudioManager::ResumeMusic()
{
    m_musicPlayer.Resume();
}

void AudioManager::FadeInMusic(float seconds)
{
    m_musicPlayer.FadeIn(seconds);
}

void AudioManager::FadeOutMusic(float seconds)
{
    m_musicPlayer.FadeOut(seconds);
}

void AudioManager::SetSoundVolume(int handle, float volume)
{
    // SE用のSourceVoiceはPlay()のたびに新規作成されるため、
    // 既存ハンドルからの音量変更は困難。Play()のvolumeパラメータで代用する。
    (void)handle;
    (void)volume;
}

void AudioManager::SetMasterVolume(float volume)
{
    m_device.SetMasterVolume(volume);
}

void AudioManager::SetListener(const AudioListener& listener)
{
    m_currentListener = &listener;
}

void AudioManager::Update(float deltaTime)
{
    // BGMのフェード音量補間と、SE再生済みVoiceの解放を行う
    m_musicPlayer.Update(deltaTime);
    m_soundPlayer.CleanupFinishedVoices();

    // 3Dサウンドの空間計算を更新
    if (m_currentListener)
    {
        m_soundPlayer.Update3D(*m_currentListener);
    }
}

void AudioManager::Shutdown()
{
    // 破棄順序: 全Voice停止 → BGM停止 → ミキサー → 全エントリ解放 → デバイス破棄
    // SoundPlayerの全Voiceを先に停止しないと、Sound(PCMデータ)やデバイス破棄後に
    // デストラクタでアクセス違反が発生する
    m_soundPlayer.StopAll();
    m_musicPlayer.Stop();
    m_mixer.Shutdown();
    m_entries.clear();
    m_pathCache.clear();
    m_freeHandles.clear();
    m_currentListener = nullptr;
    m_device.Shutdown();
    GX_LOG_INFO("AudioManager shutdown");
}

void AudioManager::ReleaseSound(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;

    auto& entry = m_entries[handle];

    // パスキャッシュからも削除して、次回LoadSoundで再読み込みできるようにする
    if (!entry.filePath.empty())
    {
        m_pathCache.erase(entry.filePath);
        entry.filePath.clear();
    }

    entry.sound.reset();
    m_freeHandles.push_back(handle);  // ハンドルをフリーリストへ返却
}

} // namespace GX
