/// @file test_PolyNavMesh.cpp
/// @brief PolyNavMesh ポリゴンベースNavMeshテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/PolyNavMesh.h"

using namespace gx;

static constexpr float kEps = 1e-3f;

// 2三角形のクワッド（4頂点）を構築するヘルパー
// (0,0,0) -- (1,0,0)
//   |    \      |
// (0,0,1) -- (1,0,1)
// 三角形0: (0,0,0),(1,0,0),(0,0,1)
// 三角形1: (1,0,0),(1,0,1),(0,0,1)
static void BuildSimpleQuad(gx::Vector<float>& verts, gx::Vector<uint32_t>& indices)
{
    verts = {
        0.0f, 0.0f, 0.0f,  // v0
        1.0f, 0.0f, 0.0f,  // v1
        0.0f, 0.0f, 1.0f,  // v2
        1.0f, 0.0f, 1.0f   // v3
    };
    indices = {
        0, 1, 2,  // tri0
        1, 3, 2   // tri1
    };
}

// 10x10グリッドメッシュを構築するヘルパー
static void BuildGrid(int gridSize, gx::Vector<float>& verts, gx::Vector<uint32_t>& indices)
{
    verts.clear();
    indices.clear();

    for (int z = 0; z <= gridSize; ++z)
    {
        for (int x = 0; x <= gridSize; ++x)
        {
            verts.push_back(static_cast<float>(x));
            verts.push_back(0.0f);
            verts.push_back(static_cast<float>(z));
        }
    }

    int stride = gridSize + 1;
    for (int z = 0; z < gridSize; ++z)
    {
        for (int x = 0; x < gridSize; ++x)
        {
            uint32_t i0 = static_cast<uint32_t>(z * stride + x);
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + static_cast<uint32_t>(stride);
            uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }
}

// ==================== BuildMesh ====================

TEST(PolyNavMeshTest, Build_SimpleQuad)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);
    EXPECT_EQ(mesh.GetPolygonCount(), 2u);
}

TEST(PolyNavMeshTest, Build_EmptyMesh)
{
    PolyNavMesh mesh;
    mesh.Build(nullptr, 0, nullptr, 0);
    EXPECT_EQ(mesh.GetPolygonCount(), 0u);
}

TEST(PolyNavMeshTest, Build_InvalidIndexCount)
{
    float verts[] = { 0, 0, 0, 1, 0, 0 };
    uint32_t indices[] = { 0, 1 };
    PolyNavMesh mesh;
    mesh.Build(verts, 2, indices, 2); // Not multiple of 3
    EXPECT_EQ(mesh.GetPolygonCount(), 0u);
}

// ==================== Adjacency ====================

TEST(PolyNavMeshTest, Adjacency_SharedEdge)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    const NavTriangle* tri0 = mesh.GetTriangle(0);
    const NavTriangle* tri1 = mesh.GetTriangle(1);
    ASSERT_NE(tri0, nullptr);
    ASSERT_NE(tri1, nullptr);

    // 少なくとも1つの隣接が設定されているはず
    bool tri0HasNeighbor = (tri0->neighborIndices[0] == 1) ||
                           (tri0->neighborIndices[1] == 1) ||
                           (tri0->neighborIndices[2] == 1);
    bool tri1HasNeighbor = (tri1->neighborIndices[0] == 0) ||
                           (tri1->neighborIndices[1] == 0) ||
                           (tri1->neighborIndices[2] == 0);
    EXPECT_TRUE(tri0HasNeighbor);
    EXPECT_TRUE(tri1HasNeighbor);
}

TEST(PolyNavMeshTest, Adjacency_NoNeighborForSingleTriangle)
{
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 0.0f, 1.0f
    };
    uint32_t indices[] = { 0, 1, 2 };

    PolyNavMesh mesh;
    mesh.Build(verts, 3, indices, 3);
    EXPECT_EQ(mesh.GetPolygonCount(), 1u);

    const NavTriangle* tri = mesh.GetTriangle(0);
    ASSERT_NE(tri, nullptr);
    EXPECT_EQ(tri->neighborIndices[0], -1);
    EXPECT_EQ(tri->neighborIndices[1], -1);
    EXPECT_EQ(tri->neighborIndices[2], -1);
}

// ==================== PointInMesh ====================

