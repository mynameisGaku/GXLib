/// @file test_NavMesh3D.cpp
/// @brief NavMesh3D（3Dボクセルベースナビゲーション）のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh3D.h"

using namespace gx;

// ============================================================================
// ビルド
// ============================================================================

TEST(NavMesh3DTest, BuildCreatesGridOfCorrectSize)
{
    NavMesh3D nav;
    Vector3 minB = { 0.0f, 0.0f, 0.0f };
    Vector3 maxB = { 10.0f, 5.0f, 8.0f };
    EXPECT_TRUE(nav.Build(minB, maxB, 1.0f));
    EXPECT_EQ(nav.GetSizeX(), 10);
    EXPECT_EQ(nav.GetSizeY(), 5);
    EXPECT_EQ(nav.GetSizeZ(), 8);
}

TEST(NavMesh3DTest, AllVoxelsInitiallyOpen)
{
    NavMesh3D nav;
    Vector3 minB = { 0.0f, 0.0f, 0.0f };
    Vector3 maxB = { 3.0f, 3.0f, 3.0f };
    nav.Build(minB, maxB, 1.0f);

    // ビルド後、すべてのボクセルはOpenであるべき
    for (int x = 0; x < nav.GetSizeX(); ++x)
        for (int y = 0; y < nav.GetSizeY(); ++y)
            for (int z = 0; z < nav.GetSizeZ(); ++z)
                EXPECT_EQ(nav.GetVoxel(x, y, z), VoxelState::Open);
}

TEST(NavMesh3DTest, GetVoxelSize)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 10, 10 }, 2.5f);
    EXPECT_FLOAT_EQ(nav.GetVoxelSize(), 2.5f);
}

TEST(NavMesh3DTest, GetMinBounds)
{
    NavMesh3D nav;
    nav.Build({ -5.0f, -3.0f, -1.0f }, { 5.0f, 3.0f, 1.0f }, 1.0f);
    Vector3 mb = nav.GetMinBounds();
    EXPECT_FLOAT_EQ(mb.x, -5.0f);
    EXPECT_FLOAT_EQ(mb.y, -3.0f);
    EXPECT_FLOAT_EQ(mb.z, -1.0f);
}

// ============================================================================
// SetVoxel / GetVoxel（ボクセルの設定/取得）
// ============================================================================

TEST(NavMesh3DTest, SetVoxelBlocked)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 5, 5 }, 1.0f);
    nav.SetVoxel(2, 2, 2, VoxelState::Blocked);
    EXPECT_EQ(nav.GetVoxel(2, 2, 2), VoxelState::Blocked);
}

TEST(NavMesh3DTest, OutOfBoundsReturnsBlocked)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 3, 3, 3 }, 1.0f);
    EXPECT_EQ(nav.GetVoxel(-1, 0, 0), VoxelState::Blocked);
    EXPECT_EQ(nav.GetVoxel(0, -1, 0), VoxelState::Blocked);
    EXPECT_EQ(nav.GetVoxel(0, 0, -1), VoxelState::Blocked);
    EXPECT_EQ(nav.GetVoxel(3, 0, 0), VoxelState::Blocked);
    EXPECT_EQ(nav.GetVoxel(100, 100, 100), VoxelState::Blocked);
}

// ============================================================================
// WorldToVoxel / VoxelToWorld（ワールド座標とボクセル座標の変換）
// ============================================================================

TEST(NavMesh3DTest, WorldToVoxelRoundTrip)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 10, 10 }, 1.0f);

    // ボクセル(3,4,5)の中心は(3.5, 4.5, 5.5)
    Vector3 worldPos = nav.VoxelToWorld(3, 4, 5);
    int vx, vy, vz;
    bool inBounds = nav.WorldToVoxel(worldPos, vx, vy, vz);
    EXPECT_TRUE(inBounds);
    EXPECT_EQ(vx, 3);
    EXPECT_EQ(vy, 4);
    EXPECT_EQ(vz, 5);
}

