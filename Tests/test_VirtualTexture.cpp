/// @file test_VirtualTexture.cpp
/// @brief Virtual Texturing system tests

#include <gtest/gtest.h>
#include "Graphics/Resource/VirtualTexture.h"

using namespace gx;

TEST(VirtualTextureTest, DefaultState)
{
    VirtualTexture vt;
    EXPECT_EQ(vt.GetCachedTileCount(), 0u);
    EXPECT_EQ(vt.GetPendingRequestCount(), 0u);
}

TEST(VirtualTextureTest, InitializeDefault)
{
    VirtualTexture vt;
    EXPECT_TRUE(vt.Initialize(nullptr));
    EXPECT_EQ(vt.GetConfig().pageSize, 128u);
    EXPECT_EQ(vt.GetConfig().poolWidth, 2048u);
}

TEST(VirtualTextureTest, InitializeInvalidConfig)
{
    VirtualTexture vt;
    VirtualTextureConfig cfg;
    cfg.pageSize = 0;
    EXPECT_FALSE(vt.Initialize(nullptr, cfg));
}

TEST(VirtualTextureTest, PoolTileCapacity)
{
    VirtualTexture vt;
    vt.Initialize(nullptr);
    // 2048/128 = 16 tiles per row, 16 rows = 256
    EXPECT_EQ(vt.GetPoolTileCapacity(), 256u);
}

TEST(VirtualTextureTest, AnalyzeFeedbackNewPages)
{
    VirtualTexture vt;
    vt.Initialize(nullptr);

    std::vector<VTPageId> pages = {
        { 0, 0, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
    vt.AnalyzeFeedback(pages, 1);
    EXPECT_EQ(vt.GetPendingRequestCount(), 3u);
}

TEST(VirtualTextureTest, UpdateTileCacheLoadsPages)
{
    VirtualTexture vt;
    vt.Initialize(nullptr);

    std::vector<VTPageId> pages = {
        { 0, 0, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
    vt.AnalyzeFeedback(pages, 1);
    uint32_t loaded = vt.UpdateTileCache();
    EXPECT_EQ(loaded, 3u);
    EXPECT_EQ(vt.GetCachedTileCount(), 3u);
}

TEST(VirtualTextureTest, MaxTilesPerFrame)
{
    VirtualTexture vt;
    VirtualTextureConfig cfg;
    cfg.maxTilesPerFrame = 2;
    vt.Initialize(nullptr, cfg);

    std::vector<VTPageId> pages;
    for (uint32_t i = 0; i < 5; i++)
        pages.push_back({ 0, 0, i, 0 });

    vt.AnalyzeFeedback(pages, 1);
    uint32_t loaded = vt.UpdateTileCache();
    EXPECT_EQ(loaded, 2u);
    EXPECT_EQ(vt.GetCachedTileCount(), 2u);
    EXPECT_EQ(vt.GetPendingRequestCount(), 3u);
}

TEST(VirtualTextureTest, TileCacheLookup)
{
    VirtualTexture vt;
    vt.Initialize(nullptr);

    VTPageId page = { 1, 2, 3, 4 };
    std::vector<VTPageId> pages = { page };
    vt.AnalyzeFeedback(pages, 1);
    vt.UpdateTileCache();

    EXPECT_TRUE(vt.IsTileCached(page));
    EXPECT_FALSE(vt.IsTileCached({ 9, 9, 9, 9 }));
}

TEST(VirtualTextureTest, ClearCache)
{
    VirtualTexture vt;
    vt.Initialize(nullptr);

    std::vector<VTPageId> pages = { { 0, 0, 0, 0 } };
    vt.AnalyzeFeedback(pages, 1);
    vt.UpdateTileCache();
    EXPECT_EQ(vt.GetCachedTileCount(), 1u);

    vt.ClearCache();
    EXPECT_EQ(vt.GetCachedTileCount(), 0u);
}
