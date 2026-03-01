/// @file test_LensFlare.cpp
/// @brief LensFlare 設定パラメータのテスト

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

TEST(LensFlareTest, DefaultSettings_GhostCount)
{
    LensFlareSettings settings;
    EXPECT_EQ(settings.ghostCount, 4);
}

TEST(LensFlareTest, DefaultSettings_GhostDispersion)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.ghostDispersion, 0.5f);
}

TEST(LensFlareTest, DefaultSettings_HaloRadius)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.haloRadius, 0.6f);
}

TEST(LensFlareTest, DefaultSettings_ChromaticShift)
{
    LensFlareSettings settings;
    EXPECT_FLOAT_EQ(settings.chromaticShift, 0.01f);
}

TEST(LensFlareTest, DefaultSettings_DustDisabled)
{
    LensFlareSettings settings;
    EXPECT_FALSE(settings.dustEnabled);
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
    settings.enabled         = true;
    settings.intensity       = 2.0f;
    settings.threshold       = 2.5f;
    settings.ghostCount      = 8;
    settings.ghostDispersion = 0.8f;
    settings.haloRadius      = 0.9f;
    settings.chromaticShift  = 0.05f;
    settings.dustEnabled     = true;

    lensFlare.SetSettings(settings);
    const auto& result = lensFlare.GetSettings();

    EXPECT_TRUE(result.enabled);
    EXPECT_FLOAT_EQ(result.intensity, 2.0f);
    EXPECT_FLOAT_EQ(result.threshold, 2.5f);
    EXPECT_EQ(result.ghostCount, 8);
    EXPECT_FLOAT_EQ(result.ghostDispersion, 0.8f);
    EXPECT_FLOAT_EQ(result.haloRadius, 0.9f);
    EXPECT_FLOAT_EQ(result.chromaticShift, 0.05f);
    EXPECT_TRUE(result.dustEnabled);
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
    // 極端な値でもクラッシュしないことを確認
    settings.intensity       = 0.0f;
    settings.threshold       = 0.0f;
    settings.ghostCount      = 0;
    settings.ghostDispersion = 0.0f;
    settings.haloRadius      = 0.0f;
    settings.chromaticShift  = 0.0f;

    LensFlare lensFlare;
    lensFlare.SetSettings(settings);
    const auto& result = lensFlare.GetSettings();
    EXPECT_FLOAT_EQ(result.intensity, 0.0f);
    EXPECT_FLOAT_EQ(result.threshold, 0.0f);
    EXPECT_EQ(result.ghostCount, 0);
}
