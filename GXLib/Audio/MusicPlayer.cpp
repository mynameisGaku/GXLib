/// @file MusicPlayer.cpp
/// @brief BGM（背景音楽）再生の実装
#include "pch.h"
#include "Audio/MusicPlayer.h"
#include "Audio/Sound.h"
#include "Audio/AudioDevice.h"
#include "Core/Logger.h"

namespace GX
{

MusicPlayer::~MusicPlayer()
{
    Stop();
}

void MusicPlayer::Initialize(AudioDevice* audioDevice)
{
    m_audioDevice = audioDevice;
}

void MusicPlayer::Play(const Sound& sound, bool loop, float volume)
{
    if (!m_audioDevice || !m_audioDevice->GetXAudio2() || !sound.IsValid())
        return;

    // 現在再生中なら停止
    Stop();

    // ソースボイスを作成
    // 初学者向け: 再生用の「音声チャンネル」をXAudio2に作ってもらいます。
    WAVEFORMATEX format = sound.GetFormat();
    HRESULT hr = m_audioDevice->GetXAudio2()->CreateSourceVoice(&m_voice, &format);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("MusicPlayer: CreateSourceVoice failed: 0x%08X", static_cast<unsigned>(hr));
        return;
    }

    // バッファをサブミット
    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = sound.GetDataSize();
    buffer.pAudioData = sound.GetData();
    buffer.Flags      = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount  = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = m_voice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        m_voice->DestroyVoice();
        m_voice = nullptr;
        GX_LOG_ERROR("MusicPlayer: SubmitSourceBuffer failed");
        return;
    }

    m_targetVolume = volume;
    m_currentVolume = volume;
    m_voice->SetVolume(volume);
    m_voice->Start(0);
    m_isPlaying = true;
    m_isPaused = false;
    m_fadeSpeed = 0.0f;
    m_stopAfterFade = false;
}

void MusicPlayer::Stop()
{
    if (m_voice)
    {
        m_voice->Stop(0);
        m_voice->FlushSourceBuffers();
        m_voice->DestroyVoice();
        m_voice = nullptr;
    }
    m_isPlaying = false;
    m_isPaused = false;
    m_fadeSpeed = 0.0f;
    m_stopAfterFade = false;
}

void MusicPlayer::Pause()
{
    if (m_voice && m_isPlaying && !m_isPaused)
    {
        m_voice->Stop(0);
        m_isPaused = true;
    }
}

void MusicPlayer::Resume()
{
    if (m_voice && m_isPlaying && m_isPaused)
    {
        m_voice->Start(0);
        m_isPaused = false;
    }
}

void MusicPlayer::SetVolume(float volume)
{
    m_targetVolume = volume;
    m_currentVolume = volume;
    if (m_voice)
    {
        m_voice->SetVolume(volume);
    }
}

void MusicPlayer::FadeIn(float seconds)
{
    if (seconds <= 0.0f)
    {
        m_currentVolume = m_targetVolume;
        if (m_voice) m_voice->SetVolume(m_currentVolume);
        m_fadeSpeed = 0.0f;
        return;
    }

    m_currentVolume = 0.0f;
    if (m_voice) m_voice->SetVolume(0.0f);
    m_fadeSpeed = m_targetVolume / seconds;
    m_stopAfterFade = false;
}

void MusicPlayer::FadeOut(float seconds)
{
    if (seconds <= 0.0f)
    {
        Stop();
        return;
    }

    m_fadeSpeed = -(m_currentVolume / seconds);
    m_stopAfterFade = true;
}

void MusicPlayer::Update(float deltaTime)
{
    if (!m_isPlaying || m_isPaused || m_fadeSpeed == 0.0f)
        return;

    m_currentVolume += m_fadeSpeed * deltaTime;

    if (m_fadeSpeed > 0.0f)
    {
        // フェードイン
        if (m_currentVolume >= m_targetVolume)
        {
            m_currentVolume = m_targetVolume;
            m_fadeSpeed = 0.0f;
        }
    }
    else
    {
        // フェードアウト
        if (m_currentVolume <= 0.0f)
        {
            m_currentVolume = 0.0f;
            m_fadeSpeed = 0.0f;
            if (m_stopAfterFade)
            {
                Stop();
                return;
            }
        }
    }

    if (m_voice)
    {
        m_voice->SetVolume(m_currentVolume);
    }
}

} // namespace GX
