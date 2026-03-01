/// @file test_Tilemap.cpp
/// @brief Tilemap Create/GetTile/SetTile/WorldToTile/TileToWorldのテスト（GPU不要）

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/Rendering/Tilemap.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

static Tilemap MakeTestMap(int w = 10, int h = 8, int tw = 32, int th = 32)
{
    Tilemap map;
    map.Create(w, h, tw, th);

    TilemapLayer layer;
    layer.name = "Ground";
    layer.width = w;
    layer.height = h;
    layer.tiles.resize(w * h, 0);
    layer.visible = true;
    layer.opacity = 1.0f;
    map.AddLayer(layer);

    return map;
}

// ==================== Create ====================

TEST(TilemapTest, Create_Dimensions)
{
    Tilemap map;
    map.Create(20, 15, 16, 16);
    EXPECT_EQ(map.GetWidth(), 20);
    EXPECT_EQ(map.GetHeight(), 15);
    EXPECT_EQ(map.GetTileWidth(), 16);
    EXPECT_EQ(map.GetTileHeight(), 16);
}

TEST(TilemapTest, Create_PixelSize)
{
    Tilemap map;
    map.Create(10, 8, 32, 32);
    EXPECT_EQ(map.GetPixelWidth(), 320);
    EXPECT_EQ(map.GetPixelHeight(), 256);
}

TEST(TilemapTest, Create_NoLayers)
{
    Tilemap map;
    map.Create(5, 5, 16, 16);
    EXPECT_EQ(map.GetLayerCount(), 0);
}

// ==================== レイヤー ====================

TEST(TilemapTest, AddLayer_Single)
{
    Tilemap map = MakeTestMap();
    EXPECT_EQ(map.GetLayerCount(), 1);
}

TEST(TilemapTest, AddLayer_Multiple)
{
    Tilemap map = MakeTestMap();
    TilemapLayer layer2;
    layer2.name = "Overlay";
    layer2.width = 10;
    layer2.height = 8;
    layer2.tiles.resize(80, 0);
    layer2.visible = true;
    layer2.opacity = 0.5f;
    map.AddLayer(layer2);
    EXPECT_EQ(map.GetLayerCount(), 2);
}

TEST(TilemapTest, GetLayer_Name)
{
    Tilemap map = MakeTestMap();
    const auto& layer = map.GetLayer(0);
    EXPECT_EQ(layer.name, "Ground");
}

TEST(TilemapTest, GetLayer_MutableAccess)
{
    Tilemap map = MakeTestMap();
    auto& layer = map.GetLayer(0);
    layer.visible = false;
    EXPECT_FALSE(map.GetLayer(0).visible);
}

// ==================== GetTile / SetTile ====================

TEST(TilemapTest, GetTile_Default)
{
    Tilemap map = MakeTestMap();
    EXPECT_EQ(map.GetTile(0, 0, 0), 0);
    EXPECT_EQ(map.GetTile(0, 5, 3), 0);
}

TEST(TilemapTest, SetTile_AndGet)
{
    Tilemap map = MakeTestMap();
    map.SetTile(0, 3, 4, 42);
    EXPECT_EQ(map.GetTile(0, 3, 4), 42);
}

TEST(TilemapTest, SetTile_MultiplePositions)
{
    Tilemap map = MakeTestMap();
    map.SetTile(0, 0, 0, 1);
    map.SetTile(0, 9, 7, 2);
    map.SetTile(0, 5, 5, 3);
    EXPECT_EQ(map.GetTile(0, 0, 0), 1);
    EXPECT_EQ(map.GetTile(0, 9, 7), 2);
    EXPECT_EQ(map.GetTile(0, 5, 5), 3);
}

TEST(TilemapTest, SetTile_Overwrite)
{
    Tilemap map = MakeTestMap();
    map.SetTile(0, 2, 3, 10);
    map.SetTile(0, 2, 3, 20);
    EXPECT_EQ(map.GetTile(0, 2, 3), 20);
}

TEST(TilemapTest, SetTile_DoesNotAffectOthers)
{
    Tilemap map = MakeTestMap();
    map.SetTile(0, 3, 3, 99);
    EXPECT_EQ(map.GetTile(0, 3, 2), 0);
    EXPECT_EQ(map.GetTile(0, 2, 3), 0);
    EXPECT_EQ(map.GetTile(0, 4, 3), 0);
}

