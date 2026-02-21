#pragma once
/// @file AudioMixer.h
/// @brief オーディオミキサー（バスグループ管理）
///
/// AudioMixerは定義済みバス（Master/BGM/SE/Voice）とカスタムバスを管理する。
/// 各バスはIXAudio2SubmixVoiceを持ち、カテゴリ別の音量制御ができる。
/// 構成: SourceVoice → SE/BGM/Voiceバス → Masterバス → MasteringVoice
///
/// DxLibには直接相当するAPIはないが、ゲーム開発では
/// 「設定画面でBGMとSEの音量を別々に調整する」機能に使う。

#include "pch.h"
#include "Audio/AudioBus.h"

namespace GX
{

class AudioDevice;

/// @brief オーディオミキサー（BGM/SE/Voiceバスの統合管理）
class AudioMixer
{
public:
    AudioMixer() = default;
    ~AudioMixer() = default;

    /// @brief ミキサーを初期化する。定義済みバスを全て作成する
    /// @param device AudioDeviceへの参照
    /// @return 成功でtrue
    bool Initialize(AudioDevice& device);

    /// @brief マスターバスを取得する（全バスの最終出力先）
    /// @return マスターバスへの参照
    AudioBus& GetMasterBus() { return m_masterBus; }

    /// @brief BGMバスを取得する
    /// @return BGMバスへの参照
    AudioBus& GetBGMBus() { return m_bgmBus; }

    /// @brief SEバスを取得する
    /// @return SEバスへの参照
    AudioBus& GetSEBus() { return m_seBus; }

    /// @brief Voiceバスを取得する（キャラクターボイス等）
    /// @return Voiceバスへの参照
    AudioBus& GetVoiceBus() { return m_voiceBus; }

    /// @brief カスタムバスを作成する
    /// @param name バス名
    /// @return 作成されたバスのポインタ。失敗時nullptr
    AudioBus* CreateBus(const std::string& name);

    /// @brief 全バスを破棄する
    void Shutdown();

private:
    AudioBus m_masterBus;   ///< 全バスの最終出力（processingStage=1）
    AudioBus m_bgmBus;      ///< BGM用バス
    AudioBus m_seBus;       ///< SE用バス
    AudioBus m_voiceBus;    ///< ボイス用バス
    std::vector<std::unique_ptr<AudioBus>> m_customBuses;
};

} // namespace GX
