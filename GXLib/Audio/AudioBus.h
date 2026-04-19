#pragma once
/// @file AudioBus.h
/// @brief オーディオバス（SubmixVoiceベースのミキシングチャンネル）
///
/// AudioBusはXAudio2のIXAudio2SubmixVoiceをラップし、
/// 複数のSourceVoiceやサブバスをグループ化してボリューム制御する。
/// 典型的な構成: SE/BGM/Voice → 各バス → マスターバス → MasteringVoice
/// DxLibには直接相当するAPIはないが、カテゴリ別の音量制御を実現する。

#include "pch_audio.h"
#include "Audio/AudioEffect.h"   // IAudioEffect 完全定義 (ADR-0017 L2)
#include <memory>

namespace gx { class XAPOBridge; }

namespace gx
{
/// @addtogroup grp_audio
/// @{

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
    bool Initialize(IXAudio2* xaudio2, const gx::String& name,
                    uint32_t channels = 2, uint32_t sampleRate = 44100);

    /// @brief バスの音量を設定する（0.0〜1.0）
    /// @param volume 音量
    void SetVolume(float volume);

    /// @brief 現在の音量を取得する
    /// @return 音量（0.0〜1.0）
    float GetVolume() const { return m_volume; }

    /// @brief ミュート状態を設定する
    /// @param muted ミュートするかどうか
    void SetMuted(bool muted) { m_muted = muted; }

    /// @brief ミュート状態を取得する
    /// @return ミュート中ならtrue
    bool IsMuted() const { return m_muted; }

    /// @brief 実効ボリューム（ミュート時は0）を取得する
    /// @return 実効ボリューム
    float GetEffectiveVolume() const { return m_muted ? 0.0f : m_volume; }

    /// @brief このバスの出力先を別のバスに変更する
    /// @param parent 出力先のバス（nullptrでMasteringVoice直結に戻る）
    void SetOutputBus(AudioBus* parent);

    /// @brief 内部のSubmixVoiceを取得する（SourceVoiceの出力先設定用）
    /// @return SubmixVoiceポインタ
    IXAudio2SubmixVoice* GetSubmixVoice() { return m_submixVoice; }

    /// @brief バス名を取得する
    /// @return バス名への参照
    const gx::String& GetName() const { return m_name; }

    /// @brief SubmixVoiceを破棄する
    void Shutdown();

    // =========================================================================
    // Layer 2 拡張: カスタム DSP エフェクトチェーン (ADR-0017 L2, ADR-0010 §8)
    // =========================================================================

    /// @brief このバスの insert チェーンにカスタムエフェクトを追加する
    /// @param effect ownership 譲渡。IAudioEffect 派生クラスのインスタンス
    /// @return エフェクトインデックス (0-3)、チェーン満杯 (4本) または null で -1
    ///
    /// @note ADR-0010 §8 により bus あたり最大 4 本。
    /// @note 実際のDSP適用は現状 XAPO ラッパー実装に依存 (将来ADRで拡張)。
    ///       現状は登録のみ機能し、Process() はまだ呼ばれない。
    ///       Process() を実動作させるには IXAPO ラップが必要。
    ///       詳細: docs/implementation-gap-analysis-2026-04-16.md T3 参照
    int AddEffect(std::unique_ptr<IAudioEffect> effect);

    /// @brief チェーンからエフェクトを削除する (ownership 破棄)
    /// @param index AddEffect が返したインデックス
    void RemoveEffect(int index);

    /// @brief 登録済みエフェクト数を返す (0〜4)
    size_t GetEffectCount() const;

    /// @brief 指定インデックスのエフェクトを取得 (読み取り専用)
    IAudioEffect* GetEffect(int index);

    /// @brief 最後の SetEffectChain の HRESULT (診断用)。S_OK = 0。
    long GetLastEffectChainStatus() const { return m_lastEffectChainHR; }

    /// @brief 最後に SetEffectChain で試みた OutputChannels (診断用)。
    unsigned int GetLastEffectChainOutputChannels() const { return m_lastEffectChainOutputChannels; }

private:
    static constexpr size_t k_MaxEffectsPerBus = 4;  ///< ADR-0010 §8 による

    IXAudio2SubmixVoice* m_submixVoice = nullptr;
    gx::String m_name;
    float m_volume = 1.0f;
    bool m_muted = false;
    long m_lastEffectChainHR = 0;                  ///< 最後の SetEffectChain HRESULT (診断)
    unsigned int m_lastEffectChainOutputChannels = 0; ///< 最後に試みた OutputChannels (診断)

    // Layer 2 DSP エフェクトチェーン (ADR-0017 L2)
    std::unique_ptr<IAudioEffect> m_effects[k_MaxEffectsPerBus];

    XAPOBridge* m_xapoBridge = nullptr;
    void RebuildEffectChain();
};

/// @}
} // namespace gx
