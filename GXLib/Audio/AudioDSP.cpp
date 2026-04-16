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

// ============================================================
// ConvolutionReverb — Initialize
// ============================================================
void ConvolutionReverb::Initialize(const float* impulseResponse, int irLength, int sampleRate)
{
    if (!impulseResponse || irLength <= 0 || sampleRate <= 0)
        return;

    m_sampleRate = sampleRate;
    m_irLength = irLength;

    // IRをコピー（正規化はしない — 呼び出し元が適切なレベルのIRを渡す）
    m_ir.resize(irLength);
    std::memcpy(m_ir.data(), impulseResponse, irLength * sizeof(float));

    // 入力サンプル履歴バッファ（IR長分の過去サンプルを保持）
    m_inputHistory.resize(irLength, 0.0f);

    // プリディレイバッファの初期化
    m_preDelaySamples = static_cast<int>(m_preDelayMs * 0.001f * static_cast<float>(sampleRate));
    if (m_preDelaySamples > 0)
    {
        m_preDelayBuffer.resize(m_preDelaySamples, 0.0f);
        m_preDelayWritePos = 0;
    }
}

// ============================================================
// ConvolutionReverb — Process
// ============================================================
void ConvolutionReverb::Process(float* buffer, int numSamples)
{
    if (!buffer || numSamples <= 0 || m_ir.empty())
        return;

    // モノラル処理 — ステレオの場合はL/Rの平均を取る形ではなく
    // 各チャンネルに同じ処理を適用するためそのまま ProcessDirect を呼ぶ
    ProcessDirect(buffer, numSamples);
}

// ============================================================
// ConvolutionReverb — ProcessDirect (時間領域直接畳み込み)
// ============================================================
void ConvolutionReverb::ProcessDirect(float* buffer, int numSamples)
{
    // プリディレイの更新
    int preDelaySamplesNeeded = static_cast<int>(m_preDelayMs * 0.001f * static_cast<float>(m_sampleRate));
    if (preDelaySamplesNeeded != m_preDelaySamples && preDelaySamplesNeeded > 0)
    {
        m_preDelaySamples = preDelaySamplesNeeded;
        m_preDelayBuffer.resize(m_preDelaySamples, 0.0f);
        m_preDelayWritePos = m_preDelayWritePos % m_preDelaySamples;
    }

    const float wet = m_wet;
    const float dry = 1.0f - wet;
    const int irLen = m_irLength;
    const float* ir = m_ir.data();

    for (int n = 0; n < numSamples; ++n)
    {
        const float inputSample = buffer[n];

        // 入力サンプルを履歴の先頭に格納（循環バッファとしてシフト）
        // 新しいサンプルを末尾に追加し、古いサンプルを先頭からシフトする方式ではなく
        // 効率のため reverse index を使用
        // inputHistory[0] = 最新、inputHistory[irLen-1] = 最古
        std::memmove(m_inputHistory.data() + 1, m_inputHistory.data(),
                     (irLen - 1) * sizeof(float));
        m_inputHistory[0] = inputSample;

        // 畳み込み: y[n] = sum_{k=0}^{irLen-1} h[k] * x[n-k]
        float convOut = 0.0f;
        for (int k = 0; k < irLen; ++k)
        {
            convOut += ir[k] * m_inputHistory[k];
        }

        // プリディレイ適用
        float delayedOut = convOut;
        if (m_preDelaySamples > 0 && !m_preDelayBuffer.empty())
        {
            // 読み出し位置
            int readPos = m_preDelayWritePos - m_preDelaySamples;
            if (readPos < 0) readPos += m_preDelaySamples;

            delayedOut = m_preDelayBuffer[readPos];
            m_preDelayBuffer[m_preDelayWritePos] = convOut;
            m_preDelayWritePos = (m_preDelayWritePos + 1) % m_preDelaySamples;
        }

        // Dry/Wet ミックス
        buffer[n] = inputSample * dry + delayedOut * wet;
    }
}

// ============================================================
// ConvolutionReverb — Reset
// ============================================================
void ConvolutionReverb::Reset()
{
    if (!m_inputHistory.empty())
        std::fill(m_inputHistory.begin(), m_inputHistory.end(), 0.0f);
    if (!m_preDelayBuffer.empty())
        std::fill(m_preDelayBuffer.begin(), m_preDelayBuffer.end(), 0.0f);
    m_preDelayWritePos = 0;
}

