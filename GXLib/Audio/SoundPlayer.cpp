/// @file SoundPlayer.cpp
/// @brief SE（効果音）再生の実装（2D + 3D空間音響対応）
#include "pch.h"
#include "Audio/SoundPlayer.h"
#include "Audio/Sound.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioEmitter.h"
#include "Audio/AudioBus.h"
#include "Core/Logger.h"
#include <x3daudio.h>

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
    m_3dVoices.clear();
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
    av.is3D = false;
    av.voiceId = -1;
    m_activeVoices.push_back(std::move(av));
    m_activeVoiceCount = static_cast<int>(m_activeVoices.size());
}

void SoundPlayer::PlayOnBus(const Sound& sound, AudioBus& bus, float volume)
{
    if (!m_audioDevice || !m_audioDevice->GetXAudio2() || !sound.IsValid())
        return;
    if (!bus.GetSubmixVoice())
        return;

    CleanupFinishedVoices();

    auto callback = std::make_unique<VoiceCallback>();

    WAVEFORMATEX format = sound.GetFormat();

    // SourceVoiceの出力先をバスのSubmixVoiceに指定する
    XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
    sendDesc.Flags = 0;
    sendDesc.pOutputVoice = bus.GetSubmixVoice();

    XAUDIO2_VOICE_SENDS sendList = {};
    sendList.SendCount = 1;
    sendList.pSends = &sendDesc;

    IXAudio2SourceVoice* voice = nullptr;
    HRESULT hr = m_audioDevice->GetXAudio2()->CreateSourceVoice(
        &voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, callback.get(), &sendList);

    if (FAILED(hr))
    {
        GX_LOG_ERROR("CreateSourceVoice (bus) failed: 0x%08X", static_cast<unsigned>(hr));
        return;
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = sound.GetDataSize();
    buffer.pAudioData = sound.GetData();
    buffer.Flags      = XAUDIO2_END_OF_STREAM;

    hr = voice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        voice->DestroyVoice();
        return;
    }

    voice->SetVolume(volume);
    voice->Start(0);

    ActiveVoice av;
    av.voice = voice;
    av.callback = std::move(callback);
    av.is3D = false;
    av.voiceId = -1;
    m_activeVoices.push_back(std::move(av));
    m_activeVoiceCount = static_cast<int>(m_activeVoices.size());
}

int SoundPlayer::Play3D(const Sound& sound, AudioEmitter& emitter, float volume)
{
    if (!m_audioDevice || !m_audioDevice->GetXAudio2() || !sound.IsValid())
        return -1;

    if (!m_audioDevice->IsX3DAudioInitialized())
    {
        GX_LOG_WARN("SoundPlayer::Play3D: X3DAudio not initialized, falling back to 2D");
        Play(sound, volume);
        return -1;
    }

    CleanupFinishedVoices();

    auto callback = std::make_unique<VoiceCallback>();

    WAVEFORMATEX format = sound.GetFormat();

    IXAudio2SourceVoice* voice = nullptr;
    HRESULT hr = m_audioDevice->GetXAudio2()->CreateSourceVoice(
        &voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, callback.get());

    if (FAILED(hr))
    {
        GX_LOG_ERROR("CreateSourceVoice (3D) failed: 0x%08X", static_cast<unsigned>(hr));
        return -1;
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = sound.GetDataSize();
    buffer.pAudioData = sound.GetData();
    buffer.Flags      = XAUDIO2_END_OF_STREAM;

    hr = voice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        voice->DestroyVoice();
        return -1;
    }

    voice->SetVolume(volume);
    voice->Start(0);

    // ボイスIDを割り当て
    int voiceId = m_nextVoiceId++;

    ActiveVoice av;
    av.voice = voice;
    av.callback = std::move(callback);
    av.is3D = true;
    av.voiceId = voiceId;
    int activeIdx = static_cast<int>(m_activeVoices.size());
    m_activeVoices.push_back(std::move(av));
    m_activeVoiceCount = static_cast<int>(m_activeVoices.size());

    // 3D情報を登録
    Voice3DInfo info;
    info.activeVoiceIndex = activeIdx;
    info.emitter = &emitter;
    info.srcChannels = format.nChannels;

    // ボイスIDをインデックスとして使用（必要に応じて拡張）
    if (voiceId >= static_cast<int>(m_3dVoices.size()))
    {
        m_3dVoices.resize(voiceId + 1);
    }
    m_3dVoices[voiceId] = info;

    return voiceId;
}