TEST(PolyNavMeshTest, PointInMesh_InsideReturnsTrue)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    EXPECT_TRUE(mesh.IsPointInMesh({ 0.5f, 0.0f, 0.5f }));
    EXPECT_TRUE(mesh.IsPointInMesh({ 0.25f, 0.0f, 0.25f }));
}

TEST(PolyNavMeshTest, PointInMesh_OutsideReturnsFalse)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    EXPECT_FALSE(mesh.IsPointInMesh({ 5.0f, 0.0f, 5.0f }));
    EXPECT_FALSE(mesh.IsPointInMesh({ -1.0f, 0.0f, -1.0f }));
}

TEST(PolyNavMeshTest, PointInMesh_EmptyMesh)
{
    PolyNavMesh mesh;
    EXPECT_FALSE(mesh.IsPointInMesh({ 0.0f, 0.0f, 0.0f }));
}

// ==================== FindNearest ====================

TEST(PolyNavMeshTest, FindNearest_InsideReturnsCorrectPoly)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    int idx = mesh.FindNearestPolygon({ 0.25f, 0.0f, 0.25f });
    EXPECT_GE(idx, 0);
    EXPECT_LT(idx, 2);
}

TEST(PolyNavMeshTest, FindNearest_OutsideReturnsClosest)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    int idx = mesh.FindNearestPolygon({ 10.0f, 0.0f, 10.0f });
    EXPECT_GE(idx, 0);
}

TEST(PolyNavMeshTest, FindNearest_EmptyMesh)
{
    PolyNavMesh mesh;
    EXPECT_EQ(mesh.FindNearestPolygon({ 0.0f, 0.0f, 0.0f }), -1);
}

// ==================== Pathfinding ====================

TEST(PolyNavMeshTest, FindPath_SameTriangle)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    auto path = mesh.FindPath({ 0.1f, 0.0f, 0.1f }, { 0.2f, 0.0f, 0.2f });
    EXPECT_GE(path.size(), 2u);
}

TEST(PolyNavMeshTest, FindPath_AcrossTriangles)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);

    auto path = mesh.FindPath({ 0.1f, 0.0f, 0.1f }, { 0.9f, 0.0f, 0.9f });
    EXPECT_GE(path.size(), 2u);

    // 開始点と終了点が含まれるはず
    EXPECT_NEAR(path.front().x, 0.1f, kEps);
    EXPECT_NEAR(path.front().z, 0.1f, kEps);
    EXPECT_NEAR(path.back().x, 0.9f, kEps);
    EXPECT_NEAR(path.back().z, 0.9f, kEps);
}

TEST(PolyNavMeshTest, FindPath_EmptyMesh)
{
    PolyNavMesh mesh;
    auto path = mesh.FindPath({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f });
    EXPECT_TRUE(path.empty());
}

TEST(PolyNavMeshTest, FindPath_GridMesh)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildGrid(5, verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), static_cast<uint32_t>(verts.size() / 3),
               indices.data(), static_cast<uint32_t>(indices.size()));

    auto path = mesh.FindPath({ 0.5f, 0.0f, 0.5f }, { 4.5f, 0.0f, 4.5f });
    EXPECT_GE(path.size(), 2u);
}

// ==================== FunnelSmoothing ====================

TEST(PolyNavMeshTest, FunnelSmoothing_PathNotLongerThanCentroid)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildGrid(5, verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), static_cast<uint32_t>(verts.size() / 3),
               indices.data(), static_cast<uint32_t>(indices.size()));

    auto path = mesh.FindPath({ 0.5f, 0.0f, 0.5f }, { 4.5f, 0.0f, 4.5f });
    ASSERT_GE(path.size(), 2u);

    // パス長を計算
    float pathLength = 0.0f;
    for (size_t i = 1; i < path.size(); ++i)
    {
        pathLength += NavPoint::Distance(path[i - 1], path[i]);
    }

    // 直線距離
    float directDist = NavPoint::Distance({ 0.5f, 0.0f, 0.5f }, { 4.5f, 0.0f, 4.5f });

    // ファンネルパスは直線距離以上だが、三角形数×2倍以下であるべき
    EXPECT_GE(pathLength, directDist - kEps);
}

// ==================== MaxSlope ====================

TEST(PolyNavMeshTest, MaxSlope_DefaultValue)
{
    PolyNavMesh mesh;
    EXPECT_NEAR(mesh.GetMaxSlope(), 45.0f, kEps);
}

