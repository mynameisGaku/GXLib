#pragma once
/// @file SoundPlayer.h
/// @brief SE（効果音）再生
///
/// DxLibのPlaySoundMem / StopSoundMemに相当するSE再生機能を提供する。
/// 同じ効果音を重ねて再生できるよう、Play()のたびに新しいSourceVoiceを作成する。
/// 再生が終わったVoiceはXAudio2のコールバック通知で検出し、
/// CleanupFinishedVoices()でまとめて解放する。

#include "pch.h"

namespace GX
{

class Sound;
class AudioDevice;

/// @brief 効果音の再生を管理するクラス（DxLibのPlaySoundMem相当）
class SoundPlayer
{
public:
    SoundPlayer() = default;
    ~SoundPlayer();

    /// @brief AudioDeviceを関連付けて初期化する
    /// @param audioDevice XAudio2エンジンを持つAudioDeviceへのポインタ
    void Initialize(AudioDevice* audioDevice);

    /// @brief 効果音を再生する。同じSoundを複数同時に鳴らせる
    /// @param sound 再生するPCMデータ
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    /// @param pan 左右パン（-1.0=左 〜 0.0=中央 〜 1.0=右、デフォルト0.0）
    void Play(const Sound& sound, float volume = 1.0f, float pan = 0.0f);

    /// @brief 現在再生中のVoice数を取得する
    /// @return アクティブなVoice数
    int GetActiveVoiceCount() const { return m_activeVoiceCount; }

    /// @brief 再生が完了したVoiceを検出して解放する。Update等から定期的に呼ぶこと
    void CleanupFinishedVoices();

private:
    /// XAudio2のSourceVoice再生完了を検知するコールバック。
    /// OnStreamEnd()でフラグを立て、CleanupFinishedVoices()でチェックする。
    struct VoiceCallback : public IXAudio2VoiceCallback
    {
        bool isFinished = false;

        void STDMETHODCALLTYPE OnStreamEnd() override { isFinished = true; }
        void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
        void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
        void STDMETHODCALLTYPE OnBufferEnd(void*) override {}
        void STDMETHODCALLTYPE OnBufferStart(void*) override {}
        void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
        void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
    };

    /// @brief 1つの再生中Voice（SourceVoice + コールバック）のペア
    struct ActiveVoice
    {
        IXAudio2SourceVoice*           voice = nullptr;
        std::unique_ptr<VoiceCallback> callback;
    };

    AudioDevice* m_audioDevice = nullptr;
    std::vector<ActiveVoice> m_activeVoices;  ///< 現在再生中のVoiceリスト
    int m_activeVoiceCount = 0;               ///< m_activeVoices.size()のキャッシュ
};

} // namespace GX
