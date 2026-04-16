/// @file test_ClusteredLighting.cpp
/// @brief ClusteredLighting テスト（構造体・設定値の検証）

#include <gtest/gtest.h>
#include "Graphics/3D/ClusteredLighting.h"

using namespace gx;

// -----------------------------------------------------------------------
// Default Settings
// -----------------------------------------------------------------------
TEST(ClusteredLightingTest, DefaultSettingsDisabled)
{
    ClusteredLighting cl;
    EXPECT_FALSE(cl.IsEnabled());
}

TEST(ClusteredLightingTest, DefaultClusterDimensions)
{
    ClusteredLighting cl;
    uint32_t x = 0, y = 0, z = 0;
    cl.GetClusterDimensions(x, y, z);
    EXPECT_EQ(x, 16u);
    EXPECT_EQ(y, 8u);
    EXPECT_EQ(z, 24u);
}

TEST(ClusteredLightingTest, TotalClusters)
{
    ClusteredLighting cl;
    EXPECT_EQ(cl.GetTotalClusters(), 16u * 8u * 24u); // 3072
}

TEST(ClusteredLightingTest, MaxLightsIs256)
{
    ClusteredLighting cl;
    EXPECT_EQ(cl.GetMaxLights(), 256u);
}

// -----------------------------------------------------------------------
// Set / Get Settings
// -----------------------------------------------------------------------
TEST(ClusteredLightingTest, SetSettingsRoundTrip)
{
    ClusteredLighting cl;
    ClusteredLightingSettings s;
    s.enabled      = true;
    s.clusterCountX = 32;
    s.clusterCountY = 16;
    s.clusterCountZ = 48;
    cl.SetSettings(s);

    const auto& result = cl.GetSettings();
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.clusterCountX, 32u);
    EXPECT_EQ(result.clusterCountY, 16u);
    EXPECT_EQ(result.clusterCountZ, 48u);
}

TEST(ClusteredLightingTest, EnableDisableToggle)
{
    ClusteredLighting cl;
    EXPECT_FALSE(cl.IsEnabled());

    ClusteredLightingSettings s = cl.GetSettings();
    s.enabled = true;
    cl.SetSettings(s);
    EXPECT_TRUE(cl.IsEnabled());

    s.enabled = false;
    cl.SetSettings(s);
    EXPECT_FALSE(cl.IsEnabled());
}

TEST(ClusteredLightingTest, TotalClustersUpdatesWithSettings)
{
    ClusteredLighting cl;
    ClusteredLightingSettings s;
    s.clusterCountX = 8;
    s.clusterCountY = 4;
    s.clusterCountZ = 12;
    cl.SetSettings(s);
    EXPECT_EQ(cl.GetTotalClusters(), 8u * 4u * 12u); // 384
}

// -----------------------------------------------------------------------
// ClusterInfo struct
// -----------------------------------------------------------------------
TEST(ClusteredLightingTest, ClusterInfoDefaults)
{
    ClusterInfo info;
    EXPECT_EQ(info.offset, 0u);
    EXPECT_EQ(info.count, 0u);
}

TEST(ClusteredLightingTest, ClusterInfoAssignment)
{
    ClusterInfo info;
    info.offset = 128;
    info.count  = 7;
    EXPECT_EQ(info.offset, 128u);
    EXPECT_EQ(info.count, 7u);
}
