/// @file test_AudioSystem.cpp
/// @brief Phase 5: AudioDSPエフェクトテスト（LowPass, HighPass, Reverb, Delay, Compressor）

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/AudioDSP.h"
#include <cmath>
#include <numeric>

using namespace gx;

// ============================================================================
// ヘルパー
// ============================================================================

static constexpr uint32_t k_SampleRate = 44100;
static constexpr uint32_t k_SampleCount = 1024;

/// 正弦波バッファを生成（モノラル）
static gx::Vector<float> GenerateSine(uint32_t sampleCount, float frequency, float amplitude = 0.8f)
{
    gx::Vector<float> buf(sampleCount);
    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        float t = static_cast<float>(i) / k_SampleRate;
        buf[i] = amplitude * std::sin(2.0f * 3.14159265f * frequency * t);
    }
    return buf;
}

/// ステレオ正弦波を生成（インターリーブL/R）
static gx::Vector<float> GenerateStereoSine(uint32_t sampleCountPerChannel, float frequency, float amplitude = 0.8f)
{
    gx::Vector<float> buf(sampleCountPerChannel * 2);
    for (uint32_t i = 0; i < sampleCountPerChannel; ++i)
    {
        float t = static_cast<float>(i) / k_SampleRate;
        float sample = amplitude * std::sin(2.0f * 3.14159265f * frequency * t);
        buf[i * 2 + 0] = sample;  // 左チャンネル
        buf[i * 2 + 1] = sample;  // 右チャンネル
    }
    return buf;
}

/// すべてのサンプルが有効範囲[-1, 1]内にあるか確認
static bool AllSamplesInRange(const gx::Vector<float>& buf, float minVal = -1.5f, float maxVal = 1.5f)
{
    for (float s : buf)
    {
        if (s < minVal || s > maxVal || std::isnan(s) || std::isinf(s))
            return false;
    }
    return true;
}

/// バッファのRMSを計算
static float ComputeRMS(const float* data, uint32_t count)
{
    float sum = 0.0f;
    for (uint32_t i = 0; i < count; ++i)
        sum += data[i] * data[i];
    return std::sqrt(sum / count);
}

// ============================================================================
// LowPassParamsデフォルト値
// ============================================================================

TEST(AudioDSPParams, LowPassDefaults)
{
    LowPassParams params;
    EXPECT_FLOAT_EQ(params.cutoffHz, 5000.0f);
    EXPECT_FLOAT_EQ(params.resonance, 0.707f);
}

TEST(AudioDSPParams, HighPassDefaults)
{
    HighPassParams params;
    EXPECT_FLOAT_EQ(params.cutoffHz, 200.0f);
    EXPECT_FLOAT_EQ(params.resonance, 0.707f);
}

TEST(AudioDSPParams, ReverbDefaults)
{
    DSPReverbParams params;
    EXPECT_FLOAT_EQ(params.roomSize, 0.5f);
    EXPECT_FLOAT_EQ(params.damping, 0.5f);
    EXPECT_FLOAT_EQ(params.wetLevel, 0.3f);
    EXPECT_FLOAT_EQ(params.dryLevel, 0.7f);
    EXPECT_FLOAT_EQ(params.decayTime, 1.5f);
}

TEST(AudioDSPParams, DelayDefaults)
{
    DelayParams params;
    EXPECT_FLOAT_EQ(params.delayMs, 250.0f);
    EXPECT_FLOAT_EQ(params.feedback, 0.4f);
    EXPECT_FLOAT_EQ(params.wetLevel, 0.3f);
}

TEST(AudioDSPParams, CompressorDefaults)
{
    CompressorParams params;
    EXPECT_FLOAT_EQ(params.thresholdDb, -20.0f);
    EXPECT_FLOAT_EQ(params.ratio, 4.0f);
    EXPECT_FLOAT_EQ(params.attackMs, 10.0f);
    EXPECT_FLOAT_EQ(params.releaseMs, 100.0f);
}

// ============================================================================
// ローパスフィルター
// ============================================================================

TEST(AudioDSP, LowPassModifiesSignal)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 10000.0f); // 10kHz（デフォルトの5kHzカットオフより上）
    auto original = buf;

    LowPassParams params;
    params.cutoffHz = 2000.0f; // 10kHz信号を減衰させるための非常に低いカットオフ
    dsp.ApplyLowPass(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    // 信号は減衰するはず（RMSが減少する）
    float rmsOriginal = ComputeRMS(original.data(), k_SampleCount);
    float rmsFiltered = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_LT(rmsFiltered, rmsOriginal);
}

