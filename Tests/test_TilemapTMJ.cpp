/// @file test_TilemapTMJ.cpp
/// @brief Tilemap manual construction and tile access tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/Rendering/Tilemap.h"

using namespace gx;

// ===========================================================================
// Tileset defaults
// ===========================================================================

TEST(TilemapTMJTest, TilesetDefaults)
{
    Tileset ts;
    EXPECT_EQ(ts.firstGid, 1);
    EXPECT_EQ(ts.tileWidth, 32);
    EXPECT_EQ(ts.tileHeight, 32);
    EXPECT_EQ(ts.columns, 1);
    EXPECT_EQ(ts.tileCount, 0);
    EXPECT_EQ(ts.textureHandle, -1);
}

// ===========================================================================
// TilemapLayer defaults
// ===========================================================================

TEST(TilemapTMJTest, LayerDefaults)
{
    TilemapLayer layer;
    EXPECT_TRUE(layer.name.empty());
    EXPECT_EQ(layer.width, 0);
    EXPECT_EQ(layer.height, 0);
    EXPECT_TRUE(layer.tiles.empty());
    EXPECT_TRUE(layer.visible);
    EXPECT_FLOAT_EQ(layer.opacity, 1.0f);
}

// ===========================================================================
// Tilemap Create
// ===========================================================================

TEST(TilemapTMJTest, CreateDimensions)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);
    EXPECT_EQ(map.GetWidth(), 4);
    EXPECT_EQ(map.GetHeight(), 3);
    EXPECT_EQ(map.GetTileWidth(), 16);
    EXPECT_EQ(map.GetTileHeight(), 16);
}

TEST(TilemapTMJTest, CreatePixelDimensions)
{
    Tilemap map;
    map.Create(10, 8, 32, 32);
    EXPECT_EQ(map.GetPixelWidth(), 320);
    EXPECT_EQ(map.GetPixelHeight(), 256);
}

// ===========================================================================
// Layer operations
// ===========================================================================

TEST(TilemapTMJTest, AddLayerCount)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);

    TilemapLayer layer;
    layer.name = "Ground";
    layer.width = 4;
    layer.height = 3;
    layer.tiles = { 1,2,3,4, 5,6,7,8, 9,10,11,12 };
    map.AddLayer(layer);

    EXPECT_EQ(map.GetLayerCount(), 1);
}

TEST(TilemapTMJTest, AddMultipleLayers)
{
    Tilemap map;
    map.Create(2, 2, 16, 16);

    TilemapLayer ground;
    ground.name = "Ground";
    ground.width = 2;
    ground.height = 2;
    ground.tiles = { 1, 2, 3, 4 };
    map.AddLayer(ground);

    TilemapLayer overlay;
    overlay.name = "Overlay";
    overlay.width = 2;
    overlay.height = 2;
    overlay.tiles = { 0, 5, 5, 0 };
    map.AddLayer(overlay);

    EXPECT_EQ(map.GetLayerCount(), 2);
}

TEST(TilemapTMJTest, GetLayerName)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);

    TilemapLayer layer;
    layer.name = "Background";
    layer.width = 4;
    layer.height = 3;
    layer.tiles.resize(12, 1);
    map.AddLayer(layer);

    EXPECT_EQ(map.GetLayer(0).name, "Background");
}

TEST(TilemapTMJTest, LayerVisibility)
{
    Tilemap map;
    map.Create(2, 2, 16, 16);

    TilemapLayer layer;
    layer.name = "Hidden";
    layer.width = 2;
    layer.height = 2;
    layer.tiles = { 1, 1, 1, 1 };
    layer.visible = false;
    layer.opacity = 0.5f;
    map.AddLayer(layer);

    EXPECT_FALSE(map.GetLayer(0).visible);
    EXPECT_FLOAT_EQ(map.GetLayer(0).opacity, 0.5f);
}

// ===========================================================================
// Tile access
// ===========================================================================

TEST(TilemapTMJTest, GetTileValues)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);

    TilemapLayer layer;
    layer.name = "Ground";
    layer.width = 4;
    layer.height = 3;
    layer.tiles = { 1,2,3,4, 5,6,7,8, 9,10,11,12 };
    map.AddLayer(layer);

    EXPECT_EQ(map.GetTile(0, 0, 0), 1);
    EXPECT_EQ(map.GetTile(0, 3, 0), 4);
    EXPECT_EQ(map.GetTile(0, 0, 2), 9);
    EXPECT_EQ(map.GetTile(0, 3, 2), 12);
}

TEST(TilemapTMJTest, SetTile)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);

    TilemapLayer layer;
    layer.name = "Ground";
    layer.width = 4;
    layer.height = 3;
    layer.tiles.resize(12, 0);
    map.AddLayer(layer);

    map.SetTile(0, 1, 1, 42);
    EXPECT_EQ(map.GetTile(0, 1, 1), 42);
}

TEST(TilemapTMJTest, GetTileOutOfBoundsReturnsZero)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);

    TilemapLayer layer;
    layer.name = "Ground";
    layer.width = 4;
    layer.height = 3;
    layer.tiles.resize(12, 1);
    map.AddLayer(layer);

    // Out of bounds
    EXPECT_EQ(map.GetTile(0, 10, 10), 0);
    EXPECT_EQ(map.GetTile(0, -1, 0), 0);
    EXPECT_EQ(map.GetTile(5, 0, 0), 0); // invalid layer
}

// ===========================================================================
// Tileset
// ===========================================================================

TEST(TilemapTMJTest, AddTileset)
{
    Tilemap map;
    map.Create(4, 3, 16, 16);

    Tileset ts;
    ts.firstGid = 1;
    ts.tileWidth = 16;
    ts.tileHeight = 16;
    ts.columns = 8;
    ts.tileCount = 64;
    ts.textureHandle = 42;
    map.AddTileset(ts);

    // No crash; tileset is used internally for rendering
    SUCCEED();
}

// ===========================================================================
// Coordinate conversion
// ===========================================================================

TEST(TilemapTMJTest, WorldToTile)
{
    Tilemap map;
    map.Create(10, 10, 32, 32);

    int tx, ty;
    map.WorldToTile(48.0f, 80.0f, tx, ty);
    EXPECT_EQ(tx, 1); // 48/32 = 1
    EXPECT_EQ(ty, 2); // 80/32 = 2
}

TEST(TilemapTMJTest, TileToWorld)
{
    Tilemap map;
    map.Create(10, 10, 32, 32);

    float wx, wy;
    map.TileToWorld(3, 5, wx, wy);
    EXPECT_FLOAT_EQ(wx, 96.0f);  // 3*32
    EXPECT_FLOAT_EQ(wy, 160.0f); // 5*32
}

TEST(TilemapTMJTest, DefaultEmpty)
{
    Tilemap map;
    EXPECT_EQ(map.GetWidth(), 0);
    EXPECT_EQ(map.GetHeight(), 0);
    EXPECT_EQ(map.GetLayerCount(), 0);
}
