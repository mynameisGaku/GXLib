#pragma once
/// @file AudioEffect.h
/// @brief オーディオDSPエフェクト（リバーブ + イコライザ）
///
/// XAudio2の組み込みエフェクト（FXReverb, FXEQ）を使い、
/// AudioBusにリバーブやEQを適用する。
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"

namespace gx
{

/// @brief リバーブプリセット
enum class ReverbPreset
{
    Default,    ///< デフォルト
    Room,       ///< 小部屋
    Hall,       ///< ホール
    Cave,       ///< 洞窟
    Arena,      ///< アリーナ
    Bathroom,   ///< 浴室
    Forest,     ///< 森林
    Underwater, ///< 水中
};

/// @brief リバーブパラメータ
struct ReverbParams
{
    float wetDryMix = 50.0f;   ///< ウェット/ドライ比（0-100）
    float roomSize  = 80.0f;   ///< 部屋サイズ（0-100、XAUDIO2FX_REVERB_MIN/MAX_DIFFUSION）
    float decay     = 1.5f;    ///< 減衰時間
    float density   = 100.0f;  ///< 密度（0-100）
    float diffusion = 100.0f;  ///< 拡散（0-100）

    /// @brief プリセットからパラメータを生成する
    static ReverbParams FromPreset(ReverbPreset preset);
};

// =========================================================================
// Layer 2 拡張ポイント — カスタムオーディオDSPエフェクト (ADR-0017 L2, ADR-0010 §8)
// =========================================================================

/// @brief ユーザー定義のオーディオDSPエフェクトインターフェース
///
/// Layer 2 ユーザーが独自の DSP エフェクト (歪み、トレモロ、カスタムフィルタ等) を
/// AudioBus の insert チェーンに追加するための基底クラス。
///
/// @section threading スレッドモデル (ADR-0010 §12)
/// - Process() は XAudio2 オーディオコールバックスレッドから呼ばれる
/// - メインスレッドのパラメータ変更は atomic double-buffer で伝播する (実装側の責務)
/// - **ヒープアロケーションは禁止** (forbidden_pattern: heap_allocation_on_audio_callback)
///   std::vector push_back (事前 reserve なし)、std::string 成長、new/delete は不可
/// - ミューテックス待ちや I/O も禁止
///
/// @section lifecycle ライフサイクル
/// 1. アプリが std::make_unique<MyEffect>() で生成
/// 2. AudioBus::AddEffect(std::move(effect)) で登録 (ownership 譲渡、≤4/bus per ADR-0010)
/// 3. 再生中、各 PCM バッファに対して Process() が呼ばれる
/// 4. ストリーム切替時に Reset() が呼ばれる
/// 5. AudioBus 破棄または RemoveEffect() で破棄
///
/// @code
/// class TremoloEffect : public gx::IAudioEffect {
/// public:
///     void Process(float* buffer, uint32_t sampleCount,
///                  uint32_t channels, uint32_t sampleRate) override {
///         for (uint32_t i = 0; i < sampleCount; ++i) {
///             float lfo = 0.5f + 0.5f * std::sinf(m_phase);
///             m_phase += 2.0f * 3.14159f * m_rateHz / sampleRate;
///             for (uint32_t c = 0; c < channels; ++c)
///                 buffer[i * channels + c] *= lfo;
///         }
///     }
///     void SetRate(float hz) { m_rateHz = hz; }   // main thread OK (float write is atomic on x86/x64)
///     const char* GetName() const override { return "Tremolo"; }
/// private:
///     std::atomic<float> m_rateHz = 5.0f;
///     float m_phase = 0.0f;
/// };
///
/// auto fx = std::make_unique<TremoloEffect>();
/// audioBus.AddEffect(std::move(fx));
/// @endcode
class IAudioEffect
{
public:
    virtual ~IAudioEffect() = default;

