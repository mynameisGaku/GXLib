/// @file test_ConvolutionReverb.cpp
/// @brief ConvolutionReverb のユニットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/AudioDSP.h"
#include <cmath>

using namespace gx;

static constexpr float kEps = 1e-4f;
static constexpr int k_SampleRate = 44100;

// ============================================================================
// ヘルパー
// ============================================================================

/// モノラル正弦波バッファを生成
static gx::Vector<float> GenerateSine(int numSamples, float frequency, float amplitude = 0.8f)
{
    gx::Vector<float> buf(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(k_SampleRate);
        buf[i] = amplitude * std::sin(2.0f * 3.14159265f * frequency * t);
    }
    return buf;
}

/// インパルスIR（最初のサンプルが1.0、残りが0）
static gx::Vector<float> MakeImpulseIR(int length)
{
    gx::Vector<float> ir(length, 0.0f);
    if (length > 0) ir[0] = 1.0f;
    return ir;
}

/// 均一減衰IR
static gx::Vector<float> MakeDecayIR(int length, float decay)
{
    gx::Vector<float> ir(length);
    for (int i = 0; i < length; ++i)
        ir[i] = std::pow(decay, static_cast<float>(i));
    return ir;
}

/// バッファのRMSを計算
static float ComputeRMS(const float* data, int count)
{
    float sum = 0.0f;
    for (int i = 0; i < count; ++i)
        sum += data[i] * data[i];
    return std::sqrt(sum / static_cast<float>(count));
}

/// すべてのサンプルが有限値であるか
static bool AllSamplesFinite(const gx::Vector<float>& buf)
{
    for (float s : buf)
    {
        if (std::isnan(s) || std::isinf(s))
            return false;
    }
    return true;
}

// ============================================================================
// ConvolutionReverb defaults
// ============================================================================

TEST(ConvolutionReverbTest, DefaultNotInitialized)
{
    ConvolutionReverb reverb;
    EXPECT_FALSE(reverb.IsInitialized());
}

TEST(ConvolutionReverbTest, DefaultWetDry)
{
    ConvolutionReverb reverb;
    EXPECT_FLOAT_EQ(reverb.GetWetDry(), 0.5f);
}

TEST(ConvolutionReverbTest, DefaultPreDelay)
{
    ConvolutionReverb reverb;
    EXPECT_FLOAT_EQ(reverb.GetPreDelay(), 0.0f);
}

TEST(ConvolutionReverbTest, DefaultIRLength)
{
    ConvolutionReverb reverb;
    EXPECT_EQ(reverb.GetIRLength(), 0);
}

TEST(ConvolutionReverbTest, DefaultSampleRate)
{
    ConvolutionReverb reverb;
    EXPECT_EQ(reverb.GetSampleRate(), 44100);
}

// ============================================================================
// Initialize
// ============================================================================

TEST(ConvolutionReverbTest, InitializeWithIR)
{
    ConvolutionReverb reverb;
    auto ir = MakeImpulseIR(256);
    reverb.Initialize(ir.data(), static_cast<int>(ir.size()), k_SampleRate);
    EXPECT_TRUE(reverb.IsInitialized());
    EXPECT_EQ(reverb.GetIRLength(), 256);
    EXPECT_EQ(reverb.GetSampleRate(), k_SampleRate);
}

TEST(ConvolutionReverbTest, InitializeWithNullptr)
{
    ConvolutionReverb reverb;
    reverb.Initialize(nullptr, 100);
    EXPECT_FALSE(reverb.IsInitialized());
}

TEST(ConvolutionReverbTest, InitializeWithZeroLength)
{
    ConvolutionReverb reverb;
    float dummy = 1.0f;
    reverb.Initialize(&dummy, 0);
    EXPECT_FALSE(reverb.IsInitialized());
}

TEST(ConvolutionReverbTest, InitializeWithNegativeLength)
{
    ConvolutionReverb reverb;
    float dummy = 1.0f;
    reverb.Initialize(&dummy, -1);
    EXPECT_FALSE(reverb.IsInitialized());
}

TEST(ConvolutionReverbTest, InitializeWithInvalidSampleRate)
{
    ConvolutionReverb reverb;
    float dummy = 1.0f;
    reverb.Initialize(&dummy, 1, 0);
    EXPECT_FALSE(reverb.IsInitialized());
}

TEST(ConvolutionReverbTest, InitializeWithCustomSampleRate)
{
    ConvolutionReverb reverb;
    auto ir = MakeImpulseIR(128);
    reverb.Initialize(ir.data(), 128, 48000);
    EXPECT_TRUE(reverb.IsInitialized());
    EXPECT_EQ(reverb.GetSampleRate(), 48000);
}