void SoundPlayer::Update3D(const AudioListener& listener)
{
    if (!m_audioDevice || !m_audioDevice->IsX3DAudioInitialized())
        return;

    uint32_t dstChannels = m_audioDevice->GetOutputChannelCount();
    if (dstChannels == 0 || dstChannels > k_MaxOutputChannels)
        return;

    const X3DAUDIO_HANDLE& x3dHandle = m_audioDevice->GetX3DHandle();

    for (int id = 0; id < static_cast<int>(m_3dVoices.size()); ++id)
    {
        auto& info = m_3dVoices[id];
        if (!info.emitter) continue;
        if (info.activeVoiceIndex < 0 ||
            info.activeVoiceIndex >= static_cast<int>(m_activeVoices.size()))
            continue;

        auto& av = m_activeVoices[info.activeVoiceIndex];
        if (!av.voice || !av.is3D || av.voiceId != id)
            continue;

        // コールバックで終了検出済みならスキップ
        if (av.callback && av.callback->isFinished)
            continue;

        // X3DAudio DSP設定を準備
        X3DAUDIO_DSP_SETTINGS dspSettings = {};
        memset(m_matrixCoefficients, 0, sizeof(m_matrixCoefficients));
        dspSettings.pMatrixCoefficients = m_matrixCoefficients;
        dspSettings.SrcChannelCount = info.srcChannels;
        dspSettings.DstChannelCount = dstChannels;

        // エミッターのチャンネル数を同期
        info.emitter->GetNativeMutable().ChannelCount = info.srcChannels;

        // X3DAudioCalculateで空間音響パラメータを計算する。
        // MATRIX: 出力チャンネルごとのボリューム配分（パンニング）
        // DOPPLER: ドップラー効果による周波数変化
        // LPF_DIRECT: 距離によるローパスフィルタ（遠い音がこもる効果）
        X3DAudioCalculate(
            x3dHandle,
            &listener.GetNative(),
            &info.emitter->GetNative(),
            X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER,
            &dspSettings);

        // 計算結果をSourceVoiceに適用
        av.voice->SetOutputMatrix(
            nullptr, info.srcChannels, dstChannels, dspSettings.pMatrixCoefficients);

        // ドップラー効果による再生速度の変化
        if (dspSettings.DopplerFactor > 0.0f)
        {
            av.voice->SetFrequencyRatio(dspSettings.DopplerFactor);
        }
    }
}

void SoundPlayer::Stop3D(int voiceId)
{
    if (voiceId < 0 || voiceId >= static_cast<int>(m_3dVoices.size()))
        return;

    auto& info = m_3dVoices[voiceId];
    if (info.activeVoiceIndex >= 0 &&
        info.activeVoiceIndex < static_cast<int>(m_activeVoices.size()))
    {
        auto& av = m_activeVoices[info.activeVoiceIndex];
        if (av.voice && av.voiceId == voiceId)
        {
            av.voice->Stop(0);
            // isFinishedフラグを立てて、次のCleanupで回収されるようにする
            if (av.callback) av.callback->isFinished = true;
        }
    }

    info.emitter = nullptr;
    info.activeVoiceIndex = -1;
}

void SoundPlayer::StopAll()
{
    for (auto& av : m_activeVoices)
    {
        if (av.voice)
        {
            av.voice->Stop(0);
            av.voice->DestroyVoice();
            av.voice = nullptr;
        }
    }
    m_activeVoices.clear();
    m_3dVoices.clear();
    m_activeVoiceCount = 0;
}

void SoundPlayer::CleanupFinishedVoices()
{
    for (auto it = m_activeVoices.begin(); it != m_activeVoices.end(); )
    {
        if (it->callback && it->callback->isFinished)
        {
            // 3Dボイスの参照をクリア
            if (it->is3D && it->voiceId >= 0 &&
                it->voiceId < static_cast<int>(m_3dVoices.size()))
            {
                m_3dVoices[it->voiceId].emitter = nullptr;
                m_3dVoices[it->voiceId].activeVoiceIndex = -1;
            }

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

    // activeVoiceのインデックスが変わったので、3D情報のインデックスを再構築
    for (int i = 0; i < static_cast<int>(m_activeVoices.size()); ++i)
    {
        auto& av = m_activeVoices[i];
        if (av.is3D && av.voiceId >= 0 &&
            av.voiceId < static_cast<int>(m_3dVoices.size()))
        {
            m_3dVoices[av.voiceId].activeVoiceIndex = i;
        }
    }

    m_activeVoiceCount = static_cast<int>(m_activeVoices.size());
}

} // namespace GX
