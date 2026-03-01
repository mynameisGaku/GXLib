/// @file AudioDSP.cpp
/// @brief オーディオDSPエフェクトの実装
#include "pch_audio.h"
#include "Audio/AudioDSP.h"
#include <cmath>

namespace gx
{

// ============================================================
// 定数
// ============================================================
static constexpr float k_Pi = 3.14159265358979323846f;

// Schroederリバーブの基準遅延サンプル数（44100Hz基準）
static constexpr int k_CombDelays[4]  = { 1557, 1617, 1491, 1422 };
static constexpr int k_AllPassDelays[2] = { 225, 556 };

// ============================================================
// コンストラクタ / デストラクタ
// ============================================================
AudioDSP::AudioDSP()  = default;
AudioDSP::~AudioDSP() = default;

// ============================================================
// Reset
// ============================================================
void AudioDSP::Reset()
{
    // バイクアッドフィルタ状態クリア
    for (auto& s : m_lowPassState)  s = {};
    for (auto& s : m_highPassState) s = {};

    // リバーブ状態クリア
    for (auto& c : m_combFilters)
    {
        std::memset(c.buffer, 0, sizeof(c.buffer));
        c.writePos = 0;
        c.lastOut  = 0;
    }
    for (auto& a : m_allPassFilters)
    {
        std::memset(a.buffer, 0, sizeof(a.buffer));
        a.writePos = 0;
    }
    m_reverbInitialized = false;

    // ディレイ状態クリア
    if (!m_delayBuffer.empty())
        std::fill(m_delayBuffer.begin(), m_delayBuffer.end(), 0.0f);
    m_delayWritePos   = 0;
    m_delayBufferSize = 0;

    // コンプレッサー状態クリア
    m_compEnvelope = 0.0f;
}

// ============================================================
// バイクアッドフィルタ共通実装
// ============================================================
void AudioDSP::ApplyBiquad(float* buffer, uint32_t sampleCount, uint32_t channels,
                           BiquadState* state, float b0, float b1, float b2,
                           float a1, float a2)
{
    if (!buffer || sampleCount == 0 || channels == 0) return;

    const uint32_t ch = (channels > 2) ? 2 : channels;

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        for (uint32_t c = 0; c < ch; ++c)
        {
            const uint32_t idx = i * channels + c;
            const float x = buffer[idx];

            // Direct Form II Transposed
            const float y = b0 * x + b1 * state[c].x1 + b2 * state[c].x2
                          - a1 * state[c].y1 - a2 * state[c].y2;

            state[c].x2 = state[c].x1;
            state[c].x1 = x;
            state[c].y2 = state[c].y1;
            state[c].y1 = y;

            buffer[idx] = y;
        }
    }
}

// ============================================================
// ローパスフィルタ
// ============================================================
void AudioDSP::ApplyLowPass(float* buffer, uint32_t sampleCount, uint32_t channels,
                            uint32_t sampleRate, const LowPassParams& params)
{
    if (!buffer || sampleCount == 0 || sampleRate == 0) return;

    const float omega = 2.0f * k_Pi * params.cutoffHz / static_cast<float>(sampleRate);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * params.resonance);

    const float a0 = 1.0f + alpha;
    const float a0Inv = 1.0f / a0;

    const float b0 = ((1.0f - cosOmega) / 2.0f) * a0Inv;
    const float b1 = (1.0f - cosOmega)           * a0Inv;
    const float b2 = ((1.0f - cosOmega) / 2.0f) * a0Inv;
    const float a1 = (-2.0f * cosOmega)          * a0Inv;
    const float a2 = (1.0f - alpha)               * a0Inv;

    ApplyBiquad(buffer, sampleCount, channels, m_lowPassState, b0, b1, b2, a1, a2);
}

