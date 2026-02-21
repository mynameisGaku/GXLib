#pragma once
/// @file MusicPlayer.h
/// @brief BGM（背景音楽）再生
///
/// DxLibのPlayMusic / StopMusicに相当するBGM再生機能を提供する。
/// SEとは異なり同時に1曲だけを管理し、ループ再生とフェードイン/アウトに対応する。
/// XAudio2のXAUDIO2_LOOP_INFINITEでループを実現し、
/// Update()で毎フレーム音量を補間してフェード効果を処理する。

#include "pch.h"

namespace GX
{

class Sound;
class AudioDevice;

/// @brief BGM再生を管理するクラス（DxLibのPlayMusic / StopMusic相当）
class MusicPlayer
{
public:
    MusicPlayer() = default;
    ~MusicPlayer();

    /// @brief AudioDeviceを関連付けて初期化する
    /// @param audioDevice XAudio2エンジンを持つAudioDeviceへのポインタ
    void Initialize(AudioDevice* audioDevice);

    /// @brief BGMを再生する。既に再生中の場合は停止してから再生する
    /// @param sound 再生するPCMデータ
    /// @param loop trueならループ再生する（デフォルトtrue）
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    void Play(const Sound& sound, bool loop = true, float volume = 1.0f);

    /// @brief BGMを停止し、SourceVoiceを破棄する
    void Stop();

    /// @brief BGMを一時停止する。Resume()で再開できる
    void Pause();

    /// @brief 一時停止中のBGMを再開する
    void Resume();

    /// @brief BGMが再生中か判定する（一時停止中もtrue）
    /// @return 再生中ならtrue
    bool IsPlaying() const { return m_isPlaying; }

    /// @brief BGMの音量を即座に設定する（フェードを中断する）
    /// @param volume 音量（0.0〜1.0）
    void SetVolume(float volume);

    /// @brief フェードインを開始する。音量0から目標音量まで徐々に上げる
    /// @param seconds フェードにかける秒数
    void FadeIn(float seconds);

    /// @brief フェードアウトを開始する。現在音量から0まで下げ、完了後に自動停止する
    /// @param seconds フェードにかける秒数
    void FadeOut(float seconds);

    /// @brief フレーム更新。フェード中の音量補間を処理する
    /// @param deltaTime 前フレームからの経過秒数
    void Update(float deltaTime);

private:
    AudioDevice*         m_audioDevice = nullptr;
    IXAudio2SourceVoice* m_voice = nullptr;    ///< 再生用のSourceVoice（1曲分）
    bool                 m_isPlaying = false;
    bool                 m_isPaused = false;

    // --- フェード管理 ---
    float m_targetVolume  = 1.0f;   ///< 目標音量（フェードインのゴール値）
    float m_currentVolume = 1.0f;   ///< 現在の実効音量
    float m_fadeSpeed     = 0.0f;   ///< 毎秒の音量変化量。0=フェードなし、正=イン、負=アウト
    bool  m_stopAfterFade = false;  ///< フェードアウト完了時に自動停止するか
};

} // namespace GX
