/// @file test_PostEffectIntegration.cpp
/// @brief PostFXFlagビットマスク操作、トーンマッピングCPU参照実装、パラメータ範囲、CB検証

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/PostEffect/PostFXMask.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// Tonemap CPU reference helpers
// ============================================================================

static float ReinhardTonemap(float x)
{
    return x / (1.0f + x);
}

static float ACESTonemap(float x)
{
    // ACES filmic curve approximation
    return x * (2.51f * x + 0.03f) / (x * (2.43f * x + 0.59f) + 0.14f);
}

// Uncharted2 per-component curve
static float Uncharted2Curve(float x)
{
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

static float Uncharted2Tonemap(float x)
{
    const float W = 11.2f;
    float curr = Uncharted2Curve(x);
    float whiteScale = 1.0f / Uncharted2Curve(W);
    return curr * whiteScale;
}

// ============================================================================
// PostFXFlag Bitmask tests
// ============================================================================

TEST(PostFXFlagBitmask, None_IsZero)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::None), 0u);
}

TEST(PostFXFlagBitmask, SSAO_IsBit0)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::SSAO), 1u << 0);
}

TEST(PostFXFlagBitmask, ContactShadows_IsBit1)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::ContactShadows), 1u << 1);
}

TEST(PostFXFlagBitmask, Reflections_IsBit2)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::Reflections), 1u << 2);
}

TEST(PostFXFlagBitmask, VolumetricLight_IsBit3)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::VolumetricLight), 1u << 3);
}

TEST(PostFXFlagBitmask, Bloom_IsBit5)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::Bloom), 1u << 5);
}

TEST(PostFXFlagBitmask, TAA_IsBit9)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::TAA), 1u << 9);
}

TEST(PostFXFlagBitmask, FXAA_IsBit11)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::FXAA), 1u << 11);
}

TEST(PostFXFlagBitmask, RTSoftShadows_IsBit17)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::RTSoftShadows), 1u << 17);
}

TEST(PostFXFlagBitmask, All_IsAllOnes)
{
    EXPECT_EQ(static_cast<uint32_t>(PostFXFlag::All), 0xFFFFFFFFu);
}

TEST(PostFXFlagBitmask, OR_CombinesFlags)
{
    PostFXFlag combined = PostFXFlag::SSAO | PostFXFlag::Bloom;
    EXPECT_EQ(static_cast<uint32_t>(combined),
              (1u << 0) | (1u << 5));
}

TEST(PostFXFlagBitmask, AND_MasksFlags)
{
    PostFXFlag combined = PostFXFlag::SSAO | PostFXFlag::Bloom | PostFXFlag::TAA;
    PostFXFlag masked = combined & PostFXFlag::Bloom;
    EXPECT_EQ(static_cast<uint32_t>(masked), 1u << 5);
}

TEST(PostFXFlagBitmask, NOT_InvertsFlags)
{
    PostFXFlag inv = ~PostFXFlag::None;
    EXPECT_EQ(static_cast<uint32_t>(inv), 0xFFFFFFFFu);
}

TEST(PostFXFlagBitmask, HasFlag_ReturnsTrueForIncluded)
{
    PostFXFlag mask = PostFXFlag::SSAO | PostFXFlag::Bloom;
    EXPECT_TRUE(HasFlag(mask, PostFXFlag::SSAO));
    EXPECT_TRUE(HasFlag(mask, PostFXFlag::Bloom));
}

TEST(PostFXFlagBitmask, HasFlag_ReturnsFalseForExcluded)
{
    PostFXFlag mask = PostFXFlag::SSAO | PostFXFlag::Bloom;
    EXPECT_FALSE(HasFlag(mask, PostFXFlag::TAA));
    EXPECT_FALSE(HasFlag(mask, PostFXFlag::FXAA));
}

TEST(PostFXFlagBitmask, HasFlag_NoneHasNoFlags)
{
    EXPECT_FALSE(HasFlag(PostFXFlag::None, PostFXFlag::SSAO));
    EXPECT_FALSE(HasFlag(PostFXFlag::None, PostFXFlag::Bloom));
}