// ============================================================
// ConvolutionReverb — GenerateRoomIR (静的ファクトリ)
// ============================================================
gx::Vector<float> ConvolutionReverb::GenerateRoomIR(float roomSizeMeters,
                                                      float dampingFactor,
                                                      int sampleRate)
{
    if (roomSizeMeters <= 0.0f || sampleRate <= 0)
        return {};

    // 減衰係数をクランプ
    dampingFactor = std::clamp(dampingFactor, 0.0f, 1.0f);

    // 残響時間の推定: RT60 ≈ 0.161 * V / A (Sabine式の簡易版)
    // ここでは roomSize から残響長さを線形にマッピング
    // 小部屋(2m) → 0.2秒、大ホール(50m) → 2.5秒
    float rt60Seconds = 0.1f + roomSizeMeters * 0.05f;
    rt60Seconds *= (1.0f - dampingFactor * 0.8f);  // ダンピングで短縮

    int irLengthSamples = static_cast<int>(rt60Seconds * static_cast<float>(sampleRate));
    // 最大4096サンプル（直接畳み込みの性能制限）
    irLengthSamples = (std::min)(irLengthSamples, 4096);
    irLengthSamples = (std::max)(irLengthSamples, 64);

    gx::Vector<float> ir(irLengthSamples, 0.0f);

    // ---- 直接音（最初のサンプル）----
    ir[0] = 1.0f;

    // ---- 初期反射 (Early Reflections) ----
    // 部屋のサイズに基づく反射遅延時間
    // 音速 ≈ 343 m/s、壁面反射の往復距離 = 2 * roomSize
    float speedOfSound = 343.0f;
    float reflectionDelay = 2.0f * roomSizeMeters / speedOfSound; // 秒
    int reflectionDelaySamples = static_cast<int>(reflectionDelay * static_cast<float>(sampleRate));

    // 6面の壁からの初期反射（左右上下前後）
    float reflectionCoeffs[] = { 0.6f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f };
    float reflectionDelayMultipliers[] = { 1.0f, 1.2f, 1.5f, 1.8f, 2.1f, 2.5f };

    for (int r = 0; r < 6; ++r)
    {
        int delaySamp = static_cast<int>(reflectionDelaySamples * reflectionDelayMultipliers[r]);
        if (delaySamp < irLengthSamples && delaySamp > 0)
        {
            // ダンピングの影響で反射係数を減衰
            float coeff = reflectionCoeffs[r] * (1.0f - dampingFactor * 0.5f);
            ir[delaySamp] += coeff;
        }
    }

    // ---- 拡散残響 (Diffuse Tail) ----
    // 指数減衰するノイズで残響テイルを構築
    // RT60 = -60dB到達時間 → 減衰率 = exp(-6.908 / (RT60 * sampleRate))
    float decayRate = 0.0f;
    if (rt60Seconds > 0.0f)
        decayRate = std::exp(-6.908f / (rt60Seconds * static_cast<float>(sampleRate)));

    // 残響テイルの開始位置（初期反射の後）
    int tailStart = (std::max)(1, static_cast<int>(reflectionDelaySamples * 3.0f));
    if (tailStart >= irLengthSamples)
        tailStart = irLengthSamples / 4;

    // 疑似乱数生成（決定的にするため固定シード）
    uint32_t seed = 0x12345678u;
    auto nextRandom = [&seed]() -> float {
        seed = seed * 1664525u + 1013904223u;
        return (static_cast<float>(seed) / static_cast<float>(0xFFFFFFFFu)) * 2.0f - 1.0f;
    };

    for (int i = tailStart; i < irLengthSamples; ++i)
    {
        // 指数減衰エンベロープ
        float envelope = std::pow(decayRate, static_cast<float>(i));

        // ノイズ密度を制御（低密度で開始、徐々に密に）
        float density = static_cast<float>(i - tailStart) / static_cast<float>(irLengthSamples - tailStart);
        density = (std::min)(density * 2.0f, 1.0f);

        // ダンピングの影響（高域を減衰させる簡易版 — 隣接サンプル平均）
        float noise = nextRandom() * density;
        float dampedNoise = noise * (1.0f - dampingFactor * 0.6f);

        ir[i] += dampedNoise * envelope * 0.15f;
    }

    // IR全体を正規化（ピーク値を1.0に）
    float peak = 0.0f;
    for (int i = 0; i < irLengthSamples; ++i)
    {
        float absVal = std::fabs(ir[i]);
        if (absVal > peak) peak = absVal;
    }
    if (peak > 0.0f)
    {
        float invPeak = 1.0f / peak;
        for (int i = 0; i < irLengthSamples; ++i)
            ir[i] *= invPeak;
    }

    return ir;
}

} // namespace gx