// ============================================================
// ハイパスフィルタ
// ============================================================
void AudioDSP::ApplyHighPass(float* buffer, uint32_t sampleCount, uint32_t channels,
                             uint32_t sampleRate, const HighPassParams& params)
{
    if (!buffer || sampleCount == 0 || sampleRate == 0) return;

    const float omega = 2.0f * k_Pi * params.cutoffHz / static_cast<float>(sampleRate);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * params.resonance);

    const float a0 = 1.0f + alpha;
    const float a0Inv = 1.0f / a0;

    const float b0 = ((1.0f + cosOmega) / 2.0f)  * a0Inv;
    const float b1 = (-(1.0f + cosOmega))         * a0Inv;
    const float b2 = ((1.0f + cosOmega) / 2.0f)  * a0Inv;
    const float a1 = (-2.0f * cosOmega)           * a0Inv;
    const float a2 = (1.0f - alpha)                * a0Inv;

    ApplyBiquad(buffer, sampleCount, channels, m_highPassState, b0, b1, b2, a1, a2);
}

// ============================================================
// リバーブ - バッファ初期化
// ============================================================
void AudioDSP::InitReverbBuffers(uint32_t sampleRate)
{
    const float rateScale = static_cast<float>(sampleRate) / 44100.0f;

    for (int i = 0; i < k_NumCombFilters; ++i)
    {
        int delay = static_cast<int>(k_CombDelays[i] * rateScale);
        if (delay >= k_MaxCombDelay) delay = k_MaxCombDelay - 1;
        m_combFilters[i].delaySamples = delay;
        m_combFilters[i].writePos     = 0;
        m_combFilters[i].lastOut      = 0;
        std::memset(m_combFilters[i].buffer, 0, sizeof(m_combFilters[i].buffer));
    }

    for (int i = 0; i < k_NumAllPassFilters; ++i)
    {
        int delay = static_cast<int>(k_AllPassDelays[i] * rateScale);
        if (delay >= k_MaxAllPassDelay) delay = k_MaxAllPassDelay - 1;
        m_allPassFilters[i].delaySamples = delay;
        m_allPassFilters[i].writePos     = 0;
        std::memset(m_allPassFilters[i].buffer, 0, sizeof(m_allPassFilters[i].buffer));
    }

    m_reverbInitialized = true;
}

// ============================================================
// リバーブ - Schroederリバーブ適用
// ============================================================
void AudioDSP::ApplyReverb(float* buffer, uint32_t sampleCount, uint32_t channels,
                           uint32_t sampleRate, const DSPReverbParams& params)
{
    if (!buffer || sampleCount == 0 || sampleRate == 0) return;

    if (!m_reverbInitialized)
        InitReverbBuffers(sampleRate);

    // パラメータからコムフィルタの係数を設定
    // roomSize はフィードバック量に影響（0.0〜1.0 → 0.7〜0.99程度にマッピング）
    const float fbBase = 0.7f + 0.28f * params.roomSize;
    for (int i = 0; i < k_NumCombFilters; ++i)
    {
        m_combFilters[i].feedback = fbBase;
        m_combFilters[i].damp     = params.damping;
    }
    for (int i = 0; i < k_NumAllPassFilters; ++i)
    {
        m_allPassFilters[i].feedback = 0.5f;
    }

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        // モノラル入力を作成（全チャンネルの平均）
        float monoIn = 0.0f;
        for (uint32_t c = 0; c < channels; ++c)
            monoIn += buffer[i * channels + c];
        monoIn /= static_cast<float>(channels);

        // ---- 4つのコムフィルタを並列処理し合算 ----
        float combSum = 0.0f;
        for (int ci = 0; ci < k_NumCombFilters; ++ci)
        {
            auto& comb = m_combFilters[ci];

            // 読み出し位置
            int readPos = comb.writePos - comb.delaySamples;
            if (readPos < 0) readPos += k_MaxCombDelay;

            const float bufOut = comb.buffer[readPos];

            // ダンピング付きフィードバック: ローパス的に高域を減衰
            const float filtered = bufOut * (1.0f - comb.damp) + comb.lastOut * comb.damp;
            comb.lastOut = filtered;

            // バッファへ書き込み
            comb.buffer[comb.writePos] = monoIn + filtered * comb.feedback;
            comb.writePos = (comb.writePos + 1) % k_MaxCombDelay;

            combSum += bufOut;
        }

        // ---- 2つのオールパスフィルタを直列処理 ----
        float apOut = combSum;
        for (int ai = 0; ai < k_NumAllPassFilters; ++ai)
        {
            auto& ap = m_allPassFilters[ai];

            int readPos = ap.writePos - ap.delaySamples;
            if (readPos < 0) readPos += k_MaxAllPassDelay;

            const float bufOut = ap.buffer[readPos];

            ap.buffer[ap.writePos] = apOut + bufOut * ap.feedback;
            ap.writePos = (ap.writePos + 1) % k_MaxAllPassDelay;

            apOut = bufOut - apOut * ap.feedback;
        }

        // ---- Dry/Wet ミックスして全チャンネルに書き戻し ----
        for (uint32_t c = 0; c < channels; ++c)
        {
            const uint32_t idx = i * channels + c;
            buffer[idx] = buffer[idx] * params.dryLevel + apOut * params.wetLevel;
        }
    }
}

