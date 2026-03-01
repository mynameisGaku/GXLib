#pragma once
/// @file AudioDSP.h
/// @brief オーディオDSPエフェクト（バイクアッドフィルタ、リバーブ、ディレイ、コンプレッサー）
///
/// PCMバッファに直接適用するソフトウェアDSPエフェクトを提供する。
/// AudioEffectChain（XAudio2ネイティブAPO）とは別に、
/// CPU側で柔軟なエフェクト処理を行いたい場合に使用する。
/// - バイクアッドフィルタ（LPF/HPF）: 周波数帯域の制御
/// - Schroederリバーブ: 残響シミュレーション
/// - ディレイ: エコー/やまびこ
/// - コンプレッサー: ダイナミクス制御
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"

namespace gx
{

/// @brief ローパスフィルタのパラメータ
struct LowPassParams
{
    float cutoffHz  = 5000.0f; ///< カットオフ周波数（Hz）
    float resonance = 0.707f;  ///< レゾナンス（Q値）。0.707でバターワース特性
};

/// @brief ハイパスフィルタのパラメータ
struct HighPassParams
{
    float cutoffHz  = 200.0f;  ///< カットオフ周波数（Hz）
    float resonance = 0.707f;  ///< レゾナンス（Q値）
};

/// @brief Schroederリバーブのパラメータ
/// @note AudioEffect.hのReverbParamsとは別の独立したDSP用パラメータ
struct DSPReverbParams
{
    float roomSize  = 0.5f;  ///< 部屋サイズ（0.0〜1.0）。コムフィルタのフィードバック量に影響
    float damping   = 0.5f;  ///< 高域減衰量（0.0〜1.0）。1.0で高域が素早く減衰
    float wetLevel  = 0.3f;  ///< エフェクト音の割合（0.0〜1.0）
    float dryLevel  = 0.7f;  ///< 原音の割合（0.0〜1.0）
    float decayTime = 1.5f;  ///< 残響時間（秒）。コムフィルタのフィードバック係数に反映
};

/// @brief ディレイエフェクトのパラメータ
struct DelayParams
{
    float delayMs  = 250.0f; ///< 遅延時間（ミリ秒）
    float feedback = 0.4f;   ///< フィードバック量（0.0〜1.0）。高いほど繰り返しが長い
    float wetLevel = 0.3f;   ///< エフェクト音の割合（0.0〜1.0）
};

/// @brief コンプレッサーのパラメータ
struct CompressorParams
{
    float thresholdDb = -20.0f; ///< 閾値（dB）。この音量を超えた部分を圧縮する
    float ratio       = 4.0f;   ///< 圧縮比（例: 4.0 = 4:1圧縮）
    float attackMs    = 10.0f;  ///< アタックタイム（ミリ秒）。圧縮開始までの時間
    float releaseMs   = 100.0f; ///< リリースタイム（ミリ秒）。圧縮解除までの時間
};

/// @brief PCMバッファに直接DSPエフェクトを適用するクラス
///
/// 各エフェクトは内部状態を保持し、連続するバッファに対して
/// ステートフルな処理を行う。異なるストリームに使い回す場合は
/// Reset()で状態をクリアすること。
class AudioDSP
{
public:
    AudioDSP();
    ~AudioDSP();

    /// @brief ローパスフィルタを適用する（バイクアッド Direct Form II Transposed）
    /// @param buffer 処理対象のインターリーブPCMバッファ（float、-1.0〜1.0）
    /// @param sampleCount サンプル数（1チャンネルあたり）
    /// @param channels チャンネル数（1=モノラル、2=ステレオ）
    /// @param sampleRate サンプルレート（Hz）
    /// @param params フィルタパラメータ
    void ApplyLowPass(float* buffer, uint32_t sampleCount, uint32_t channels,
                      uint32_t sampleRate, const LowPassParams& params);

    /// @brief ハイパスフィルタを適用する（バイクアッド Direct Form II Transposed）
    /// @param buffer 処理対象のインターリーブPCMバッファ
    /// @param sampleCount サンプル数（1チャンネルあたり）
    /// @param channels チャンネル数
    /// @param sampleRate サンプルレート（Hz）
    /// @param params フィルタパラメータ
    void ApplyHighPass(float* buffer, uint32_t sampleCount, uint32_t channels,
                       uint32_t sampleRate, const HighPassParams& params);

