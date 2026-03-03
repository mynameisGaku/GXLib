/// @file test_LensFlare.cpp
/// @brief LensFlare (物理ベース光束追跡) 設定パラメータ・CPU初期化テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/PostEffect/LensFlare.h"

using namespace gx;

// ==================== デフォルト設定 ====================

TEST(LensFlareTest, DefaultSettings_Disabled)
{
    LensFlareSettings settings;
    EXPECT_FALSE(settings.enabled);
}

TEST(LensFlareTest, DefaultSettings_Intensity)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.intensity, 1.0f);
}

TEST(LensFlareTest, DefaultSettings_Threshold)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.threshold, 1.5f);
}

TEST(LensFlareTest, DefaultSettings_GridSize)
{
    LensFlareSettings settings;
    EXPECT_EQ(settings.gridSize, 32);
}

TEST(LensFlareTest, DefaultSettings_ApertureBlades)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.apertureBlades, 6.0f);
}

TEST(LensFlareTest, DefaultSettings_StarburstStrength)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.starburstStrength, 0.3f);
}

// ==================== LensFlare クラス ====================

TEST(LensFlareTest, IsEnabled_Default)
{
    LensFlare lensFlare;
    EXPECT_FALSE(lensFlare.IsEnabled());
}

TEST(LensFlareTest, SetSettings_RoundTrip)
{
    LensFlare lensFlare;
    LensFlareSettings settings;
    settings.enabled           = true;
    settings.intensity         = 2.0f;
    settings.threshold         = 2.5f;
    settings.gridSize          = 64;
    settings.apertureBlades    = 8.0f;
    settings.starburstStrength = 0.5f;

    lensFlare.SetSettings(settings);
    const auto& result = lensFlare.GetSettings();

    EXPECT_TRUE(result.enabled);
    EXPECT_FLOAT_EQ(result.intensity, 2.0f);
    EXPECT_FLOAT_EQ(result.threshold, 2.5f);
    EXPECT_EQ(result.gridSize, 64);
    EXPECT_FLOAT_EQ(result.apertureBlades, 8.0f);
    EXPECT_FLOAT_EQ(result.starburstStrength, 0.5f);
}

TEST(LensFlareTest, IsEnabled_AfterSetSettings)
{
    LensFlare lensFlare;
    LensFlareSettings settings;
    settings.enabled = true;
    lensFlare.SetSettings(settings);
    EXPECT_TRUE(lensFlare.IsEnabled());
}

TEST(LensFlareTest, ParameterRanges)
{
    LensFlareSettings settings;
    settings.intensity         = 0.0f;
    settings.threshold         = 0.0f;
    settings.gridSize          = 16;
    settings.apertureBlades    = 3.0f;
    settings.starburstStrength = 0.0f;

    LensFlare lensFlare;
    lensFlare.SetSettings(settings);
    const auto& result = lensFlare.GetSettings();
    EXPECT_FLOAT_EQ(result.intensity, 0.0f);
    EXPECT_FLOAT_EQ(result.threshold, 0.0f);
    EXPECT_EQ(result.gridSize, 16);
}

TEST(LensFlareTest, SetEnabled_Toggle)
{
    LensFlare lensFlare;
    EXPECT_FALSE(lensFlare.IsEnabled());
    lensFlare.SetEnabled(true);
    EXPECT_TRUE(lensFlare.IsEnabled());
    lensFlare.SetEnabled(false);
    EXPECT_FALSE(lensFlare.IsEnabled());
}

TEST(LensFlareTest, SetIntensity)
{
    LensFlare lensFlare;
    lensFlare.SetIntensity(3.5f);
    EXPECT_FLOAT_EQ(lensFlare.GetIntensity(), 3.5f);
}

TEST(LensFlareTest, SetThreshold)
{
    LensFlare lensFlare;
    lensFlare.SetThreshold(2.0f);
    EXPECT_FLOAT_EQ(lensFlare.GetThreshold(), 2.0f);
}

TEST(LensFlareTest, SetLightDirection)
{
    LensFlare lensFlare;
    Vector3 dir = { 0.5f, -1.0f, 0.3f };
    lensFlare.SetLightDirection(dir);
    const auto& result = lensFlare.GetLightDirection();
    EXPECT_FLOAT_EQ(result.x, 0.5f);
    EXPECT_FLOAT_EQ(result.y, -1.0f);
    EXPECT_FLOAT_EQ(result.z, 0.3f);
}

TEST(LensFlareTest, SetLightColor)
{
    LensFlare lensFlare;
    Vector3 color = { 1.0f, 0.9f, 0.8f };
    lensFlare.SetLightColor(color);
    const auto& result = lensFlare.GetLightColor();
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 0.9f);
    EXPECT_FLOAT_EQ(result.z, 0.8f);
}

TEST(LensFlareTest, Initialize_CPUOnly)
{
    LensFlare lensFlare;
    bool ok = lensFlare.Initialize(nullptr, 1920, 1080);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(lensFlare.IsEnabled()); // 初期化後もデフォルトはdisabled
}
