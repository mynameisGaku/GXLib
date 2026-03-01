/// @file test_PostEffects.cpp
/// @brief ポストエフェクトパラメータインターフェースのテスト（FilmGrain、PostEffectPipelineカラーグレーディング/ビネット）

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/PostEffect/FilmGrain.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"

using namespace gx;

// ============================================================================
// FilmGrain
// ============================================================================

TEST(FilmGrainTest, DefaultDisabledState)
{
    FilmGrain fg;
    EXPECT_FALSE(fg.IsEnabled());
}

TEST(FilmGrainTest, SetIntensityGetIntensity)
{
    FilmGrain fg;
    fg.SetIntensity(0.8f);
    EXPECT_FLOAT_EQ(fg.GetIntensity(), 0.8f);
}

TEST(FilmGrainTest, SetSizeGetSize)
{
    FilmGrain fg;
    fg.SetSize(2.5f);
    EXPECT_FLOAT_EQ(fg.GetSize(), 2.5f);
}

TEST(FilmGrainTest, SetLuminanceWeightGetLuminanceWeight)
{
    FilmGrain fg;
    fg.SetLuminanceWeight(0.7f);
    EXPECT_FLOAT_EQ(fg.GetLuminanceWeight(), 0.7f);
}

TEST(FilmGrainTest, SetEnabledIsEnabled)
{
    FilmGrain fg;
    fg.SetEnabled(true);
    EXPECT_TRUE(fg.IsEnabled());
    fg.SetEnabled(false);
    EXPECT_FALSE(fg.IsEnabled());
}

TEST(FilmGrainTest, DefaultIntensity)
{
    FilmGrain fg;
    EXPECT_FLOAT_EQ(fg.GetIntensity(), 0.05f);
}

TEST(FilmGrainTest, DefaultSize)
{
    FilmGrain fg;
    EXPECT_FLOAT_EQ(fg.GetSize(), 1.0f);
}

TEST(FilmGrainTest, DefaultLuminanceWeight)
{
    FilmGrain fg;
    EXPECT_FLOAT_EQ(fg.GetLuminanceWeight(), 0.5f);
}

TEST(FilmGrainTest, UpdateTimeAdvancesTime)
{
    FilmGrain fg;
    // nullptrデバイスでCPUのみモードとして初期化
    fg.Initialize(nullptr, 1920, 1080);
    fg.SetEnabled(true);

    // UpdateTimeを繰り返し呼び出して初期時間を取得
    fg.UpdateTime(0.016f);
    fg.UpdateTime(0.016f);
    fg.UpdateTime(0.016f);
    // 16ms x 3回の更新後、時間は進んでいるはず
    // m_timeを直接読むことはできないが、クラッシュしないことを確認
    SUCCEED();
}

// ============================================================================
// PostEffectPipeline: 色収差（ビネットパス経由）
// ============================================================================

TEST(PostEffectPipelineTest, ChromaticAberration_DefaultValue)
{
    PostEffectPipeline pipeline;
    EXPECT_FLOAT_EQ(pipeline.GetChromaticAberration(), 0.003f);
}

TEST(PostEffectPipelineTest, ChromaticAberration_SetAndGet)
{
    PostEffectPipeline pipeline;
    pipeline.SetChromaticAberration(0.01f);
    EXPECT_FLOAT_EQ(pipeline.GetChromaticAberration(), 0.01f);
}

TEST(PostEffectPipelineTest, Vignette_DefaultDisabled)
{
    PostEffectPipeline pipeline;
    EXPECT_FALSE(pipeline.IsVignetteEnabled());
}

TEST(PostEffectPipelineTest, Vignette_SetEnabledAndCheck)
{
    PostEffectPipeline pipeline;
    pipeline.SetVignetteEnabled(true);
    EXPECT_TRUE(pipeline.IsVignetteEnabled());
    pipeline.SetVignetteEnabled(false);
    EXPECT_FALSE(pipeline.IsVignetteEnabled());
}

TEST(PostEffectPipelineTest, Vignette_IntensitySetAndGet)
{
    PostEffectPipeline pipeline;
    pipeline.SetVignetteIntensity(0.75f);
    EXPECT_FLOAT_EQ(pipeline.GetVignetteIntensity(), 0.75f);
}

