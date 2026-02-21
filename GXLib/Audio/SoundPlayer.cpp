/// @file SoundPlayer.cpp
/// @brief SE（効果音）再生の実装
#include "pch.h"
#include "Audio/SoundPlayer.h"
#include "Audio/Sound.h"
#include "Audio/AudioDevice.h"
#include "Core/Logger.h"

namespace GX
{

SoundPlayer::~SoundPlayer()
{
    // 再生中のVoiceを全て停止・破棄する
    for (auto& av : m_activeVoices)
    {
        if (av.voice)
        {
            av.voice->Stop(0);
            av.voice->DestroyVoice();
        }
    }
    m_activeVoices.clear();
}

void SoundPlayer::Initialize(AudioDevice* audioDevice)
{
    m_audioDevice = audioDevice;
}

void SoundPlayer::Play(const Sound& sound, float volume, float pan)
{
    if (!m_audioDevice || !m_audioDevice->GetXAudio2() || !sound.IsValid())
        return;

    // まず終了済みVoiceをクリーンアップ
    CleanupFinishedVoices();

    auto callback = std::make_unique<VoiceCallback>();

    // SE再生のたびに新しいSourceVoiceを作成する。
    // これにより同じ効果音を複数重ねて再生できる。
    IXAudio2SourceVoice* voice = nullptr;
    WAVEFORMATEX format = sound.GetFormat();

    HRESULT hr = m_audioDevice->GetXAudio2()->CreateSourceVoice(
        &voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, callback.get());

    if (FAILED(hr))
    {
        GX_LOG_ERROR("CreateSourceVoice failed: 0x%08X", static_cast<unsigned>(hr));
        return;
    }

    // バッファをサブミット
    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = sound.GetDataSize();
    buffer.pAudioData = sound.GetData();
    buffer.Flags      = XAUDIO2_END_OF_STREAM;

    hr = voice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        voice->DestroyVoice();
        GX_LOG_ERROR("SubmitSourceBuffer failed: 0x%08X", static_cast<unsigned>(hr));
        return;
    }

    // 音量設定
    voice->SetVolume(volume);

    // モノラル音源のパン設定。OutputMatrixでL/Rの出力配分を変える。
    // pan=-1.0で左100%、pan=1.0で右100%、pan=0.0で左右均等。
    if (format.nChannels == 1)
    {
        float left  = 0.5f - pan * 0.5f;
        float right = 0.5f + pan * 0.5f;
        float outputMatrix[2] = { left, right };
        voice->SetOutputMatrix(nullptr, 1, 2, outputMatrix);
    }

    // 再生開始
    voice->Start(0);

    // アクティブリストに追加
    ActiveVoice av;
    av.voice = voice;
    av.callback = std::move(callback);
    m_activeVoices.push_back(std::move(av));
    m_activeVoiceCount = static_cast<int>(m_activeVoices.size());
}

void SoundPlayer::CleanupFinishedVoices()
{
    for (auto it = m_activeVoices.begin(); it != m_activeVoices.end(); )
    {
        if (it->callback && it->callback->isFinished)
        {
            if (it->voice)
            {
                it->voice->Stop(0);
                it->voice->DestroyVoice();
            }
            it = m_activeVoices.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_activeVoiceCount = static_cast<int>(m_activeVoices.size());
}

} // namespace GX
