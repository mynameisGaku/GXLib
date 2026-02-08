/// @file AudioManager.cpp
/// @brief オーディオマネージャーの実装
#include "pch.h"
#include "Audio/AudioManager.h"
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

    m_entries.reserve(k_MaxSounds);
    GX_LOG_INFO("AudioManager initialized (max: %u sounds)", k_MaxSounds);
    return true;
}

int AudioManager::AllocateHandle()
{
    if (!m_freeHandles.empty())
    {
        int handle = m_freeHandles.back();
        m_freeHandles.pop_back();
        return handle;
    }

    int handle = m_nextHandle++;
    if (handle >= static_cast<int>(m_entries.size()))
    {
        m_entries.resize(handle + 1);
    }
    return handle;
}

int AudioManager::LoadSound(const std::wstring& filePath)
{
    // キャッシュチェック
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
    // ハンドル単位の音量は個別Voiceでは制御しにくいので、
    // Play時のvolumeパラメータで対応
    (void)handle;
    (void)volume;
}

void AudioManager::SetMasterVolume(float volume)
{
    m_device.SetMasterVolume(volume);
}

void AudioManager::Update(float deltaTime)
{
    m_musicPlayer.Update(deltaTime);
    m_soundPlayer.CleanupFinishedVoices();
}

void AudioManager::Shutdown()
{
    m_musicPlayer.Stop();
    m_entries.clear();
    m_pathCache.clear();
    m_freeHandles.clear();
    m_device.Shutdown();
    GX_LOG_INFO("AudioManager shutdown");
}

void AudioManager::ReleaseSound(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;

    auto& entry = m_entries[handle];

    if (!entry.filePath.empty())
    {
        m_pathCache.erase(entry.filePath);
        entry.filePath.clear();
    }

    entry.sound.reset();
    m_freeHandles.push_back(handle);
}

} // namespace GX