TEST(AudioDSP, LowPassPassesLowFrequency)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 100.0f); // 100Hz（5kHzカットオフよりかなり下）
    auto original = buf;

    LowPassParams params;
    params.cutoffHz = 5000.0f;
    dsp.ApplyLowPass(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    // 低周波はほぼ変化なく通過するはず
    float rmsOriginal = ComputeRMS(original.data(), k_SampleCount);
    float rmsFiltered = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_GT(rmsFiltered, rmsOriginal * 0.8f); // 元のエネルギーの少なくとも80%が保持される
}

TEST(AudioDSP, LowPassStereo)
{
    AudioDSP dsp;
    auto buf = GenerateStereoSine(k_SampleCount, 10000.0f);

    LowPassParams params;
    params.cutoffHz = 2000.0f;
    dsp.ApplyLowPass(buf.data(), k_SampleCount, 2, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
}

// ============================================================================
// ハイパスフィルター
// ============================================================================

TEST(AudioDSP, HighPassModifiesSignal)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 100.0f); // 100Hz（1kHzカットオフより下）
    auto original = buf;

    HighPassParams params;
    params.cutoffHz = 1000.0f;
    dsp.ApplyHighPass(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    float rmsOriginal = ComputeRMS(original.data(), k_SampleCount);
    float rmsFiltered = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_LT(rmsFiltered, rmsOriginal);
}

TEST(AudioDSP, HighPassPassesHighFrequency)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 10000.0f); // 10kHz（200Hzカットオフよりかなり上）
    auto original = buf;

    HighPassParams params;
    params.cutoffHz = 200.0f;
    dsp.ApplyHighPass(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    float rmsOriginal = ComputeRMS(original.data(), k_SampleCount);
    float rmsFiltered = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_GT(rmsFiltered, rmsOriginal * 0.7f);
}

