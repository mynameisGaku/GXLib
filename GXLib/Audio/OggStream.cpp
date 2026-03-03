/// @file OggStream.cpp
/// @brief OGG Vorbisストリーミング再生の実装
#include "pch_audio.h"
#include "Audio/OggStream.h"
#include "Core/Logger.h"

#define STB_VORBIS_HEADER_ONLY
#include "ThirdParty/stb_vorbis.c"

namespace gx
{

OggStream::~OggStream()
{
    StopStreaming();
    Close();
}

bool OggStream::Open(const gx::WString& filePath)
{
    Close();

    // ワイド文字列をマルチバイトに変換
    int size = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    gx::String path(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, path.DataMut(), size, nullptr, nullptr);

    int error = 0;
    m_vorbis = stb_vorbis_open_filename(path.c_str(), &error, nullptr);
    if (!m_vorbis)
    {
        GX_LOG_ERROR("OggStream: Failed to open OGG file (error %d)", error);
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(m_vorbis);
    m_channels = info.channels;
    m_sampleRate = info.sample_rate;

    // バッファ確保
    for (int i = 0; i < k_StreamBufferCount; ++i)
    {
        m_buffers[i].resize(k_StreamBufferSamples * m_channels);
    }

    return true;
}

void OggStream::Close()
{
    StopStreaming();
    if (m_vorbis)
    {
        stb_vorbis_close(m_vorbis);
        m_vorbis = nullptr;
    }
}

bool OggStream::StartStreaming(IXAudio2* xaudio2, IXAudio2SubmixVoice* output)
{
    if (!m_vorbis || !xaudio2) return false;

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = static_cast<WORD>(m_channels);
    wfx.nSamplesPerSec = m_sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    XAUDIO2_SEND_DESCRIPTOR send = {};
    send.pOutputVoice = output;
    XAUDIO2_VOICE_SENDS sendList = {};
    sendList.SendCount = output ? 1 : 0;
    sendList.pSends = output ? &send : nullptr;

    HRESULT hr = xaudio2->CreateSourceVoice(&m_voice, &wfx, 0, 2.0f,
        nullptr, output ? &sendList : nullptr);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("OggStream: CreateSourceVoice failed: 0x%08X", hr);
        return false;
    }

    // ファイル先頭に巻き戻し
    stb_vorbis_seek_start(m_vorbis);

    // 初回バッファ充填
    for (int i = 0; i < k_StreamBufferCount; ++i)
    {
        if (FillBuffer(i))
        {
            XAUDIO2_BUFFER buf = {};
            buf.AudioBytes = static_cast<UINT32>(m_buffers[i].size() * sizeof(int16_t));
            buf.pAudioData = reinterpret_cast<const BYTE*>(m_buffers[i].data());
            m_voice->SubmitSourceBuffer(&buf);
        }
    }

    m_running = true;
    m_playing = true;
    m_currentBuffer = 0;
    m_decodeBufferIndex = 0;
    m_needBuffer = false;

    // デコードスレッド開始
    m_decoderThread = std::thread(&OggStream::DecoderThread, this);

    m_voice->Start();
    return true;
}

void OggStream::StopStreaming()
{
    m_running = false;
    m_playing = false;
    m_cv.notify_all();

    if (m_decoderThread.joinable())
        m_decoderThread.join();

    if (m_voice)
    {
        m_voice->Stop();
        m_voice->FlushSourceBuffers();
        m_voice->DestroyVoice();
        m_voice = nullptr;
    }
}

void OggStream::PauseStreaming()
{
    if (m_voice && m_playing)
    {
        m_voice->Stop();
        m_playing = false;
    }
}

void OggStream::ResumeStreaming()
{
    if (m_voice && !m_playing && m_running)
    {
        m_voice->Start();
        m_playing = true;
    }
}

void OggStream::SetVolume(float vol)
{
    if (m_voice)
        m_voice->SetVolume(vol);
}

void OggStream::SetLoop(bool loop)
{
    m_loop = loop;
}

void OggStream::Update()
{
    if (!m_voice || !m_running) return;

    XAUDIO2_VOICE_STATE state;
    m_voice->GetState(&state);

    // バッファが消費されたら新しいバッファを供給
    if (state.BuffersQueued < k_StreamBufferCount)
    {
        m_needBuffer = true;
        m_cv.notify_one();
    }
}

bool OggStream::IsPlaying() const
{
    return m_playing && m_running;
}

void OggStream::DecoderThread()
{
    while (m_running)
    {
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] { return m_needBuffer.load() || !m_running.load(); });
        }

        if (!m_running) break;

        if (FillBuffer(m_decodeBufferIndex))
        {
            XAUDIO2_BUFFER buf = {};
            buf.AudioBytes = static_cast<UINT32>(m_buffers[m_decodeBufferIndex].size() * sizeof(int16_t));
            buf.pAudioData = reinterpret_cast<const BYTE*>(m_buffers[m_decodeBufferIndex].data());

            if (m_voice)
                m_voice->SubmitSourceBuffer(&buf);

            m_decodeBufferIndex = (m_decodeBufferIndex + 1) % k_StreamBufferCount;
        }
        else
        {
            // デコード終了
            if (!m_loop)
            {
                m_running = false;
                m_playing = false;
            }
        }

        m_needBuffer = false;
    }
}

bool OggStream::FillBuffer(int bufIndex)
{
    if (!m_vorbis) return false;

    int totalSamples = k_StreamBufferSamples;
    int samplesRead = stb_vorbis_get_samples_short_interleaved(
        m_vorbis, m_channels, m_buffers[bufIndex].data(), totalSamples * m_channels);

    if (samplesRead == 0)
    {
        if (m_loop)
        {
            stb_vorbis_seek_start(m_vorbis);
            samplesRead = stb_vorbis_get_samples_short_interleaved(
                m_vorbis, m_channels, m_buffers[bufIndex].data(), totalSamples * m_channels);
        }
        if (samplesRead == 0) return false;
    }

    // 読んだ分だけサイズ調整（最後のバッファは短い可能性）
    if (samplesRead < totalSamples)
    {
        m_buffers[bufIndex].resize(samplesRead * m_channels);
    }
    else
    {
        m_buffers[bufIndex].resize(totalSamples * m_channels);
    }

    return true;
}

} // namespace gx
