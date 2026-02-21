#pragma once
/// @file AudioBus.h
/// @brief オーディオバス（SubmixVoiceベースのミキシングチャンネル）
///
/// AudioBusはXAudio2のIXAudio2SubmixVoiceをラップし、
/// 複数のSourceVoiceやサブバスをグループ化してボリューム制御する。
/// 典型的な構成: SE/BGM/Voice → 各バス → マスターバス → MasteringVoice
/// DxLibには直接相当するAPIはないが、カテゴリ別の音量制御を実現する。

#include "pch.h"

namespace GX
{

/// @brief オーディオバス（SubmixVoiceラッパー）
///
/// SubmixVoiceはXAudio2のミキシングノード。
/// 複数のSourceVoice出力をまとめて1つのボリュームノブで制御できる。
class AudioBus
{
public:
    AudioBus() = default;
    ~AudioBus();

    /// @brief SubmixVoiceを作成してバスを初期化する
    /// @param xaudio2 XAudio2エンジン
    /// @param name バス名（"BGM", "SE"等、デバッグ用）
    /// @param channels 出力チャンネル数（デフォルト2=ステレオ）
    /// @param sampleRate サンプルレート（デフォルト44100Hz）
    /// @return 成功でtrue
    bool Initialize(IXAudio2* xaudio2, const std::string& name,
                    uint32_t channels = 2, uint32_t sampleRate = 44100);

    /// @brief バスの音量を設定する（0.0〜1.0）
    /// @param volume 音量
    void SetVolume(float volume);

    /// @brief 現在の音量を取得する
    /// @return 音量（0.0〜1.0）
    float GetVolume() const { return m_volume; }

    /// @brief このバスの出力先を別のバスに変更する
    /// @param parent 出力先のバス（nullptrでMasteringVoice直結に戻る）
    void SetOutputBus(AudioBus* parent);

    /// @brief 内部のSubmixVoiceを取得する（SourceVoiceの出力先設定用）
    /// @return SubmixVoiceポインタ
    IXAudio2SubmixVoice* GetSubmixVoice() { return m_submixVoice; }

    /// @brief バス名を取得する
    /// @return バス名への参照
    const std::string& GetName() const { return m_name; }

    /// @brief SubmixVoiceを破棄する
    void Shutdown();

private:
    IXAudio2SubmixVoice* m_submixVoice = nullptr;
    std::string m_name;
    float m_volume = 1.0f;
};

} // namespace GX
