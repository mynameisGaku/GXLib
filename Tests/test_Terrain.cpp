#include "pch_graphics.h"
#include "Graphics/3D/Terrain.h"
#include <gtest/gtest.h>

using namespace gx;

class TerrainTest : public ::testing::Test
{
protected:
    Terrain terrain;
};

TEST_F(TerrainTest, DefaultDimensions)
{
    EXPECT_FLOAT_EQ(terrain.GetWidth(), 0.0f);
    EXPECT_FLOAT_EQ(terrain.GetDepth(), 0.0f);
    EXPECT_EQ(terrain.GetIndexCount(), 0u);
}

TEST_F(TerrainTest, GetHeightEmpty)
{
    EXPECT_FLOAT_EQ(terrain.GetHeight(0.0f, 0.0f), 0.0f);
}

TEST_F(TerrainTest, ProceduralCreation)
{
    // nullptr device: creates height map but no GPU buffers
    bool ok = terrain.CreateProcedural(nullptr, 100.0f, 100.0f, 16, 16, 10.0f);
    EXPECT_TRUE(ok);

    // Height query should work
    float h = terrain.GetHeight(0.0f, 0.0f);
    // Just check it returns a finite value
    EXPECT_TRUE(std::isfinite(h));
}

TEST_F(TerrainTest, ProceduralDimensions)
{
    terrain.CreateProcedural(nullptr, 200.0f, 150.0f, 8, 8, 5.0f);
    EXPECT_FLOAT_EQ(terrain.GetWidth(), 200.0f);
    EXPECT_FLOAT_EQ(terrain.GetDepth(), 150.0f);
}

TEST_F(TerrainTest, ProceduralOrigin)
{
    terrain.CreateProcedural(nullptr, 100.0f, 100.0f, 4, 4, 10.0f);
    // Origin should be at -width/2, -depth/2
    EXPECT_FLOAT_EQ(terrain.GetOriginX(), -50.0f);
    EXPECT_FLOAT_EQ(terrain.GetOriginZ(), -50.0f);
}

TEST_F(TerrainTest, GetHeightMultiplePoints)
{
    terrain.CreateProcedural(nullptr, 100.0f, 100.0f, 16, 16, 10.0f);

    // Query multiple points, all should be finite
    float h1 = terrain.GetHeight(10.0f, 10.0f);
    float h2 = terrain.GetHeight(-20.0f, 30.0f);
    float h3 = terrain.GetHeight(0.0f, 0.0f);
    EXPECT_TRUE(std::isfinite(h1));
    EXPECT_TRUE(std::isfinite(h2));
    EXPECT_TRUE(std::isfinite(h3));
}

TEST_F(TerrainTest, CreateFromHeightmap)
{
    // Create a flat heightmap (all zeros)
    std::vector<float> heights(9, 0.0f); // 3x3
    bool ok = terrain.CreateFromHeightmap(nullptr, heights.data(), 3, 3,
                                          30.0f, 30.0f, 10.0f);
    EXPECT_TRUE(ok);
    EXPECT_FLOAT_EQ(terrain.GetWidth(), 30.0f);
    EXPECT_FLOAT_EQ(terrain.GetDepth(), 30.0f);

    // All heights should be 0 for a zero heightmap
    float h = terrain.GetHeight(0.0f, 0.0f);
    EXPECT_NEAR(h, 0.0f, 0.01f);
}

TEST_F(TerrainTest, CreateFromHeightmapNonFlat)
{
    // 2x2 heightmap with varied values
    float heights[] = { 0.0f, 0.5f, 0.5f, 1.0f };
    bool ok = terrain.CreateFromHeightmap(nullptr, heights, 2, 2,
                                          20.0f, 20.0f, 10.0f);
    EXPECT_TRUE(ok);

    // Heights should be between 0 and maxHeight
    float h = terrain.GetHeight(5.0f, 5.0f);
    EXPECT_TRUE(std::isfinite(h));
    EXPECT_GE(h, 0.0f);
    EXPECT_LE(h, 10.0f);
}
