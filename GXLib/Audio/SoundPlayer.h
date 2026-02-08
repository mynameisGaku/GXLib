#pragma once
/// @file SoundPlayer.h
/// @brief SE（効果音）再生
///
/// 【初学者向け解説】
/// SE（Sound Effect）はゲーム中の短い効果音です（ジャンプ音、攻撃音など）。
/// 同じSEを重ねて再生できるように、毎回新しいSourceVoiceを作成します。
///
/// XAudio2のSourceVoiceは再生が終わるとコールバックで通知されるので、
/// そのタイミングでVoiceを解放します。

#include "pch.h"

namespace GX
{

class Sound;
class AudioDevice;

/// @brief SE再生クラス
class SoundPlayer
{
public:
    SoundPlayer() = default;
    ~SoundPlayer();

    /// 初期化
    void Initialize(AudioDevice* audioDevice);

    /// SEを再生（同じSoundを複数同時再生可能）
    void Play(const Sound& sound, float volume = 1.0f, float pan = 0.0f);

    /// アクティブなボイス数を取得
    int GetActiveVoiceCount() const { return m_activeVoiceCount; }

    /// 終了した再生済みVoiceを解放
    void CleanupFinishedVoices();

private:
    /// SourceVoice再生完了コールバック
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

    struct ActiveVoice
    {
        IXAudio2SourceVoice*         voice = nullptr;
        std::unique_ptr<VoiceCallback> callback;
    };

    AudioDevice* m_audioDevice = nullptr;
    std::vector<ActiveVoice> m_activeVoices;
    int m_activeVoiceCount = 0;
};

} // namespace GX