TEST(PostFXFlagBitmask, HasFlag_AllIncludesEveryFlag)
{
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::SSAO));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::ContactShadows));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::Reflections));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::VolumetricLight));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::Bloom));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::DoF));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::MotionBlur));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::Outline));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::TAA));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::ColorGrading));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::FXAA));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::Vignette));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::RTGI));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::AutoExposure));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::LensFlare));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::VolumetricFog));
    EXPECT_TRUE(HasFlag(PostFXFlag::All, PostFXFlag::RTSoftShadows));
}

TEST(PostFXFlagBitmask, CompoundAssignOR)
{
    PostFXFlag mask = PostFXFlag::None;
    mask |= PostFXFlag::SSAO;
    mask |= PostFXFlag::TAA;
    EXPECT_TRUE(HasFlag(mask, PostFXFlag::SSAO));
    EXPECT_TRUE(HasFlag(mask, PostFXFlag::TAA));
    EXPECT_FALSE(HasFlag(mask, PostFXFlag::Bloom));
}

TEST(PostFXFlagBitmask, CompoundAssignAND)
{
    PostFXFlag mask = PostFXFlag::All;
    mask &= ~PostFXFlag::SSAO;
    EXPECT_FALSE(HasFlag(mask, PostFXFlag::SSAO));
    EXPECT_TRUE(HasFlag(mask, PostFXFlag::Bloom));
}

// ============================================================================
// Tonemap CPU reference tests
// ============================================================================

TEST(TonemapCPU, Reinhard_BlackReturnsZero)
{
    EXPECT_NEAR(ReinhardTonemap(0.0f), 0.0f, kEps);
}

TEST(TonemapCPU, Reinhard_OneReturnHalf)
{
    EXPECT_NEAR(ReinhardTonemap(1.0f), 0.5f, kEps);
}

TEST(TonemapCPU, Reinhard_LargeValueApproachesOne)
{
    float result = ReinhardTonemap(1000.0f);
    EXPECT_GT(result, 0.99f);
    EXPECT_LE(result, 1.0f);
}

TEST(TonemapCPU, ACES_BlackReturnsNearZero)
{
    float result = ACESTonemap(0.0f);
    // ACES(0) = 0 * (0+0.03) / (0 + 0.14) = 0
    EXPECT_NEAR(result, 0.0f, 0.01f);
}

TEST(TonemapCPU, ACES_OneReturnsMidtone)
{
    float result = ACESTonemap(1.0f);
    // expected: 1*(2.51+0.03)/(1*(2.43+0.59)+0.14) = 2.54/3.16 ≈ 0.8038
    EXPECT_NEAR(result, 2.54f / 3.16f, 0.01f);
}

TEST(TonemapCPU, ACES_WhiteClamped)
{
    float result = ACESTonemap(100.0f);
    EXPECT_GT(result, 0.9f);
    EXPECT_LE(result, 1.1f); // ACES can slightly overshoot 1.0
}

TEST(TonemapCPU, Uncharted2_BlackReturnsZero)
{
    EXPECT_NEAR(Uncharted2Tonemap(0.0f), 0.0f, 0.02f);
}

TEST(TonemapCPU, Uncharted2_PositiveInputPositiveOutput)
{
    EXPECT_GT(Uncharted2Tonemap(1.0f), 0.0f);
    EXPECT_LT(Uncharted2Tonemap(1.0f), 1.0f);
}

TEST(TonemapCPU, Uncharted2_MonotonicIncrease)
{
    float v1 = Uncharted2Tonemap(0.5f);
    float v2 = Uncharted2Tonemap(1.0f);
    float v3 = Uncharted2Tonemap(2.0f);
    EXPECT_LT(v1, v2);
    EXPECT_LT(v2, v3);
}

// ============================================================================
// Pipeline defaults and parameter tests
// ============================================================================

TEST(PipelineDefaults, TonemapModeDefault_ACES)
{
    PostEffectPipeline pipeline;
    EXPECT_EQ(pipeline.GetTonemapMode(), TonemapMode::ACES);
}

