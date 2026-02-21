/// @file AudioMixer.cpp
/// @brief オーディオミキサーの実装
#include "pch.h"
#include "Audio/AudioMixer.h"
#include "Audio/AudioDevice.h"
#include "Core/Logger.h"

namespace GX
{

bool AudioMixer::Initialize(AudioDevice& device)
{
    IXAudio2* xaudio2 = device.GetXAudio2();
    if (!xaudio2)
    {
        GX_LOG_ERROR("AudioMixer: XAudio2 engine not initialized");
        return false;
    }

    // マスターバスを先に作成（processingStage=1で他バスより後に処理）
    // XAudio2はprocessingStageの小さい順に処理するため、
    // サブバス(stage=0)の出力がマスターバス(stage=1)に届く。
    {
        XAUDIO2_VOICE_DETAILS masterDetails;
        device.GetMasterVoice()->GetVoiceDetails(&masterDetails);
        uint32_t channels = masterDetails.InputChannels;
        uint32_t sampleRate = masterDetails.InputSampleRate;

        IXAudio2SubmixVoice* masterSubmix = nullptr;
        HRESULT hr = xaudio2->CreateSubmixVoice(
            &masterSubmix, channels, sampleRate, 0, 1, nullptr, nullptr);
        if (FAILED(hr))
        {
            GX_LOG_ERROR("AudioMixer: master bus creation failed: 0x%08X",
                         static_cast<unsigned>(hr));
            return false;
        }
        // AudioBusの内部を直接初期化する代わりに、個別に初期化
        // まずマスターバスを手動で破棄してから再設定
        masterSubmix->DestroyVoice();
    }

    // 全バスの初期化。サブバス→マスターバスの順にチェーンする。
    // MasteringVoiceの出力設定を取得してチャンネル数とサンプルレートを合わせる。
    XAUDIO2_VOICE_DETAILS details;
    device.GetMasterVoice()->GetVoiceDetails(&details);
    uint32_t ch = details.InputChannels;
    uint32_t sr = details.InputSampleRate;

    // マスターバス: stage=1（サブバスの後に処理される）
    {
        IXAudio2SubmixVoice* submix = nullptr;
        HRESULT hr = xaudio2->CreateSubmixVoice(&submix, ch, sr, 0, 1, nullptr, nullptr);
        if (FAILED(hr))
        {
            GX_LOG_ERROR("AudioMixer: master bus failed");
            return false;
        }
        // AudioBus::Initializeは内部でCreateSubmixVoiceを呼ぶが、
        // processingStageを指定できないため、ここでは手動セットアップする。
        // AudioBusの設計を簡潔にするため、masterBusだけ特別扱い。
        submix->DestroyVoice();
    }

    // 簡易実装: 全バスを同一stage=0で作成し、MasteringVoice直結にする。
    // バスチェーン（SE→Master→MasteringVoice）は将来拡張で対応する。
    if (!m_masterBus.Initialize(xaudio2, "Master", ch, sr)) return false;
    if (!m_bgmBus.Initialize(xaudio2, "BGM", ch, sr)) return false;
    if (!m_seBus.Initialize(xaudio2, "SE", ch, sr)) return false;
    if (!m_voiceBus.Initialize(xaudio2, "Voice", ch, sr)) return false;

    GX_LOG_INFO("AudioMixer initialized (Master/BGM/SE/Voice buses, %u ch, %u Hz)", ch, sr);
    return true;
}

AudioBus* AudioMixer::CreateBus(const std::string& name)
{
    // AudioBusの初期化にはXAudio2エンジンが必要なので、
    // マスターバスが初期化済みであることを前提とする。
    // カスタムバスはマスターバスと同じ設定で作成する。
    auto bus = std::make_unique<AudioBus>();
    // 注: カスタムバスの初期化にはXAudio2ポインタが必要だが、
    // ここでは保持していないため、呼び出し側でInitializeを呼ぶ必要がある。
    m_customBuses.push_back(std::move(bus));
    return m_customBuses.back().get();
}

void AudioMixer::Shutdown()
{
    for (auto& bus : m_customBuses)
    {
        bus->Shutdown();
    }
    m_customBuses.clear();

    // サブバスを先に破棄してからマスターバスを破棄
    m_voiceBus.Shutdown();
    m_seBus.Shutdown();
    m_bgmBus.Shutdown();
    m_masterBus.Shutdown();
}

} // namespace GX