    /// @brief PCM サンプル処理 (オーディオコールバックスレッドで呼ばれる)
    /// @param buffer インターリーブPCMバッファ (float32, -1.0〜1.0, in-place)
    /// @param sampleCount 1チャンネルあたりのサンプル数
    /// @param channels チャンネル数 (1=モノ, 2=ステレオ)
    /// @param sampleRate サンプルレート (Hz)
    ///
    /// ⚠ ヒープアロケーション禁止。すべての作業バッファは事前確保すること。
    virtual void Process(float* buffer, uint32_t sampleCount,
                         uint32_t channels, uint32_t sampleRate) = 0;

    /// @brief 内部状態をリセットする (ストリーム切替時に呼ばれる)
    virtual void Reset() {}

    /// @brief エフェクト名 (デバッグ・ログ用、任意)
    virtual const char* GetName() const { return "CustomEffect"; }
};

/// @brief EQパラメータ（4バンド）
struct EQParams
{
    float lowGain     = 1.0f;  ///< 低域ゲイン（0.126〜7.94）
    float midLowGain  = 1.0f;  ///< 中低域ゲイン
    float midHighGain = 1.0f;  ///< 中高域ゲイン
    float highGain    = 1.0f;  ///< 高域ゲイン
};

/// @brief オーディオエフェクトチェーン
///
/// AudioBusの SubmixVoice にリバーブとEQのDSPエフェクトを適用する。
class AudioEffectChain
{
public:
    AudioEffectChain() = default;
    ~AudioEffectChain();

    /// @brief エフェクトチェーンを初期化する
    /// @param submixVoice 対象のSubmixVoice
    /// @param channels チャンネル数
    /// @param sampleRate サンプルレート
    /// @return 成功でtrue
    bool Initialize(IXAudio2SubmixVoice* submixVoice, uint32_t channels = 2, uint32_t sampleRate = 44100);

    // --- リバーブ ---
    /// @brief リバーブの有効/無効を設定する
    /// @param enabled trueで有効
    void SetReverbEnabled(bool enabled) { m_reverbEnabled = enabled; }
    /// @brief リバーブが有効か取得する
    /// @return 有効ならtrue
    bool IsReverbEnabled() const { return m_reverbEnabled; }
    /// @brief リバーブパラメータを設定する
    /// @param params リバーブパラメータ
    void SetReverbParams(const ReverbParams& params);
    /// @brief リバーブプリセットを適用する
    /// @param preset プリセット種別
    void SetReverbPreset(ReverbPreset preset);
    /// @brief 現在のリバーブパラメータを取得する
    /// @return パラメータへの参照
    const ReverbParams& GetReverbParams() const { return m_reverbParams; }

    // --- EQ ---
    /// @brief EQの有効/無効を設定する
    /// @param enabled trueで有効
    void SetEQEnabled(bool enabled) { m_eqEnabled = enabled; }
    /// @brief EQが有効か取得する
    /// @return 有効ならtrue
    bool IsEQEnabled() const { return m_eqEnabled; }
    /// @brief EQパラメータを設定する
    /// @param params 4バンドEQパラメータ
    void SetEQParams(const EQParams& params);
    /// @brief 現在のEQパラメータを取得する
    /// @return パラメータへの参照
    const EQParams& GetEQParams() const { return m_eqParams; }

    /// @brief 現在のパラメータをSubmixVoiceに適用する
    void Apply();

    /// @brief エフェクトチェーンを破棄する
    void Shutdown();

private:
    IXAudio2SubmixVoice* m_submixVoice = nullptr;  ///< 対象のサブミックスボイス
    bool m_initialized = false;                     ///< 初期化済みフラグ

    bool m_reverbEnabled = false;   ///< リバーブ有効フラグ
    ReverbParams m_reverbParams;    ///< 現在のリバーブパラメータ

    bool m_eqEnabled = false;       ///< EQ有効フラグ
    EQParams m_eqParams;            ///< 現在のEQパラメータ

    uint32_t m_channels   = 2;      ///< チャンネル数
    uint32_t m_sampleRate = 44100;  ///< サンプルレート（Hz）
};

} // namespace gx
/// @}
