#pragma once
/// @file AudioManager.h
/// @brief オーディオマネージャー — ハンドルベースのサウンド管理
///
/// 【初学者向け解説】
/// AudioManagerは、TextureManagerと同じハンドルベースの管理パターンで
/// サウンドを管理します。DXLib互換のAPIを提供します。
///
/// 使い方：
/// 1. Initialize() — オーディオデバイス初期化
/// 2. LoadSound("se.wav") → int handle — サウンド読み込み
/// 3. PlaySound(handle) — 効果音（SE）を再生
/// 4. PlayMusic(handle) — BGMを再生
/// 5. Update(dt) — フレーム更新（フェード処理等）
/// 6. Shutdown() — 終了処理

#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/Sound.h"
#include "Audio/SoundPlayer.h"
#include "Audio/MusicPlayer.h"

namespace GX
{

/// @brief ハンドルベースのオーディオ管理クラス
class AudioManager
{
public:
    static constexpr uint32_t k_MaxSounds = 256;

    AudioManager() = default;
    ~AudioManager() = default;

    /// オーディオシステムを初期化
    bool Initialize();

    /// サウンドファイルを読み込み、ハンドルを返す
    int LoadSound(const std::wstring& filePath);

    /// SE再生
    void PlaySound(int handle, float volume = 1.0f, float pan = 0.0f);

    /// BGM再生
    void PlayMusic(int handle, bool loop = true, float volume = 1.0f);

    /// BGM停止
    void StopMusic();

    /// BGM一時停止
    void PauseMusic();

    /// BGM再開
    void ResumeMusic();

    /// BGMフェードイン
    void FadeInMusic(float seconds);

    /// BGMフェードアウト（完了後に自動停止）
    void FadeOutMusic(float seconds);

    /// BGMが再生中か
    bool IsMusicPlaying() const { return m_musicPlayer.IsPlaying(); }

    /// 音量設定（0.0〜1.0）
    void SetSoundVolume(int handle, float volume);

    /// マスターボリューム設定
    void SetMasterVolume(float volume);

    /// フレーム更新（フェード処理等）
    void Update(float deltaTime);

    /// 終了処理
    void Shutdown();

    /// サウンドを解放
    void ReleaseSound(int handle);

private:
    int AllocateHandle();

    AudioDevice  m_device;
    SoundPlayer  m_soundPlayer;
    MusicPlayer  m_musicPlayer;

    struct SoundEntry
    {
        std::unique_ptr<Sound> sound;
        std::wstring filePath;
    };

    std::vector<SoundEntry>                m_entries;
    std::unordered_map<std::wstring, int>  m_pathCache;
    std::vector<int>                       m_freeHandles;
    int                                    m_nextHandle = 0;
};

} // namespace GX