// ============================================================================
// SetWetDry / SetPreDelay
// ============================================================================

TEST(ConvolutionReverbTest, SetWetDry)
{
    ConvolutionReverb reverb;
    reverb.SetWetDry(0.3f);
    EXPECT_FLOAT_EQ(reverb.GetWetDry(), 0.3f);
}

TEST(ConvolutionReverbTest, SetWetDryClampHigh)
{
    ConvolutionReverb reverb;
    reverb.SetWetDry(1.5f);
    EXPECT_FLOAT_EQ(reverb.GetWetDry(), 1.0f);
}

TEST(ConvolutionReverbTest, SetWetDryClampLow)
{
    ConvolutionReverb reverb;
    reverb.SetWetDry(-0.5f);
    EXPECT_FLOAT_EQ(reverb.GetWetDry(), 0.0f);
}

TEST(ConvolutionReverbTest, SetPreDelay)
{
    ConvolutionReverb reverb;
    reverb.SetPreDelay(20.0f);
    EXPECT_FLOAT_EQ(reverb.GetPreDelay(), 20.0f);
}

TEST(ConvolutionReverbTest, SetPreDelayNegativeClamped)
{
    ConvolutionReverb reverb;
    reverb.SetPreDelay(-10.0f);
    EXPECT_FLOAT_EQ(reverb.GetPreDelay(), 0.0f);
}

// ============================================================================
// Process — impulse IR identity test
// ============================================================================

TEST(ConvolutionReverbTest, ImpulseIR_FullWet)
{
    // 純粋なインパルスIR (delta) + wet=1.0 → 出力 ≈ 入力
    ConvolutionReverb reverb;
    auto ir = MakeImpulseIR(64);
    reverb.Initialize(ir.data(), 64, k_SampleRate);
    reverb.SetWetDry(1.0f);

    auto signal = GenerateSine(512, 440.0f, 0.5f);
    auto original = signal; // コピー保持

    reverb.Process(signal.data(), 512);

    // 出力は入力とほぼ同じはず (IR = delta function)
    EXPECT_TRUE(AllSamplesFinite(signal));
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_NEAR(signal[i], original[i], 0.01f)
            << "Mismatch at sample " << i;
    }
}

TEST(ConvolutionReverbTest, ImpulseIR_FullDry)
{
    // wet=0.0 → 出力 = 入力（dry）
    ConvolutionReverb reverb;
    auto ir = MakeDecayIR(128, 0.98f);
    reverb.Initialize(ir.data(), 128, k_SampleRate);
    reverb.SetWetDry(0.0f);

    auto signal = GenerateSine(256, 440.0f, 0.5f);
    auto original = signal;

    reverb.Process(signal.data(), 256);

    EXPECT_TRUE(AllSamplesFinite(signal));
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_NEAR(signal[i], original[i], kEps)
            << "Mismatch at sample " << i;
    }
}

// ============================================================================
// Process — reverb adds energy
// ============================================================================

TEST(ConvolutionReverbTest, ReverbAddsEnergy)
{
    // 減衰IR + wet>0 → RMS should differ from dry
    ConvolutionReverb reverb;
    auto ir = MakeDecayIR(256, 0.95f);
    reverb.Initialize(ir.data(), 256, k_SampleRate);
    reverb.SetWetDry(0.5f);

    auto signal = GenerateSine(1024, 440.0f, 0.5f);
    auto original = signal;

    float rmsOriginal = ComputeRMS(original.data(), 1024);

    reverb.Process(signal.data(), 1024);

    float rmsProcessed = ComputeRMS(signal.data(), 1024);

    EXPECT_TRUE(AllSamplesFinite(signal));
    // With a decay IR and wet mixing, the RMS should change
    // (it can be larger or different from original)
    EXPECT_NE(rmsOriginal, rmsProcessed);
}

// ============================================================================
// Process — null / empty safety
// ============================================================================

TEST(ConvolutionReverbTest, ProcessNullBuffer)
{
    ConvolutionReverb reverb;
    auto ir = MakeImpulseIR(64);
    reverb.Initialize(ir.data(), 64, k_SampleRate);
    reverb.Process(nullptr, 100); // should not crash
}

TEST(ConvolutionReverbTest, ProcessZeroSamples)
{
    ConvolutionReverb reverb;
    auto ir = MakeImpulseIR(64);
    reverb.Initialize(ir.data(), 64, k_SampleRate);
    float buf[] = { 0.5f };
    reverb.Process(buf, 0); // should not crash
    EXPECT_FLOAT_EQ(buf[0], 0.5f); // unchanged
}

