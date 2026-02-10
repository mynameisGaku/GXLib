/// @file test_Spatial.cpp
/// @brief Quadtree, Octree, BVH 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Collision/Quadtree.h"
#include "Math/Collision/Octree.h"
#include "Math/Collision/BVH.h"

using namespace GX;

// ============================================================================
// Quadtree（2D空間を4分割で管理する構造）
// ============================================================================

TEST(QuadtreeTest, InsertAndQuery)
{
    Quadtree<int> qt(AABB2D({0, 0}, {100, 100}));

    // 5個のオブジェクトを登録
    for (int i = 0; i < 5; ++i)
    {
        float x = static_cast<float>(i * 10);
        qt.Insert(i, AABB2D({x, x}, {x + 5, x + 5}));
    }

    EXPECT_EQ(qt.GetObjectCount(), 5);

    // 全領域で検索
    std::vector<int> results;
    qt.Query(AABB2D({0, 0}, {100, 100}), results);
    EXPECT_EQ(static_cast<int>(results.size()), 5);
}

TEST(QuadtreeTest, QuerySubRegion)
{
    Quadtree<int> qt(AABB2D({0, 0}, {100, 100}));

    qt.Insert(0, AABB2D({5, 5}, {10, 10}));
    qt.Insert(1, AABB2D({50, 50}, {55, 55}));
    qt.Insert(2, AABB2D({90, 90}, {95, 95}));

    // 左上の区画だけ検索
    std::vector<int> results;
    qt.Query(AABB2D({0, 0}, {25, 25}), results);
    EXPECT_EQ(static_cast<int>(results.size()), 1);
    EXPECT_EQ(results[0], 0);
}

TEST(QuadtreeTest, Clear)
{
    Quadtree<int> qt(AABB2D({0, 0}, {100, 100}));
    qt.Insert(0, AABB2D({5, 5}, {10, 10}));
    qt.Clear();
    EXPECT_EQ(qt.GetObjectCount(), 0);
}

TEST(QuadtreeTest, QueryCircle)
{
    Quadtree<int> qt(AABB2D({0, 0}, {100, 100}));
    qt.Insert(0, AABB2D({48, 48}, {52, 52})); // 中央
    qt.Insert(1, AABB2D({0, 0}, {2, 2}));     // 端

    std::vector<int> results;
    qt.Query(Circle({50, 50}, 10.0f), results);

    // 中央の要素だけが見つかるはず
    EXPECT_GE(static_cast<int>(results.size()), 1);
}

// ============================================================================
// Octree（3D空間を8分割で管理する構造）
// ============================================================================

TEST(OctreeTest, InsertAndQuery)
{
    Octree<int> ot(AABB3D({0, 0, 0}, {100, 100, 100}));

    for (int i = 0; i < 5; ++i)
    {
        float x = static_cast<float>(i * 10);
        ot.Insert(i, AABB3D({x, x, x}, {x + 5, x + 5, x + 5}));
    }

    std::vector<int> results;
    ot.Query(AABB3D({0, 0, 0}, {100, 100, 100}), results);
    EXPECT_EQ(static_cast<int>(results.size()), 5);
}

TEST(OctreeTest, QuerySubRegion)
{
    Octree<int> ot(AABB3D({0, 0, 0}, {100, 100, 100}));

    ot.Insert(0, AABB3D({5, 5, 5}, {10, 10, 10}));
    ot.Insert(1, AABB3D({80, 80, 80}, {85, 85, 85}));

    std::vector<int> results;
    ot.Query(AABB3D({0, 0, 0}, {50, 50, 50}), results);
    EXPECT_EQ(static_cast<int>(results.size()), 1);
}

TEST(OctreeTest, QuerySphere)
{
    Octree<int> ot(AABB3D({0, 0, 0}, {100, 100, 100}));
    ot.Insert(0, AABB3D({48, 48, 48}, {52, 52, 52}));
    ot.Insert(1, AABB3D({0, 0, 0}, {2, 2, 2}));

    std::vector<int> results;
    ot.Query(Sphere({50, 50, 50}, 10.0f), results);
    EXPECT_GE(static_cast<int>(results.size()), 1);
}

// ============================================================================
// BVH（Bounding Volume Hierarchy：境界体の階層）
// ============================================================================

TEST(BVHTest, BuildAndQuery)
{
    BVH<int> bvh;

    std::vector<std::pair<int, AABB3D>> objects;
    for (int i = 0; i < 10; ++i)
    {
        float x = static_cast<float>(i * 5);
        objects.push_back({i, AABB3D({x, 0, 0}, {x + 3, 3, 3})});
    }

    bvh.Build(objects);

    // 全空間で検索
    std::vector<int> results;
    bvh.Query(AABB3D({-100, -100, -100}, {100, 100, 100}), results);
    EXPECT_EQ(static_cast<int>(results.size()), 10);
}

TEST(BVHTest, QuerySubRegion)
{
    BVH<int> bvh;
    std::vector<std::pair<int, AABB3D>> objects;
    objects.push_back({0, AABB3D({0, 0, 0}, {5, 5, 5})});
    objects.push_back({1, AABB3D({50, 50, 50}, {55, 55, 55})});
    bvh.Build(objects);

    std::vector<int> results;
    bvh.Query(AABB3D({0, 0, 0}, {10, 10, 10}), results);
    EXPECT_EQ(static_cast<int>(results.size()), 1);
}

TEST(BVHTest, Raycast)
{
    BVH<int> bvh;
    std::vector<std::pair<int, AABB3D>> objects;
    objects.push_back({0, AABB3D({5, -1, -1}, {7, 1, 1})});
    objects.push_back({1, AABB3D({50, -1, -1}, {52, 1, 1})});
    bvh.Build(objects);

    float t;
    int hitObj;
    Ray ray({0, 0, 0}, {1, 0, 0});
    EXPECT_TRUE(bvh.Raycast(ray, t, &hitObj));
    EXPECT_EQ(hitObj, 0); // 一番近いオブジェクト
    EXPECT_NEAR(t, 5.0f, 1e-3f);
}

TEST(BVHTest, RaycastMiss)
{
    BVH<int> bvh;
    std::vector<std::pair<int, AABB3D>> objects;
    objects.push_back({0, AABB3D({5, 5, 5}, {7, 7, 7})});
    bvh.Build(objects);

    float t;
    Ray ray({0, 0, 0}, {1, 0, 0}); // X方向に飛ばす（対象はY=5以上）
    EXPECT_FALSE(bvh.Raycast(ray, t));
}

TEST(BVHTest, Empty)
{
    BVH<int> bvh;
    std::vector<std::pair<int, AABB3D>> objects;
    bvh.Build(objects);

    std::vector<int> results;
    bvh.Query(AABB3D({0, 0, 0}, {10, 10, 10}), results);
    EXPECT_TRUE(results.empty());

    float t;
    Ray ray({0, 0, 0}, {1, 0, 0});
    EXPECT_FALSE(bvh.Raycast(ray, t));
}

TEST(BVHTest, Clear)
{
    BVH<int> bvh;
    std::vector<std::pair<int, AABB3D>> objects;
    objects.push_back({0, AABB3D({0, 0, 0}, {5, 5, 5})});
    bvh.Build(objects);

    bvh.Clear();

    std::vector<int> results;
    bvh.Query(AABB3D({0, 0, 0}, {10, 10, 10}), results);
    EXPECT_TRUE(results.empty());
}