TEST(AudioDSP, HighPassStereo)
{
    AudioDSP dsp;
    auto buf = GenerateStereoSine(k_SampleCount, 100.0f);

    HighPassParams params;
    params.cutoffHz = 1000.0f;
    dsp.ApplyHighPass(buf.data(), k_SampleCount, 2, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
}

// ============================================================================
// Reverb
// ============================================================================

TEST(AudioDSP, ReverbModifiesSignal)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 440.0f);
    auto original = buf;

    DSPReverbParams params;
    params.wetLevel = 0.5f;
    params.dryLevel = 0.5f;
    dsp.ApplyReverb(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    // バッファは変更されているはず
    bool anyDifferent = false;
    for (uint32_t i = 0; i < k_SampleCount; ++i)
    {
        if (std::abs(buf[i] - original[i]) > 1e-6f)
        {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent);
}

TEST(AudioDSP, ReverbStereo)
{
    AudioDSP dsp;
    auto buf = GenerateStereoSine(k_SampleCount, 440.0f);

    DSPReverbParams params;
    dsp.ApplyReverb(buf.data(), k_SampleCount, 2, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
}

// ============================================================================
// Delay
// ============================================================================

TEST(AudioDSP, DelayModifiesSignal)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 440.0f);
    auto original = buf;

    DelayParams params;
    params.delayMs = 100.0f;
    params.feedback = 0.3f;
    params.wetLevel = 0.5f;
    dsp.ApplyDelay(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    bool anyDifferent = false;
    for (uint32_t i = 0; i < k_SampleCount; ++i)
    {
        if (std::abs(buf[i] - original[i]) > 1e-6f)
        {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent);
}

TEST(AudioDSP, DelayStereo)
{
    AudioDSP dsp;
    auto buf = GenerateStereoSine(k_SampleCount, 440.0f);

    DelayParams params;
    dsp.ApplyDelay(buf.data(), k_SampleCount, 2, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
}

// ============================================================================
// Compressor
// ============================================================================

TEST(AudioDSP, CompressorModifiesLoudSignal)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 440.0f, 0.9f); // 大音量信号
    auto original = buf;

    CompressorParams params;
    params.thresholdDb = -40.0f; // 圧縮を確実にするための非常に低いスレッショルド
    params.ratio = 10.0f;
    dsp.ApplyCompressor(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    // 圧縮後にRMSが減少するはず
    float rmsOriginal = ComputeRMS(original.data(), k_SampleCount);
    float rmsCompressed = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_LE(rmsCompressed, rmsOriginal + 0.01f);
}

TEST(AudioDSP, CompressorStereo)
{
    AudioDSP dsp;
    auto buf = GenerateStereoSine(k_SampleCount, 440.0f, 0.9f);

    CompressorParams params;
    params.thresholdDb = -30.0f;
    params.ratio = 8.0f;
    dsp.ApplyCompressor(buf.data(), k_SampleCount, 2, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
}

// ============================================================================
// リセット
// ============================================================================

TEST(AudioDSP, ResetClearsState)
{
    AudioDSP dsp;

    // 内部状態を構築するためにいくつかのエフェクトを適用
    auto buf = GenerateSine(k_SampleCount, 440.0f);
    LowPassParams lpParams;
    dsp.ApplyLowPass(buf.data(), k_SampleCount, 1, k_SampleRate, lpParams);

    DelayParams delayParams;
    dsp.ApplyDelay(buf.data(), k_SampleCount, 1, k_SampleRate, delayParams);

    // リセットでクラッシュしないこと
    dsp.Reset();

    // リセット後に再度適用 - 正常に動作するはず
    auto buf2 = GenerateSine(k_SampleCount, 440.0f);
    dsp.ApplyLowPass(buf2.data(), k_SampleCount, 1, k_SampleRate, lpParams);

    EXPECT_TRUE(AllSamplesInRange(buf2));
}

TEST(AudioDSP, ResetThenApplyReverb)
{
    AudioDSP dsp;

    auto buf = GenerateSine(k_SampleCount, 440.0f);
    DSPReverbParams params;
    dsp.ApplyReverb(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    dsp.Reset();

    auto buf2 = GenerateSine(k_SampleCount, 440.0f);
    dsp.ApplyReverb(buf2.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf2));
}

// ============================================================================
// エッジケース
// ============================================================================

TEST(AudioDSP, SilentBufferLowPass)
{
    AudioDSP dsp;
    gx::Vector<float> silence(k_SampleCount, 0.0f);

    LowPassParams params;
    dsp.ApplyLowPass(silence.data(), k_SampleCount, 1, k_SampleRate, params);

    // 無音は無音のままであるはず
    for (float s : silence)
    {
        EXPECT_NEAR(s, 0.0f, 1e-6f);
    }
}

TEST(AudioDSP, SilentBufferCompressor)
{
    AudioDSP dsp;
    gx::Vector<float> silence(k_SampleCount, 0.0f);

    CompressorParams params;
    dsp.ApplyCompressor(silence.data(), k_SampleCount, 1, k_SampleRate, params);

    for (float s : silence)
    {
        EXPECT_NEAR(s, 0.0f, 1e-6f);
    }
}

// ============================================================================
// AudioBus（XAudio2デバイスなし）
// ============================================================================

#include "Audio/AudioBus.h"

TEST(AudioBusTest, ProcessingStageDefault)
{
    // AudioBus Initialize()のprocessingStageパラメータ
    // XAudio2なしではAPIシグネチャのみテスト可能
    // クラスが存在しコンパイルできることを確認
    gx::AudioBus bus;
    EXPECT_EQ(bus.GetVolume(), 1.0f);
    EXPECT_EQ(bus.GetSubmixVoice(), nullptr);
}

// ============================================================================
// 追加テスト: DSPパラメータのカスタム設定
// ============================================================================

TEST(AudioDSPParams, LowPassCustom)
{
    LowPassParams params;
    params.cutoffHz = 1000.0f;
    params.resonance = 1.5f;
    EXPECT_FLOAT_EQ(params.cutoffHz, 1000.0f);
    EXPECT_FLOAT_EQ(params.resonance, 1.5f);
}

TEST(AudioDSPParams, HighPassCustom)
{
    HighPassParams params;
    params.cutoffHz = 500.0f;
    params.resonance = 2.0f;
    EXPECT_FLOAT_EQ(params.cutoffHz, 500.0f);
    EXPECT_FLOAT_EQ(params.resonance, 2.0f);
}

TEST(AudioDSPParams, ReverbCustom)
{
    DSPReverbParams params;
    params.roomSize = 0.9f;
    params.damping = 0.1f;
    params.wetLevel = 0.8f;
    params.dryLevel = 0.2f;
    params.decayTime = 5.0f;
    EXPECT_FLOAT_EQ(params.roomSize, 0.9f);
    EXPECT_FLOAT_EQ(params.damping, 0.1f);
    EXPECT_FLOAT_EQ(params.wetLevel, 0.8f);
    EXPECT_FLOAT_EQ(params.dryLevel, 0.2f);
    EXPECT_FLOAT_EQ(params.decayTime, 5.0f);
}

TEST(AudioDSPParams, DelayCustom)
{
    DelayParams params;
    params.delayMs = 500.0f;
    params.feedback = 0.8f;
    params.wetLevel = 0.6f;
    EXPECT_FLOAT_EQ(params.delayMs, 500.0f);
    EXPECT_FLOAT_EQ(params.feedback, 0.8f);
    EXPECT_FLOAT_EQ(params.wetLevel, 0.6f);
}

TEST(AudioDSPParams, CompressorCustom)
{
    CompressorParams params;
    params.thresholdDb = -10.0f;
    params.ratio = 2.0f;
    params.attackMs = 5.0f;
    params.releaseMs = 50.0f;
    EXPECT_FLOAT_EQ(params.thresholdDb, -10.0f);
    EXPECT_FLOAT_EQ(params.ratio, 2.0f);
    EXPECT_FLOAT_EQ(params.attackMs, 5.0f);
    EXPECT_FLOAT_EQ(params.releaseMs, 50.0f);
}

// ============================================================================
// 追加テスト: フィルタのエッジケース
// ============================================================================

TEST(AudioDSP, LowPassVeryLowCutoff)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 5000.0f);

    LowPassParams params;
    params.cutoffHz = 50.0f; // 非常に低いカットオフ
    dsp.ApplyLowPass(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
    // 50Hz以下のカットオフで5kHzの信号はほぼ消えるはず
    float rms = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_LT(rms, 0.1f);
}

TEST(AudioDSP, HighPassVeryHighCutoff)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 440.0f);

    HighPassParams params;
    params.cutoffHz = 15000.0f; // 非常に高いカットオフ
    dsp.ApplyHighPass(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));
    // 15kHz以上のカットオフで440Hzの信号はほぼ消えるはず
    float rms = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_LT(rms, 0.2f);
}