// ============================================================
// ディレイ
// ============================================================
void AudioDSP::ApplyDelay(float* buffer, uint32_t sampleCount, uint32_t channels,
                          uint32_t sampleRate, const DelayParams& params)
{
    if (!buffer || sampleCount == 0 || sampleRate == 0) return;

    // 遅延サンプル数を計算（全チャンネル分のインターリーブ単位）
    const int delaySamples = static_cast<int>(params.delayMs * static_cast<float>(sampleRate) / 1000.0f)
                             * static_cast<int>(channels);

    // バッファサイズが不足していれば再確保
    if (delaySamples > m_delayBufferSize || m_delayBuffer.empty())
    {
        m_delayBufferSize = delaySamples;
        m_delayBuffer.resize(m_delayBufferSize, 0.0f);
        m_delayWritePos = 0;
    }

    const int totalSamples = static_cast<int>(sampleCount * channels);
    const float wet = params.wetLevel;
    const float dry = 1.0f - wet;

    for (int i = 0; i < totalSamples; ++i)
    {
        // ディレイバッファから読み出し
        int readPos = m_delayWritePos - delaySamples;
        if (readPos < 0) readPos += m_delayBufferSize;

        const float delayedSample = m_delayBuffer[readPos];

        // フィードバック付きで書き込み
        m_delayBuffer[m_delayWritePos] = buffer[i] + delayedSample * params.feedback;
        m_delayWritePos = (m_delayWritePos + 1) % m_delayBufferSize;

        // Dry/Wet ミックス
        buffer[i] = buffer[i] * dry + delayedSample * wet;
    }
}

// ============================================================
// コンプレッサー
// ============================================================
void AudioDSP::ApplyCompressor(float* buffer, uint32_t sampleCount, uint32_t channels,
                               uint32_t sampleRate, const CompressorParams& params)
{
    if (!buffer || sampleCount == 0 || sampleRate == 0) return;

    // アタック/リリース係数を計算（指数平滑化）
    const float attackCoeff  = std::exp(-1.0f / (params.attackMs  * 0.001f * static_cast<float>(sampleRate)));
    const float releaseCoeff = std::exp(-1.0f / (params.releaseMs * 0.001f * static_cast<float>(sampleRate)));

    // 閾値をリニアスケールに変換
    const float thresholdLinear = std::pow(10.0f, params.thresholdDb / 20.0f);

    const float ratio = params.ratio;

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        // 全チャンネルの最大絶対値を取得
        float peak = 0.0f;
        for (uint32_t c = 0; c < channels; ++c)
        {
            const float absVal = std::fabs(buffer[i * channels + c]);
            if (absVal > peak) peak = absVal;
        }

        // エンベロープフォロワー（ピーク検出）
        if (peak > m_compEnvelope)
            m_compEnvelope = attackCoeff * m_compEnvelope + (1.0f - attackCoeff) * peak;
        else
            m_compEnvelope = releaseCoeff * m_compEnvelope + (1.0f - releaseCoeff) * peak;

        // ゲイン計算
        float gain = 1.0f;
        if (m_compEnvelope > thresholdLinear && m_compEnvelope > 0.0f)
        {
            // 閾値を超えた分を圧縮比で抑制
            // 目標レベル = threshold + (envelope - threshold) / ratio
            const float targetLevel = thresholdLinear + (m_compEnvelope - thresholdLinear) / ratio;
            gain = targetLevel / m_compEnvelope;
        }

        // 全チャンネルに同一ゲインを適用
        for (uint32_t c = 0; c < channels; ++c)
        {
            buffer[i * channels + c] *= gain;
        }
    }
}

} // namespace gx