TEST(ConvolutionReverbTest, ProcessWithoutInit)
{
    ConvolutionReverb reverb;
    auto signal = GenerateSine(256, 440.0f);
    auto original = signal;
    reverb.Process(signal.data(), 256); // should be a no-op
    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ(signal[i], original[i]);
}

// ============================================================================
// Reset
// ============================================================================

TEST(ConvolutionReverbTest, ResetClearsState)
{
    ConvolutionReverb reverb;
    auto ir = MakeDecayIR(128, 0.95f);
    reverb.Initialize(ir.data(), 128, k_SampleRate);
    reverb.SetWetDry(0.7f);

    // Process some data to build up internal state
    auto signal = GenerateSine(512, 440.0f, 0.5f);
    reverb.Process(signal.data(), 512);

    // Reset
    reverb.Reset();

    // IR should still be valid
    EXPECT_TRUE(reverb.IsInitialized());
    EXPECT_EQ(reverb.GetIRLength(), 128);

    // Process silence — output should be silence (no tail from previous signal)
    gx::Vector<float> silence(256, 0.0f);
    reverb.Process(silence.data(), 256);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ(silence[i], 0.0f);
}

TEST(ConvolutionReverbTest, ResetWithoutInit)
{
    ConvolutionReverb reverb;
    reverb.Reset(); // should not crash
    EXPECT_FALSE(reverb.IsInitialized());
}

// ============================================================================
// GenerateRoomIR
// ============================================================================

TEST(ConvolutionReverbTest, GenerateRoomIR_SmallRoom)
{
    auto ir = ConvolutionReverb::GenerateRoomIR(3.0f, 0.5f, k_SampleRate);
    EXPECT_FALSE(ir.empty());
    EXPECT_LE(static_cast<int>(ir.size()), 4096);
    EXPECT_GE(static_cast<int>(ir.size()), 64);

    // First sample should be the direct sound (normalized peak)
    EXPECT_FLOAT_EQ(ir[0], 1.0f);

    // All samples should be finite
    for (float s : ir)
    {
        EXPECT_FALSE(std::isnan(s));
        EXPECT_FALSE(std::isinf(s));
    }
}

TEST(ConvolutionReverbTest, GenerateRoomIR_LargeRoom)
{
    auto ir = ConvolutionReverb::GenerateRoomIR(30.0f, 0.2f, k_SampleRate);
    EXPECT_FALSE(ir.empty());
    EXPECT_LE(static_cast<int>(ir.size()), 4096);

    // Large room IR should be longer than small room
    auto irSmall = ConvolutionReverb::GenerateRoomIR(3.0f, 0.2f, k_SampleRate);
    EXPECT_GE(ir.size(), irSmall.size());
}

TEST(ConvolutionReverbTest, GenerateRoomIR_HighDamping)
{
    auto irLow = ConvolutionReverb::GenerateRoomIR(10.0f, 0.1f, k_SampleRate);
    auto irHigh = ConvolutionReverb::GenerateRoomIR(10.0f, 0.9f, k_SampleRate);

    // High damping should produce shorter IR
    EXPECT_LE(irHigh.size(), irLow.size());
}

TEST(ConvolutionReverbTest, GenerateRoomIR_InvalidSize)
{
    auto ir = ConvolutionReverb::GenerateRoomIR(0.0f, 0.5f, k_SampleRate);
    EXPECT_TRUE(ir.empty());
}

TEST(ConvolutionReverbTest, GenerateRoomIR_NegativeSize)
{
    auto ir = ConvolutionReverb::GenerateRoomIR(-5.0f, 0.5f, k_SampleRate);
    EXPECT_TRUE(ir.empty());
}

TEST(ConvolutionReverbTest, GenerateRoomIR_InvalidSampleRate)
{
    auto ir = ConvolutionReverb::GenerateRoomIR(5.0f, 0.5f, 0);
    EXPECT_TRUE(ir.empty());
}

TEST(ConvolutionReverbTest, GenerateRoomIR_DampingClamp)
{
    // Damping > 1.0 should be clamped
    auto ir = ConvolutionReverb::GenerateRoomIR(5.0f, 2.0f, k_SampleRate);
    EXPECT_FALSE(ir.empty());
    for (float s : ir)
    {
        EXPECT_FALSE(std::isnan(s));
        EXPECT_FALSE(std::isinf(s));
    }
}