TEST(PipelineDefaults, FXAADefault_Enabled)
{
    PostEffectPipeline pipeline;
    EXPECT_TRUE(pipeline.IsFXAAEnabled());
}

TEST(PipelineDefaults, VignetteDefault_Disabled)
{
    PostEffectPipeline pipeline;
    EXPECT_FALSE(pipeline.IsVignetteEnabled());
}

TEST(PipelineDefaults, ColorGradingDefault_Disabled)
{
    PostEffectPipeline pipeline;
    EXPECT_FALSE(pipeline.IsColorGradingEnabled());
}

TEST(PipelineDefaults, ExposureDefault_One)
{
    PostEffectPipeline pipeline;
    EXPECT_NEAR(pipeline.GetExposure(), 1.0f, kEps);
}

TEST(ParameterRange, SetExposure_StoresValue)
{
    PostEffectPipeline pipeline;
    pipeline.SetExposure(2.5f);
    EXPECT_NEAR(pipeline.GetExposure(), 2.5f, kEps);
}

TEST(ParameterRange, VignetteIntensity_StoresValue)
{
    PostEffectPipeline pipeline;
    pipeline.SetVignetteIntensity(1.5f);
    EXPECT_NEAR(pipeline.GetVignetteIntensity(), 1.5f, kEps);
}

TEST(ParameterRange, ChromaticAberration_StoresValue)
{
    PostEffectPipeline pipeline;
    pipeline.SetChromaticAberration(0.05f);
    EXPECT_NEAR(pipeline.GetChromaticAberration(), 0.05f, kEps);
}

TEST(ParameterRange, Contrast_StoresValue)
{
    PostEffectPipeline pipeline;
    pipeline.SetContrast(1.5f);
    EXPECT_NEAR(pipeline.GetContrast(), 1.5f, kEps);
}

TEST(ParameterRange, Saturation_StoresValue)
{
    PostEffectPipeline pipeline;
    pipeline.SetSaturation(0.7f);
    EXPECT_NEAR(pipeline.GetSaturation(), 0.7f, kEps);
}

TEST(ParameterRange, Temperature_StoresValue)
{
    PostEffectPipeline pipeline;
    pipeline.SetTemperature(-0.3f);
    EXPECT_NEAR(pipeline.GetTemperature(), -0.3f, kEps);
}

TEST(ParameterRange, TonemapMode_SetReinhard)
{
    PostEffectPipeline pipeline;
    pipeline.SetTonemapMode(TonemapMode::Reinhard);
    EXPECT_EQ(pipeline.GetTonemapMode(), TonemapMode::Reinhard);
}

TEST(ParameterRange, TonemapMode_SetUncharted2)
{
    PostEffectPipeline pipeline;
    pipeline.SetTonemapMode(TonemapMode::Uncharted2);
    EXPECT_EQ(pipeline.GetTonemapMode(), TonemapMode::Uncharted2);
}

TEST(ParameterRange, TonemapModeEnumValues_Distinct)
{
    EXPECT_NE(static_cast<uint32_t>(TonemapMode::Reinhard),
              static_cast<uint32_t>(TonemapMode::ACES));
    EXPECT_NE(static_cast<uint32_t>(TonemapMode::ACES),
              static_cast<uint32_t>(TonemapMode::Uncharted2));
    EXPECT_NE(static_cast<uint32_t>(TonemapMode::Reinhard),
              static_cast<uint32_t>(TonemapMode::Uncharted2));
}

// ============================================================================
// CB size verification (256-byte alignment for D3D12)
// ============================================================================

TEST(CBSize, PostEffectPipeline_HDRFormat_R16G16B16A16)
{
    PostEffectPipeline pipeline;
    EXPECT_EQ(pipeline.GetHDRFormat(), DXGI_FORMAT_R16G16B16A16_FLOAT);
}

TEST(CBSize, RTReflections_DefaultNull)
{
    PostEffectPipeline pipeline;
    EXPECT_EQ(pipeline.GetRTReflections(), nullptr);
}

TEST(CBSize, RTGI_DefaultNull)
{
    PostEffectPipeline pipeline;
    EXPECT_EQ(pipeline.GetRTGI(), nullptr);
}