// ==================== WorldToTile ====================

TEST(TilemapTest, WorldToTile_Origin)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    int tx, ty;
    map.WorldToTile(0.0f, 0.0f, tx, ty);
    EXPECT_EQ(tx, 0);
    EXPECT_EQ(ty, 0);
}

TEST(TilemapTest, WorldToTile_Center)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    int tx, ty;
    map.WorldToTile(48.0f, 48.0f, tx, ty);
    EXPECT_EQ(tx, 1);  // 48/32 = 1.5 → tile 1
    EXPECT_EQ(ty, 1);
}

TEST(TilemapTest, WorldToTile_EdgeOfTile)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    int tx, ty;
    map.WorldToTile(31.9f, 31.9f, tx, ty);
    EXPECT_EQ(tx, 0);
    EXPECT_EQ(ty, 0);
}

TEST(TilemapTest, WorldToTile_SecondTile)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    int tx, ty;
    map.WorldToTile(32.0f, 0.0f, tx, ty);
    EXPECT_EQ(tx, 1);
    EXPECT_EQ(ty, 0);
}

TEST(TilemapTest, WorldToTile_LargeTileSize)
{
    Tilemap map;
    map.Create(5, 5, 64, 64);
    int tx, ty;
    map.WorldToTile(128.0f, 192.0f, tx, ty);
    EXPECT_EQ(tx, 2);
    EXPECT_EQ(ty, 3);
}

// ==================== TileToWorld ====================

TEST(TilemapTest, TileToWorld_Origin)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    float wx, wy;
    map.TileToWorld(0, 0, wx, wy);
    EXPECT_NEAR(wx, 0.0f, kEps);
    EXPECT_NEAR(wy, 0.0f, kEps);
}

TEST(TilemapTest, TileToWorld_NextTile)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    float wx, wy;
    map.TileToWorld(1, 0, wx, wy);
    EXPECT_NEAR(wx, 32.0f, kEps);
    EXPECT_NEAR(wy, 0.0f, kEps);
}

TEST(TilemapTest, TileToWorld_Diagonal)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    float wx, wy;
    map.TileToWorld(3, 4, wx, wy);
    EXPECT_NEAR(wx, 96.0f, kEps);
    EXPECT_NEAR(wy, 128.0f, kEps);
}

TEST(TilemapTest, TileToWorld_RoundTrip)
{
    Tilemap map = MakeTestMap(10, 10, 32, 32);
    float wx, wy;
    map.TileToWorld(5, 7, wx, wy);
    int tx, ty;
    map.WorldToTile(wx, wy, tx, ty);
    EXPECT_EQ(tx, 5);
    EXPECT_EQ(ty, 7);
}

// ==================== Tileset ====================

TEST(TilemapTest, AddTileset)
{
    Tilemap map = MakeTestMap();
    Tileset ts;
    ts.firstGid = 1;
    ts.tileWidth = 32;
    ts.tileHeight = 32;
    ts.columns = 8;
    ts.tileCount = 64;
    ts.textureHandle = -1;
    map.AddTileset(ts);
    // クラッシュしないことだけ確認
    EXPECT_EQ(map.GetWidth(), 10);
}

// ==================== 異なるタイルサイズ ====================

TEST(TilemapTest, NonSquareTiles)
{
    Tilemap map;
    map.Create(10, 20, 16, 32);
    EXPECT_EQ(map.GetPixelWidth(), 160);
    EXPECT_EQ(map.GetPixelHeight(), 640);
}

TEST(TilemapTest, NonSquareTiles_WorldToTile)
{
    Tilemap map;
    map.Create(10, 10, 16, 32);
    TilemapLayer layer;
    layer.name = "Layer";
    layer.width = 10;
    layer.height = 10;
    layer.tiles.resize(100, 0);
    layer.visible = true;
    layer.opacity = 1.0f;
    map.AddLayer(layer);

    int tx, ty;
    map.WorldToTile(16.0f, 32.0f, tx, ty);
    EXPECT_EQ(tx, 1);
    EXPECT_EQ(ty, 1);
}