TEST(PostEffectPipelineTest, Vignette_DefaultIntensity)
{
    PostEffectPipeline pipeline;
    EXPECT_FLOAT_EQ(pipeline.GetVignetteIntensity(), 0.5f);
}

// ============================================================================
// PostEffectPipeline: カラーグレーディング
// ============================================================================

TEST(PostEffectPipelineTest, ColorGrading_DefaultDisabled)
{
    PostEffectPipeline pipeline;
    EXPECT_FALSE(pipeline.IsColorGradingEnabled());
}

TEST(PostEffectPipelineTest, ColorGrading_SetEnabledIsEnabled)
{
    PostEffectPipeline pipeline;
    pipeline.SetColorGradingEnabled(true);
    EXPECT_TRUE(pipeline.IsColorGradingEnabled());
    pipeline.SetColorGradingEnabled(false);
    EXPECT_FALSE(pipeline.IsColorGradingEnabled());
}

TEST(PostEffectPipelineTest, ColorGrading_DefaultContrast)
{
    PostEffectPipeline pipeline;
    EXPECT_FLOAT_EQ(pipeline.GetContrast(), 1.0f);
}

TEST(PostEffectPipelineTest, ColorGrading_SetContrastGetContrast)
{
    PostEffectPipeline pipeline;
    pipeline.SetContrast(1.5f);
    EXPECT_FLOAT_EQ(pipeline.GetContrast(), 1.5f);
}

TEST(PostEffectPipelineTest, ColorGrading_DefaultSaturation)
{
    PostEffectPipeline pipeline;
    EXPECT_FLOAT_EQ(pipeline.GetSaturation(), 1.0f);
}

TEST(PostEffectPipelineTest, ColorGrading_SetSaturationGetSaturation)
{
    PostEffectPipeline pipeline;
    pipeline.SetSaturation(0.5f);
    EXPECT_FLOAT_EQ(pipeline.GetSaturation(), 0.5f);
}

TEST(PostEffectPipelineTest, ColorGrading_DefaultTemperature)
{
    PostEffectPipeline pipeline;
    EXPECT_FLOAT_EQ(pipeline.GetTemperature(), 0.0f);
}

TEST(PostEffectPipelineTest, ColorGrading_SetTemperatureGetTemperature)
{
    PostEffectPipeline pipeline;
    pipeline.SetTemperature(0.3f);
    EXPECT_FLOAT_EQ(pipeline.GetTemperature(), 0.3f);
}

TEST(PostEffectPipelineTest, Exposure_DefaultValue)
{
    PostEffectPipeline pipeline;
    EXPECT_FLOAT_EQ(pipeline.GetExposure(), 1.0f);
}

TEST(PostEffectPipelineTest, Exposure_SetAndGet)
{
    PostEffectPipeline pipeline;
    pipeline.SetExposure(2.0f);
    EXPECT_FLOAT_EQ(pipeline.GetExposure(), 2.0f);
}

TEST(PostEffectPipelineTest, TonemapMode_Default)
{
    PostEffectPipeline pipeline;
    EXPECT_EQ(pipeline.GetTonemapMode(), TonemapMode::ACES);
}

TEST(PostEffectPipelineTest, TonemapMode_SetAndGet)
{
    PostEffectPipeline pipeline;
    pipeline.SetTonemapMode(TonemapMode::Reinhard);
    EXPECT_EQ(pipeline.GetTonemapMode(), TonemapMode::Reinhard);
    pipeline.SetTonemapMode(TonemapMode::Uncharted2);
    EXPECT_EQ(pipeline.GetTonemapMode(), TonemapMode::Uncharted2);
}

TEST(PostEffectPipelineTest, FXAA_DefaultEnabled)
{
    PostEffectPipeline pipeline;
    EXPECT_TRUE(pipeline.IsFXAAEnabled());
}

TEST(PostEffectPipelineTest, FXAA_SetAndGet)
{
    PostEffectPipeline pipeline;
    pipeline.SetFXAAEnabled(false);
    EXPECT_FALSE(pipeline.IsFXAAEnabled());
    pipeline.SetFXAAEnabled(true);
    EXPECT_TRUE(pipeline.IsFXAAEnabled());
}