TEST(NavMesh3DTest, WorldToVoxelBoundsCheck)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 5, 5 }, 1.0f);

    int vx, vy, vz;
    // グリッド外の位置
    Vector3 outside = { -10.0f, -10.0f, -10.0f };
    bool inBounds = nav.WorldToVoxel(outside, vx, vy, vz);
    EXPECT_FALSE(inBounds);
}

// ============================================================================
// 経路探索
// ============================================================================

TEST(NavMesh3DTest, FindPathStraightLine)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 1, 1 }, 1.0f);

    Vector3 start = { 0.5f, 0.5f, 0.5f };
    Vector3 end   = { 9.5f, 0.5f, 0.5f };
    gx::Vector<Vector3> path;
    bool found = nav.FindPath(start, end, path);
    EXPECT_TRUE(found);
    EXPECT_GE(path.size(), 2u);
    // 最初と最後のポイントは開始点と終了点に一致するべき
    EXPECT_FLOAT_EQ(path.front().x, start.x);
    EXPECT_FLOAT_EQ(path.back().x, end.x);
}

TEST(NavMesh3DTest, FindPathAroundBlockedVoxel)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 1, 3 }, 1.0f);

    // 中央の列をブロックし、z方向への迂回を強制
    nav.SetVoxel(2, 0, 1, VoxelState::Blocked);

    Vector3 start = { 0.5f, 0.5f, 1.5f };
    Vector3 end   = { 4.5f, 0.5f, 1.5f };
    gx::Vector<Vector3> path;
    bool found = nav.FindPath(start, end, path);
    EXPECT_TRUE(found);
    EXPECT_GE(path.size(), 2u);
}

TEST(NavMesh3DTest, FindPathNoPathWhenCompletelyBlocked)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 3, 1, 3 }, 1.0f);

    // 目的地の周囲を壁でブロック
    for (int x = 0; x < 3; ++x)
        for (int z = 0; z < 3; ++z)
            if (!(x == 0 && z == 0))
                nav.SetVoxel(x, 0, z, VoxelState::Blocked);

    Vector3 start = { 0.5f, 0.5f, 0.5f };
    Vector3 end   = { 2.5f, 0.5f, 2.5f };
    gx::Vector<Vector3> path;
    bool found = nav.FindPath(start, end, path);
    EXPECT_FALSE(found);
}

TEST(NavMesh3DTest, FindPathAllowedStatesFilter)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 1, 1 }, 1.0f);

    // 中央のボクセルをWaterに設定
    nav.SetVoxel(2, 0, 0, VoxelState::Water);

    // Openのみ許可（ビット1 = 0x02）、Waterは不許可（ビット2 = 0x04）
    uint8_t allowOnlyOpen = (1 << static_cast<int>(VoxelState::Open));

    Vector3 start = { 0.5f, 0.5f, 0.5f };
    Vector3 end   = { 4.5f, 0.5f, 0.5f };
    gx::Vector<Vector3> path;
    bool found = nav.FindPath(start, end, path, allowOnlyOpen);
    // 幅1の通路ではこのフィルタでWaterを通過できない
    EXPECT_FALSE(found);
}

// ============================================================================
// オフメッシュリンク
// ============================================================================

TEST(NavMesh3DTest, AddOffMeshLinkIncreasesCount)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 10, 10 }, 1.0f);
    EXPECT_EQ(nav.GetOffMeshLinkCount(), 0u);

    OffMeshLink link;
    link.start = { 1.0f, 1.0f, 1.0f };
    link.end   = { 8.0f, 8.0f, 8.0f };
    link.cost  = 2.0f;
    nav.AddOffMeshLink(link);
    EXPECT_EQ(nav.GetOffMeshLinkCount(), 1u);
}

TEST(NavMesh3DTest, RemoveOffMeshLink)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 10, 10 }, 1.0f);

    OffMeshLink link;
    link.start = { 1, 1, 1 };
    link.end   = { 5, 5, 5 };
    uint32_t idx = nav.AddOffMeshLink(link);
    EXPECT_EQ(nav.GetOffMeshLinkCount(), 1u);
    nav.RemoveOffMeshLink(idx);
    EXPECT_EQ(nav.GetOffMeshLinkCount(), 0u);
}

// ============================================================================
// FindNearestOpen
// ============================================================================

