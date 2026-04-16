/// @file test_RTSoftShadows.cpp
/// @brief RTSoftShadows 設定パラメータのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/RayTracing/RTSoftShadows.h"

using namespace gx;

// ==================== デフォルト設定 ====================

TEST(RTSoftShadowsTest, DefaultSettings_Disabled)
{
    RTSoftShadowSettings settings;
    EXPECT_FALSE(settings.enabled);
}

TEST(RTSoftShadowsTest, DefaultSettings_LightRadius)
{
    RTSoftShadowSettings settings;
    EXPECT_FLOAT_EQ(settings.lightRadius, 0.5f);
}

TEST(RTSoftShadowsTest, DefaultSettings_NumRays)
{
    RTSoftShadowSettings settings;
    EXPECT_EQ(settings.numRays, 8);
}

TEST(RTSoftShadowsTest, DefaultSettings_TemporalBlend)
{
    RTSoftShadowSettings settings;
    EXPECT_FLOAT_EQ(settings.temporalBlend, 0.9f);
}

TEST(RTSoftShadowsTest, DefaultSettings_MaxDistance)
{
    RTSoftShadowSettings settings;
    EXPECT_FLOAT_EQ(settings.maxDistance, 100.0f);
}

// ==================== RTSoftShadows クラス ====================

TEST(RTSoftShadowsTest, IsEnabled_Default)
{
    RTSoftShadows shadows;
    EXPECT_FALSE(shadows.IsEnabled());
}

TEST(RTSoftShadowsTest, SetSettings_RoundTrip)
{
    RTSoftShadows shadows;
    RTSoftShadowSettings settings;
    settings.enabled       = true;
    settings.lightRadius   = 1.5f;
    settings.numRays       = 16;
    settings.temporalBlend = 0.85f;
    settings.maxDistance    = 200.0f;

    shadows.SetSettings(settings);
    const auto& result = shadows.GetSettings();

    EXPECT_TRUE(result.enabled);
    EXPECT_FLOAT_EQ(result.lightRadius, 1.5f);
    EXPECT_EQ(result.numRays, 16);
    EXPECT_FLOAT_EQ(result.temporalBlend, 0.85f);
    EXPECT_FLOAT_EQ(result.maxDistance, 200.0f);
}

TEST(RTSoftShadowsTest, TLAS_DefaultNull)
{
    RTSoftShadows shadows;
    // TLAS is not set by default - Execute should early-out
    // We verify the class can be constructed and settings modified without TLAS
    RTSoftShadowSettings settings;
    settings.enabled = true;
    shadows.SetSettings(settings);
    EXPECT_TRUE(shadows.IsEnabled());
}

TEST(RTSoftShadowsTest, IsEnabled_AfterSetSettings)
{
    RTSoftShadows shadows;
    RTSoftShadowSettings settings;
    settings.enabled = true;
    shadows.SetSettings(settings);
    EXPECT_TRUE(shadows.IsEnabled());

    settings.enabled = false;
    shadows.SetSettings(settings);
    EXPECT_FALSE(shadows.IsEnabled());
}

TEST(RTSoftShadowsTest, ParameterRanges)
{
    RTSoftShadowSettings settings;
    // 極端な値でもクラッシュしないことを確認
    settings.lightRadius   = 0.0f;
    settings.numRays       = 1;
    settings.temporalBlend = 0.0f;
    settings.maxDistance    = 0.1f;

    RTSoftShadows shadows;
    shadows.SetSettings(settings);
    const auto& result = shadows.GetSettings();
    EXPECT_FLOAT_EQ(result.lightRadius, 0.0f);
    EXPECT_EQ(result.numRays, 1);
    EXPECT_FLOAT_EQ(result.temporalBlend, 0.0f);
    EXPECT_FLOAT_EQ(result.maxDistance, 0.1f);
}