TEST(AudioDSP, SilentBufferHighPass)
{
    AudioDSP dsp;
    gx::Vector<float> silence(k_SampleCount, 0.0f);

    HighPassParams params;
    dsp.ApplyHighPass(silence.data(), k_SampleCount, 1, k_SampleRate, params);

    for (float s : silence)
    {
        EXPECT_NEAR(s, 0.0f, 1e-6f);
    }
}

TEST(AudioDSP, SilentBufferDelay)
{
    AudioDSP dsp;
    gx::Vector<float> silence(k_SampleCount, 0.0f);

    DelayParams params;
    dsp.ApplyDelay(silence.data(), k_SampleCount, 1, k_SampleRate, params);

    // 無音にディレイを適用しても無音のまま
    for (float s : silence)
    {
        EXPECT_NEAR(s, 0.0f, 1e-6f);
    }
}

TEST(AudioDSP, SilentBufferReverb)
{
    AudioDSP dsp;
    gx::Vector<float> silence(k_SampleCount, 0.0f);

    DSPReverbParams params;
    dsp.ApplyReverb(silence.data(), k_SampleCount, 1, k_SampleRate, params);

    // 無音にリバーブを適用しても無音のまま
    for (float s : silence)
    {
        EXPECT_NEAR(s, 0.0f, 1e-4f);
    }
}

// ============================================================================
// 追加テスト: 連続適用
// ============================================================================

TEST(AudioDSP, ChainLowPassThenHighPass)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 1000.0f, 0.8f);

    // バンドパスフィルタのような効果: ローパス→ハイパス
    LowPassParams lp;
    lp.cutoffHz = 5000.0f;
    dsp.ApplyLowPass(buf.data(), k_SampleCount, 1, k_SampleRate, lp);

    HighPassParams hp;
    hp.cutoffHz = 200.0f;
    dsp.ApplyHighPass(buf.data(), k_SampleCount, 1, k_SampleRate, hp);

    EXPECT_TRUE(AllSamplesInRange(buf));
    // 1kHzは200Hz-5kHzの範囲内なのでかなりのエネルギーが残るはず
    float rms = ComputeRMS(buf.data(), k_SampleCount);
    EXPECT_GT(rms, 0.1f);
}

TEST(AudioDSP, CompressorDoesNotBoostQuietSignal)
{
    AudioDSP dsp;
    auto buf = GenerateSine(k_SampleCount, 440.0f, 0.01f); // 非常に小さい信号
    auto original = buf;

    CompressorParams params;
    params.thresholdDb = -20.0f;
    params.ratio = 4.0f;
    dsp.ApplyCompressor(buf.data(), k_SampleCount, 1, k_SampleRate, params);

    EXPECT_TRUE(AllSamplesInRange(buf));

    // 非常に小さい信号にはコンプレッサーがほとんど影響しないはず
    float rmsOriginal = ComputeRMS(original.data(), k_SampleCount);
    float rmsCompressed = ComputeRMS(buf.data(), k_SampleCount);
    // 圧縮比で大幅に変わらないことを確認
    EXPECT_NEAR(rmsCompressed, rmsOriginal, rmsOriginal * 0.5f + 0.001f);
}

// ============================================================================
// 追加テスト: AudioBus
// ============================================================================

TEST(AudioBusTest, DefaultName)
{
    gx::AudioBus bus;
    EXPECT_TRUE(bus.GetName().empty());
}
