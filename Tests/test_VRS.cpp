/// @file test_VRS.cpp
/// @brief VRSManagerとQualitySettingsの単体テスト（GPU不要）

#include <gtest/gtest.h>
#include "Graphics/Device/VRSManager.h"
#include "Graphics/QualitySettings.h"

using namespace gx;

// ============================================================================
// VRSTier / VRSMode列挙型テスト
// ============================================================================

TEST(VRSManager, VRSTierEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(VRSTier::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(VRSTier::Tier1), 1u);
    EXPECT_EQ(static_cast<uint32_t>(VRSTier::Tier2), 2u);
}

TEST(VRSManager, VRSModeEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(VRSMode::Off), 0u);
    EXPECT_EQ(static_cast<uint32_t>(VRSMode::PerDraw), 1u);
    EXPECT_EQ(static_cast<uint32_t>(VRSMode::Image), 2u);
}

// ============================================================================
// VRSManagerデフォルト状態テスト
// ============================================================================

TEST(VRSManager, DefaultTierIsNone)
{
    VRSManager mgr;
    EXPECT_EQ(mgr.GetSupportedTier(), VRSTier::None);
}

TEST(VRSManager, NoShadingRateImageByDefault)
{
    VRSManager mgr;
    EXPECT_EQ(mgr.GetShadingRateImage(), nullptr);
}

TEST(VRSManager, InitializeWithoutDeviceReturnsFalse)
{
    VRSManager mgr;
    EXPECT_FALSE(mgr.Initialize(nullptr, 1920, 1080));
}

TEST(VRSManager, OnResizeWithoutInit)
{
    VRSManager mgr;
    // TierがNoneなのでOnResizeはデバイスに触れずに早期リターン
    mgr.OnResize(nullptr, 1920, 1080);
    EXPECT_EQ(mgr.GetShadingRateImage(), nullptr);
}

TEST(VRSManager, TierRemainsNoneAfterNullInit)
{
    VRSManager mgr;
    mgr.Initialize(nullptr, 800, 600);
    EXPECT_EQ(mgr.GetSupportedTier(), VRSTier::None);
}

// ============================================================================
// QualityParams VRSフィールドテスト
// ============================================================================

TEST(QualitySettings, VRSModeDefaultIsOff)
{
    QualityParams params;
    EXPECT_EQ(params.vrsMode, 0u);
}

TEST(QualitySettings, VRSModeCustomImageMode)
{
    QualityParams params;
    params.vrsMode = 2; // イメージモード
    EXPECT_EQ(params.vrsMode, 2u);
}