TEST(NavMesh3DTest, FindNearestOpenFindsItself)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 5, 5 }, 1.0f);
    // すべてのボクセルがOpenなので、任意の位置から最も近いOpenはボクセル中心
    Vector3 pos = { 2.5f, 2.5f, 2.5f };
    Vector3 nearest = nav.FindNearestOpen(pos);
    // posを含むボクセルの中心を返すべき
    int vx, vy, vz;
    nav.WorldToVoxel(pos, vx, vy, vz);
    Vector3 expected = nav.VoxelToWorld(vx, vy, vz);
    EXPECT_NEAR(nearest.x, expected.x, 0.01f);
    EXPECT_NEAR(nearest.y, expected.y, 0.01f);
    EXPECT_NEAR(nearest.z, expected.z, 0.01f);
}

TEST(NavMesh3DTest, FindNearestOpenFindsNearbyIfBlocked)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 1, 1 }, 1.0f);
    // (2,0,0)のボクセルをブロックし、FindNearestOpenが隣接するボクセルを見つけることを確認
    nav.SetVoxel(2, 0, 0, VoxelState::Blocked);
    Vector3 pos = { 2.5f, 0.5f, 0.5f }; // ブロックされたボクセルの中心
    Vector3 nearest = nav.FindNearestOpen(pos);
    // ブロックされたボクセルの中心であってはならない
    int vx, vy, vz;
    nav.WorldToVoxel(nearest, vx, vy, vz);
    EXPECT_NE(nav.GetVoxel(vx, vy, vz), VoxelState::Blocked);
}

// ============================================================================
// レイキャスト
// ============================================================================

TEST(NavMesh3DTest, RaycastHitsBlockedVoxel)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 1, 1 }, 1.0f);
    nav.SetVoxel(5, 0, 0, VoxelState::Blocked);

    Vector3 origin = { 0.5f, 0.5f, 0.5f };
    Vector3 dir    = { 1.0f, 0.0f, 0.0f };
    Vector3 hitPos;
    bool hit = nav.Raycast(origin, dir, 20.0f, hitPos);
    EXPECT_TRUE(hit);
    // ヒット位置はボクセル5の手前またはボクセル5にあるべき
    EXPECT_LE(hitPos.x, 6.0f);
}

TEST(NavMesh3DTest, RaycastMissesInOpenSpace)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 10, 1, 1 }, 1.0f);
    // すべてOpen、ブロックされたボクセルなし

    Vector3 origin = { 0.5f, 0.5f, 0.5f };
    Vector3 dir    = { 1.0f, 0.0f, 0.0f };
    Vector3 hitPos;
    bool hit = nav.Raycast(origin, dir, 20.0f, hitPos);
    EXPECT_FALSE(hit);
}

// ============================================================================
// GetOpenVoxelCount / Clear（Open数取得/クリア）
// ============================================================================

TEST(NavMesh3DTest, GetOpenVoxelCount)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 3, 3, 3 }, 1.0f);
    uint32_t total = static_cast<uint32_t>(3 * 3 * 3);
    EXPECT_EQ(nav.GetOpenVoxelCount(), total);

    nav.SetVoxel(0, 0, 0, VoxelState::Blocked);
    nav.SetVoxel(1, 1, 1, VoxelState::Blocked);
    EXPECT_EQ(nav.GetOpenVoxelCount(), total - 2);
}

TEST(NavMesh3DTest, ClearResetsEverything)
{
    NavMesh3D nav;
    nav.Build({ 0, 0, 0 }, { 5, 5, 5 }, 1.0f);
    OffMeshLink link;
    link.start = { 1, 1, 1 };
    link.end   = { 4, 4, 4 };
    nav.AddOffMeshLink(link);

    nav.Clear();
    EXPECT_EQ(nav.GetSizeX(), 0);
    EXPECT_EQ(nav.GetSizeY(), 0);
    EXPECT_EQ(nav.GetSizeZ(), 0);
    EXPECT_EQ(nav.GetOffMeshLinkCount(), 0u);
    EXPECT_EQ(nav.GetOpenVoxelCount(), 0u);
}