TEST(ConvolutionReverbTest, GenerateRoomIR_Deterministic)
{
    // Same parameters should produce the same IR (deterministic noise)
    auto ir1 = ConvolutionReverb::GenerateRoomIR(8.0f, 0.4f, k_SampleRate);
    auto ir2 = ConvolutionReverb::GenerateRoomIR(8.0f, 0.4f, k_SampleRate);
    ASSERT_EQ(ir1.size(), ir2.size());
    for (size_t i = 0; i < ir1.size(); ++i)
        EXPECT_FLOAT_EQ(ir1[i], ir2[i]);
}

TEST(ConvolutionReverbTest, GenerateRoomIR_DifferentSampleRates)
{
    // Use small room size so IR length stays below 4096-sample cap
    auto ir44 = ConvolutionReverb::GenerateRoomIR(0.5f, 0.5f, 44100);
    auto ir48 = ConvolutionReverb::GenerateRoomIR(0.5f, 0.5f, 48000);
    // Different sample rates produce different IR lengths (more samples at higher rate)
    EXPECT_NE(ir44.size(), ir48.size());
}

// ============================================================================
// Integration: GenerateRoomIR + Process
// ============================================================================

TEST(ConvolutionReverbTest, Integration_RoomIRProcess)
{
    auto ir = ConvolutionReverb::GenerateRoomIR(5.0f, 0.5f, k_SampleRate);
    ASSERT_FALSE(ir.empty());

    ConvolutionReverb reverb;
    reverb.Initialize(ir.data(), static_cast<int>(ir.size()), k_SampleRate);
    reverb.SetWetDry(0.4f);

    auto signal = GenerateSine(1024, 440.0f, 0.5f);
    reverb.Process(signal.data(), 1024);

    EXPECT_TRUE(AllSamplesFinite(signal));
    // Signal should be modified (not all zeros)
    float rms = ComputeRMS(signal.data(), 1024);
    EXPECT_GT(rms, 0.0f);
}

// ============================================================================
// Continuous processing (state preservation)
// ============================================================================

TEST(ConvolutionReverbTest, ContinuousProcessing)
{
    ConvolutionReverb reverb;
    auto ir = MakeDecayIR(64, 0.9f);
    reverb.Initialize(ir.data(), 64, k_SampleRate);
    reverb.SetWetDry(0.5f);

    // Process in chunks — should produce smooth result
    gx::Vector<float> chunk1 = GenerateSine(256, 440.0f, 0.5f);
    gx::Vector<float> chunk2 = GenerateSine(256, 440.0f, 0.5f);

    reverb.Process(chunk1.data(), 256);
    reverb.Process(chunk2.data(), 256);

    EXPECT_TRUE(AllSamplesFinite(chunk1));
    EXPECT_TRUE(AllSamplesFinite(chunk2));
}

// ============================================================================
// PreDelay effect
// ============================================================================

TEST(ConvolutionReverbTest, PreDelayEffect)
{
    // With predelay, the wet signal should be delayed
    ConvolutionReverb reverbNoDelay;
    ConvolutionReverb reverbWithDelay;

    auto ir = MakeImpulseIR(64);

    reverbNoDelay.Initialize(ir.data(), 64, k_SampleRate);
    reverbNoDelay.SetWetDry(1.0f);
    reverbNoDelay.SetPreDelay(0.0f);

    reverbWithDelay.Initialize(ir.data(), 64, k_SampleRate);
    reverbWithDelay.SetWetDry(1.0f);
    reverbWithDelay.SetPreDelay(50.0f); // 50ms pre-delay

    // Create identical input signals
    auto signal1 = GenerateSine(1024, 440.0f, 0.5f);
    auto signal2 = signal1; // copy

    reverbNoDelay.Process(signal1.data(), 1024);
    reverbWithDelay.Process(signal2.data(), 1024);

    // The signals should be different due to predelay
    bool different = false;
    for (int i = 0; i < 1024; ++i)
    {
        if (std::fabs(signal1[i] - signal2[i]) > kEps)
        {
            different = true;
            break;
        }
    }
    EXPECT_TRUE(different);
}

// ============================================================================
// Short IR edge case
// ============================================================================

TEST(ConvolutionReverbTest, SingleSampleIR)
{
    ConvolutionReverb reverb;
    float ir = 0.5f;
    reverb.Initialize(&ir, 1, k_SampleRate);
    reverb.SetWetDry(1.0f);

    EXPECT_TRUE(reverb.IsInitialized());
    EXPECT_EQ(reverb.GetIRLength(), 1);

    auto signal = GenerateSine(256, 440.0f, 0.8f);
    auto original = signal;

    reverb.Process(signal.data(), 256);

    // IR = {0.5}, wet=1.0 → output = input * 0.5
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_NEAR(signal[i], original[i] * 0.5f, kEps);
    }
}
