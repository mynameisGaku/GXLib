/// @file test_SkyWater.cpp
/// @brief SkyAtmosphereとWaterRendererのテスト — パラメータ管理、ComputeSunColor

#include <gtest/gtest.h>
#include "Graphics/3D/SkyAtmosphere.h"
#include "Graphics/3D/WaterRenderer.h"
#include <cmath>

using namespace gx;

// ============================================================================
// SkyAtmosphereパラメータ
// ============================================================================

TEST(SkyAtmosphereTest, DefaultState)
{
    SkyAtmosphere sky;
    EXPECT_FALSE(sky.IsEnabled());
    EXPECT_EQ(sky.GetNumSteps(), 8);
    EXPECT_FLOAT_EQ(sky.GetSunIntensity(), 20.0f);
}

TEST(SkyAtmosphereTest, EnableDisable)
{
    SkyAtmosphere sky;
    sky.SetEnabled(true);
    EXPECT_TRUE(sky.IsEnabled());
    sky.SetEnabled(false);
    EXPECT_FALSE(sky.IsEnabled());
}

TEST(SkyAtmosphereTest, SunParameters)
{
    SkyAtmosphere sky;
    sky.SetSunDirection({ 0.5f, 0.5f, 0.0f });
    EXPECT_FLOAT_EQ(sky.GetSunDirection().x, 0.5f);
    EXPECT_FLOAT_EQ(sky.GetSunDirection().y, 0.5f);

    sky.SetSunIntensity(10.0f);
    EXPECT_FLOAT_EQ(sky.GetSunIntensity(), 10.0f);
}

TEST(SkyAtmosphereTest, AtmosphereParameters)
{
    SkyAtmosphere sky;
    sky.SetPlanetRadius(6000000.0f);
    EXPECT_FLOAT_EQ(sky.GetPlanetRadius(), 6000000.0f);

    sky.SetAtmosphereRadius(6100000.0f);
    EXPECT_FLOAT_EQ(sky.GetAtmosphereRadius(), 6100000.0f);
}

TEST(SkyAtmosphereTest, QualitySettings)
{
    SkyAtmosphere sky;
    sky.SetNumSteps(16);
    EXPECT_EQ(sky.GetNumSteps(), 16);
}

TEST(SkyAtmosphereTest, ScatteringParameters)
{
    SkyAtmosphere sky;
    sky.SetRayleighCoefficients({ 1e-5f, 2e-5f, 3e-5f });
    sky.SetRayleighScaleHeight(7000.0f);
    sky.SetMieCoefficient(2e-5f);
    sky.SetMieScaleHeight(1000.0f);
    sky.SetMieG(0.8f);
    // クラッシュしないことを確認 — 値は内部に格納される
    EXPECT_FALSE(sky.IsEnabled()); // まだ無効
}

// ============================================================================
// ComputeSunColor
// ============================================================================

TEST(SkyAtmosphereTest, ComputeSunColorZenith)
{
    // 天頂（PI/2）では太陽は明るく白っぽいはず
    XMFLOAT3 c = SkyAtmosphere::ComputeSunColor(3.14159265f / 2.0f);
    EXPECT_GT(c.x, 0.5f);
    EXPECT_GT(c.y, 0.5f);
    EXPECT_GT(c.z, 0.5f);
}

TEST(SkyAtmosphereTest, ComputeSunColorHorizon)
{
    // 地平線（0）では太陽はより赤みがかった暖色になるはず
    XMFLOAT3 c = SkyAtmosphere::ComputeSunColor(0.0f);
    // 赤チャンネルは緑・青チャンネル以上であるはず（夕焼けの赤み）
    EXPECT_GE(c.x, c.z);
}

TEST(SkyAtmosphereTest, ComputeSunColorBelowHorizon)
{
    // 地平線より下（負の値）でも有効な色を返すはず（クランプ済み）
    XMFLOAT3 c = SkyAtmosphere::ComputeSunColor(-0.5f);
    EXPECT_GE(c.x, 0.0f);
    EXPECT_GE(c.y, 0.0f);
    EXPECT_GE(c.z, 0.0f);
}

// ============================================================================
// WaterRendererパラメータ
// ============================================================================

TEST(WaterRendererTest, DefaultState)
{
    WaterRenderer water;
    EXPECT_FALSE(water.IsEnabled());
    EXPECT_FLOAT_EQ(water.GetWaterLevel(), 0.0f);
    EXPECT_FLOAT_EQ(water.GetPlaneSize(), 512.0f);
    EXPECT_FLOAT_EQ(water.GetWaveAmplitude(), 0.5f);
    EXPECT_EQ(water.GetGridResolution(), 128);
}

TEST(WaterRendererTest, EnableDisable)
{
    WaterRenderer water;
    water.SetEnabled(true);
    EXPECT_TRUE(water.IsEnabled());
    water.SetEnabled(false);
    EXPECT_FALSE(water.IsEnabled());
}

TEST(WaterRendererTest, WaterParameters)
{
    WaterRenderer water;
    water.SetWaterLevel(10.0f);
    EXPECT_FLOAT_EQ(water.GetWaterLevel(), 10.0f);

    water.SetPlaneSize(256.0f);
    EXPECT_FLOAT_EQ(water.GetPlaneSize(), 256.0f);

    water.SetWaveAmplitude(1.0f);
    EXPECT_FLOAT_EQ(water.GetWaveAmplitude(), 1.0f);

    water.SetWaveFrequency(0.5f);
    EXPECT_FLOAT_EQ(water.GetWaveFrequency(), 0.5f);
}

TEST(WaterRendererTest, WindDirection)
{
    WaterRenderer water;
    water.SetWindDirection({ 0.0f, 1.0f });
    EXPECT_FLOAT_EQ(water.GetWindDirection().x, 0.0f);
    EXPECT_FLOAT_EQ(water.GetWindDirection().y, 1.0f);
}

TEST(WaterRendererTest, WaterColor)
{
    WaterRenderer water;
    water.SetWaterColor({ 0.1f, 0.4f, 0.6f, 0.9f });
    const auto& c = water.GetWaterColor();
    EXPECT_FLOAT_EQ(c.x, 0.1f);
    EXPECT_FLOAT_EQ(c.y, 0.4f);
    EXPECT_FLOAT_EQ(c.z, 0.6f);
    EXPECT_FLOAT_EQ(c.w, 0.9f);
}

TEST(WaterRendererTest, FresnelAndSpecular)
{
    WaterRenderer water;
    water.SetFresnelBias(0.05f);
    water.SetFresnelPower(3.0f);
    water.SetSpecularPower(128.0f);
    // クラッシュしないことを確認 — 内部状態のみ
    EXPECT_FALSE(water.IsEnabled());
}

// ============================================================================
// 定数バッファサイズ検証
// ============================================================================

TEST(SkyAtmosphereTest, ConstantBufferSize)
{
    // SkyAtmosphereConstantsは160バイトであるべき
    EXPECT_EQ(sizeof(SkyAtmosphereConstants), 160u);
}

TEST(WaterRendererTest, ConstantBufferSize)
{
    // WaterConstantsは224バイトであるべき
    EXPECT_EQ(sizeof(WaterConstants), 224u);
}

TEST(WaterRendererTest, FFTConstantBufferSize)
{
    // WaterFFTConstantsは32バイトであるべき
    EXPECT_EQ(sizeof(WaterFFTConstants), 32u);
}