    /// @brief Schroederリバーブを適用する（4コムフィルタ + 2オールパスフィルタ）
    /// @param buffer 処理対象のインターリーブPCMバッファ
    /// @param sampleCount サンプル数（1チャンネルあたり）
    /// @param channels チャンネル数
    /// @param sampleRate サンプルレート（Hz）
    /// @param params リバーブパラメータ
    void ApplyReverb(float* buffer, uint32_t sampleCount, uint32_t channels,
                     uint32_t sampleRate, const DSPReverbParams& params);

    /// @brief ディレイエフェクトを適用する（円形バッファ方式）
    /// @param buffer 処理対象のインターリーブPCMバッファ
    /// @param sampleCount サンプル数（1チャンネルあたり）
    /// @param channels チャンネル数
    /// @param sampleRate サンプルレート（Hz）
    /// @param params ディレイパラメータ
    void ApplyDelay(float* buffer, uint32_t sampleCount, uint32_t channels,
                    uint32_t sampleRate, const DelayParams& params);

    /// @brief コンプレッサーを適用する（RMSエンベロープフォロワー方式）
    /// @param buffer 処理対象のインターリーブPCMバッファ
    /// @param sampleCount サンプル数（1チャンネルあたり）
    /// @param channels チャンネル数
    /// @param sampleRate サンプルレート（Hz）
    /// @param params コンプレッサーパラメータ
    void ApplyCompressor(float* buffer, uint32_t sampleCount, uint32_t channels,
                         uint32_t sampleRate, const CompressorParams& params);

    /// @brief 全エフェクトの内部状態をリセットする
    ///
    /// 異なるオーディオストリームに切り替える場合や、
    /// パラメータを大幅に変更した場合に呼び出す。
    void Reset();

private:
    /// @brief バイクアッドフィルタの内部状態（チャンネルごと）
    struct BiquadState
    {
        float x1 = 0, x2 = 0; ///< 入力履歴
        float y1 = 0, y2 = 0; ///< 出力履歴
    };
    BiquadState m_lowPassState[2];  ///< ローパス用（最大2チャンネル）
    BiquadState m_highPassState[2]; ///< ハイパス用（最大2チャンネル）

    // --- Schroederリバーブ内部構造 ---
    static constexpr int k_NumCombFilters    = 4;    ///< コムフィルタ数
    static constexpr int k_NumAllPassFilters = 2;    ///< オールパスフィルタ数
    static constexpr int k_MaxCombDelay      = 4096; ///< コムフィルタ最大遅延サンプル数
    static constexpr int k_MaxAllPassDelay   = 1024; ///< オールパスフィルタ最大遅延サンプル数

    /// @brief コムフィルタ（残響のベースとなるフィードバック遅延）
    struct CombFilter
    {
        float buffer[k_MaxCombDelay] = {};
        int writePos     = 0;
        int delaySamples = 0;
        float feedback   = 0;
        float damp       = 0;
        float lastOut    = 0;
    };

    /// @brief オールパスフィルタ（残響の拡散・密度向上用）
    struct AllPassFilter
    {
        float buffer[k_MaxAllPassDelay] = {};
        int writePos     = 0;
        int delaySamples = 0;
        float feedback   = 0.5f;
    };

    CombFilter    m_combFilters[k_NumCombFilters];
    AllPassFilter m_allPassFilters[k_NumAllPassFilters];
    bool          m_reverbInitialized = false;

    // --- ディレイ内部状態 ---
    std::vector<float> m_delayBuffer;
    int m_delayWritePos   = 0;
    int m_delayBufferSize = 0;

    // --- コンプレッサー内部状態 ---
    float m_compEnvelope = 0.0f; ///< エンベロープフォロワーの現在値

    /// @brief リバーブ用バッファをサンプルレートに合わせて初期化する
    /// @param sampleRate サンプルレート（Hz）
    void InitReverbBuffers(uint32_t sampleRate);

    /// @brief バイクアッドフィルタをバッファに適用する（共通実装）
    /// @param buffer インターリーブPCMバッファ
    /// @param sampleCount サンプル数（1チャンネルあたり）
    /// @param channels チャンネル数
    /// @param state フィルタ状態配列（チャンネル数分）
    /// @param b0, b1, b2 フィルタの分子係数（a0で正規化済み）
    /// @param a1, a2 フィルタの分母係数（a0で正規化済み）
    void ApplyBiquad(float* buffer, uint32_t sampleCount, uint32_t channels,
                     BiquadState* state, float b0, float b1, float b2,
                     float a1, float a2);
};

} // namespace gx
/// @}
