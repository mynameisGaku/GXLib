/// @file test_LightProbe.cpp
/// @brief LightProbeSystemのテスト — 設定、プローブ数、有効/無効、パラメータ

#include <gtest/gtest.h>
#include "Graphics/3D/LightProbeSystem.h"

using namespace gx;

// ============================================================================
// デフォルト状態
// ============================================================================

TEST(LightProbeSystemTest, DefaultState)
{
    LightProbeSystem probes;
    EXPECT_FALSE(probes.IsEnabled());
    EXPECT_EQ(probes.GetRaysPerProbe(), 64);
    EXPECT_FLOAT_EQ(probes.GetHysteresis(), 0.97f);
}

TEST(LightProbeSystemTest, EnableDisable)
{
    LightProbeSystem probes;
    probes.SetEnabled(true);
    EXPECT_TRUE(probes.IsEnabled());
    probes.SetEnabled(false);
    EXPECT_FALSE(probes.IsEnabled());
}

// ============================================================================
// 設定
// ============================================================================

TEST(LightProbeSystemTest, DefaultProbeCount)
{
    LightProbeSystem probes;
    // デフォルト設定: 8 x 4 x 8 = 256 プローブ
    EXPECT_EQ(probes.GetProbeCount(), 256u);
}

TEST(LightProbeSystemTest, CustomConfig)
{
    LightProbeSystem probes;
    ProbeGridConfig config;
    config.dimensions = { 10, 5, 10 };
    config.spacing = { 2.0f, 2.0f, 2.0f };
    config.origin = { -10.0f, 0.0f, -10.0f };
    probes.SetConfig(config);

    EXPECT_EQ(probes.GetProbeCount(), 500u); // 10 * 5 * 10

    const auto& c = probes.GetConfig();
    EXPECT_FLOAT_EQ(c.origin.x, -10.0f);
    EXPECT_FLOAT_EQ(c.spacing.x, 2.0f);
    EXPECT_EQ(c.dimensions.x, 10u);
}

// ============================================================================
// 更新パラメータ
// ============================================================================

TEST(LightProbeSystemTest, RaysPerProbe)
{
    LightProbeSystem probes;
    probes.SetRaysPerProbe(128);
    EXPECT_EQ(probes.GetRaysPerProbe(), 128);
}

TEST(LightProbeSystemTest, Hysteresis)
{
    LightProbeSystem probes;
    probes.SetHysteresis(0.9f);
    EXPECT_FLOAT_EQ(probes.GetHysteresis(), 0.9f);
}

// ============================================================================
// デバイスなしで初期化
// ============================================================================

TEST(LightProbeSystemTest, InitializeNullDevice)
{
    LightProbeSystem probes;
    ProbeGridConfig config;
    config.dimensions = { 4, 2, 4 };
    bool result = probes.Initialize(nullptr, config);
    EXPECT_FALSE(result);
}

// ============================================================================
// 定数バッファサイズ
// ============================================================================

TEST(LightProbeSystemTest, ConstantBufferSize)
{
    EXPECT_EQ(sizeof(LightProbeUpdateConstants), 64u);
}

// ============================================================================
// シャットダウンの安全性
// ============================================================================

TEST(LightProbeSystemTest, ShutdownWithoutInit)
{
    LightProbeSystem probes;
    probes.Shutdown();
    EXPECT_FALSE(probes.IsEnabled());
    EXPECT_EQ(probes.GetIrradianceAtlas(), nullptr);
    EXPECT_EQ(probes.GetDepthAtlas(), nullptr);
}