TEST(PolyNavMeshTest, MaxSlope_SetGet)
{
    PolyNavMesh mesh;
    mesh.SetMaxSlope(30.0f);
    EXPECT_NEAR(mesh.GetMaxSlope(), 30.0f, kEps);
}

TEST(PolyNavMeshTest, MaxSlope_SteepTriangleFiltered)
{
    // 90度の壁面三角形（法線がY=0）
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f  // 垂直な壁
    };
    uint32_t indices[] = { 0, 1, 2 };

    PolyNavMesh mesh;
    mesh.SetMaxSlope(45.0f);
    mesh.Build(verts, 3, indices, 3);

    // 垂直壁はフィルタされるべき
    EXPECT_EQ(mesh.GetPolygonCount(), 0u);
}

TEST(PolyNavMeshTest, MaxSlope_FlatTriangleKept)
{
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 0.0f, 1.0f  // 水平
    };
    uint32_t indices[] = { 0, 1, 2 };

    PolyNavMesh mesh;
    mesh.SetMaxSlope(45.0f);
    mesh.Build(verts, 3, indices, 3);

    EXPECT_EQ(mesh.GetPolygonCount(), 1u);
}

// ==================== TriangleBuild ====================

TEST(PolyNavMeshTest, TriangleBuild_CorrectVertexStorage)
{
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 0.0f, 1.0f
    };
    uint32_t indices[] = { 0, 1, 2 };

    PolyNavMesh mesh;
    mesh.Build(verts, 3, indices, 3);

    const NavTriangle* tri = mesh.GetTriangle(0);
    ASSERT_NE(tri, nullptr);
    EXPECT_NEAR(tri->v0.x, 0.0f, kEps);
    EXPECT_NEAR(tri->v1.x, 1.0f, kEps);
    EXPECT_NEAR(tri->v2.x, 0.5f, kEps);
    EXPECT_NEAR(tri->v2.z, 1.0f, kEps);
}

TEST(PolyNavMeshTest, GetTriangle_InvalidIndex)
{
    PolyNavMesh mesh;
    EXPECT_EQ(mesh.GetTriangle(0), nullptr);
    EXPECT_EQ(mesh.GetTriangle(999), nullptr);
}

// ==================== LargeGrid ====================

TEST(PolyNavMeshTest, LargeGrid_PathfindingSucceeds)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildGrid(10, verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), static_cast<uint32_t>(verts.size() / 3),
               indices.data(), static_cast<uint32_t>(indices.size()));

    EXPECT_EQ(mesh.GetPolygonCount(), 200u); // 10x10 grid = 100 quads = 200 tris

    auto path = mesh.FindPath({ 0.5f, 0.0f, 0.5f }, { 9.5f, 0.0f, 9.5f });
    EXPECT_GE(path.size(), 2u);
}

TEST(PolyNavMeshTest, LargeGrid_PointInMesh)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildGrid(10, verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), static_cast<uint32_t>(verts.size() / 3),
               indices.data(), static_cast<uint32_t>(indices.size()));

    EXPECT_TRUE(mesh.IsPointInMesh({ 5.0f, 0.0f, 5.0f }));
    EXPECT_FALSE(mesh.IsPointInMesh({ 15.0f, 0.0f, 15.0f }));
}

// ==================== Clear ====================

TEST(PolyNavMeshTest, Clear_ResetsState)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);
    EXPECT_EQ(mesh.GetPolygonCount(), 2u);

    mesh.Clear();
    EXPECT_EQ(mesh.GetPolygonCount(), 0u);
    EXPECT_FALSE(mesh.IsPointInMesh({ 0.5f, 0.0f, 0.5f }));
}

TEST(PolyNavMeshTest, Clear_FindPathReturnsEmpty)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);
    mesh.Clear();

    auto path = mesh.FindPath({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f });
    EXPECT_TRUE(path.empty());
}

// ==================== Rebuild ====================

TEST(PolyNavMeshTest, Rebuild_AfterClear)
{
    gx::Vector<float> verts;
    gx::Vector<uint32_t> indices;
    BuildSimpleQuad(verts, indices);

    PolyNavMesh mesh;
    mesh.Build(verts.data(), 4, indices.data(), 6);
    mesh.Clear();
    mesh.Build(verts.data(), 4, indices.data(), 6);

    EXPECT_EQ(mesh.GetPolygonCount(), 2u);
    EXPECT_TRUE(mesh.IsPointInMesh({ 0.5f, 0.0f, 0.5f }));
}
