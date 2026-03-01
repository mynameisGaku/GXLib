/// @file test_SSGI.cpp
/// @brief SSGI (Screen-Space Global Illumination) のCPU側パラメータテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/PostEffect/SSGI.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// DefaultState: 初期状態で無効
// ============================================================================
TEST(SSGITest, DefaultState)
{
    SSGI ssgi;
    EXPECT_FALSE(ssgi.IsEnabled());
    EXPECT_EQ(ssgi.GetSampleCount(), 32);
    EXPECT_NEAR(ssgi.GetIntensity(), 0.5f, kEps);
    EXPECT_NEAR(ssgi.GetMaxDistance(), 50.0f, kEps);
}

// ============================================================================
// EnableDisable: 有効/無効切り替え
// ============================================================================
TEST(SSGITest, EnableDisable)
{
    SSGI ssgi;
    ssgi.SetEnabled(true);
    EXPECT_TRUE(ssgi.IsEnabled());
    ssgi.SetEnabled(false);
    EXPECT_FALSE(ssgi.IsEnabled());
}

// ============================================================================
// SetIntensity: 強度パラメータ設定
// ============================================================================
TEST(SSGITest, SetIntensity)
{
    SSGI ssgi;
    ssgi.SetIntensity(1.2f);
    EXPECT_NEAR(ssgi.GetIntensity(), 1.2f, kEps);
}

// ============================================================================
// SetSampleCount: サンプル数設定
// ============================================================================
TEST(SSGITest, SetSampleCount)
{
    SSGI ssgi;
    ssgi.SetSampleCount(64);
    EXPECT_EQ(ssgi.GetSampleCount(), 64);
}

// ============================================================================
// SetMaxDistance: 最大距離設定
// ============================================================================
TEST(SSGITest, SetMaxDistance)
{
    SSGI ssgi;
    ssgi.SetMaxDistance(100.0f);
    EXPECT_NEAR(ssgi.GetMaxDistance(), 100.0f, kEps);
}

// ============================================================================
// InitializeNullDevice: nullptrデバイスでfalse返却
// ============================================================================
TEST(SSGITest, InitializeNullDevice)
{
    SSGI ssgi;
    bool result = ssgi.Initialize(nullptr, 1920, 1080);
    EXPECT_FALSE(result);
}

// ============================================================================
// ParameterRange: パラメータの有効範囲テスト
// ============================================================================
TEST(SSGITest, ParameterRange)
{
    SSGI ssgi;
    // 正常値
    ssgi.SetIntensity(1.0f);
    EXPECT_NEAR(ssgi.GetIntensity(), 1.0f, kEps);

    ssgi.SetSampleCount(16);
    EXPECT_EQ(ssgi.GetSampleCount(), 16);

    ssgi.SetMaxDistance(75.0f);
    EXPECT_NEAR(ssgi.GetMaxDistance(), 75.0f, kEps);
}

// ============================================================================
// DefaultParameters: デフォルト値の確認
// ============================================================================
TEST(SSGITest, DefaultParameters)
{
    SSGI ssgi;
    EXPECT_NEAR(ssgi.GetIntensity(), 0.5f, kEps);
    EXPECT_EQ(ssgi.GetSampleCount(), 32);
    EXPECT_NEAR(ssgi.GetMaxDistance(), 50.0f, kEps);
    EXPECT_NEAR(ssgi.GetThickness(), 0.5f, kEps);
}

// ============================================================================
// IntensityClamp: 強度のクランプ動作
// ============================================================================
TEST(SSGITest, IntensityClamp)
{
    SSGI ssgi;

    // 上限クランプ
    ssgi.SetIntensity(5.0f);
    EXPECT_NEAR(ssgi.GetIntensity(), 2.0f, kEps);

    // 下限クランプ
    ssgi.SetIntensity(-1.0f);
    EXPECT_NEAR(ssgi.GetIntensity(), 0.0f, kEps);
}

// ============================================================================
// SampleCountClamp: サンプル数のクランプ動作
// ============================================================================
TEST(SSGITest, SampleCountClamp)
{
    SSGI ssgi;

    // 上限クランプ
    ssgi.SetSampleCount(500);
    EXPECT_EQ(ssgi.GetSampleCount(), 128);

    // 下限クランプ
    ssgi.SetSampleCount(1);
    EXPECT_EQ(ssgi.GetSampleCount(), 4);
}
