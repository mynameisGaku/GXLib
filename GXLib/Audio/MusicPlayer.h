#pragma once
/// @file MusicPlayer.h
/// @brief BGM（背景音楽）再生
///
/// 【初学者向け解説】
/// BGMはゲーム中にループ再生される音楽です。
/// SEと違って同時に1曲だけ再生し、フェードイン・フェードアウトが可能です。
///
/// XAudio2のループ機能（XAUDIO2_LOOP_INFINITE）を使って無限ループを実現し、
/// Update()で音量を徐々に変化させてフェード効果を作ります。

#include "pch.h"

namespace GX
{

class Sound;
class AudioDevice;

/// @brief BGM再生クラス
class MusicPlayer
{
public:
    MusicPlayer() = default;
    ~MusicPlayer();

    /// 初期化
    void Initialize(AudioDevice* audioDevice);

    /// BGMを再生
    void Play(const Sound& sound, bool loop = true, float volume = 1.0f);

    /// 停止
    void Stop();

    /// 一時停止
    void Pause();

    /// 再開
    void Resume();

    /// 再生中か
    bool IsPlaying() const { return m_isPlaying; }

    /// 音量設定（0.0〜1.0）
    void SetVolume(float volume);

    /// フェードイン開始
    void FadeIn(float seconds);

    /// フェードアウト開始（完了後に自動停止）
    void FadeOut(float seconds);

    /// フレーム更新（フェード処理）
    void Update(float deltaTime);

private:
    AudioDevice*         m_audioDevice = nullptr;
    IXAudio2SourceVoice* m_voice = nullptr;
    bool                 m_isPlaying = false;
    bool                 m_isPaused = false;

    // フェード管理
    float m_targetVolume  = 1.0f;
    float m_currentVolume = 1.0f;
    float m_fadeSpeed     = 0.0f;   ///< 0=フェードなし、正=フェードイン、負=フェードアウト
    bool  m_stopAfterFade = false;
};

} // namespace GX
