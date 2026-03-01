/// @file test_CollisionExtended.cpp
/// @brief カプセル、OBB MTV、GJK/EPA衝突テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Collision/Collision3D.h"

using namespace gx;

// ============================================================================
// カプセル構造体
// ============================================================================

TEST(CapsuleTest, Center)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    auto center = c.Center();
    EXPECT_NEAR(center.x, 0.0f, 1e-5f);
    EXPECT_NEAR(center.y, 2.0f, 1e-5f);
    EXPECT_NEAR(center.z, 0.0f, 1e-5f);
}

TEST(CapsuleTest, Height)
{
    Capsule c({0, 0, 0}, {0, 6, 0}, 1.0f);
    // Height() = セグメント長 + 2*半径 = 6 + 2 = 8
    EXPECT_NEAR(c.Height(), 8.0f, 1e-5f);
}

// ============================================================================
// カプセル vs カプセル
// ============================================================================

TEST(CapsuleTest, CapsuleVsCapsule_Overlap)
{
    Capsule a({0, 0, 0}, {0, 4, 0}, 1.0f);
    Capsule b({1.5f, 0, 0}, {1.5f, 4, 0}, 1.0f);
    EXPECT_TRUE(Collision3D::TestCapsuleVsCapsule(a, b));
}

TEST(CapsuleTest, CapsuleVsCapsule_Separated)
{
    Capsule a({0, 0, 0}, {0, 4, 0}, 1.0f);
    Capsule b({5.0f, 0, 0}, {5.0f, 4, 0}, 1.0f);
    EXPECT_FALSE(Collision3D::TestCapsuleVsCapsule(a, b));
}

TEST(CapsuleTest, CapsuleVsCapsule_Touching)
{
    Capsule a({0, 0, 0}, {0, 4, 0}, 1.0f);
    Capsule b({2.0f, 0, 0}, {2.0f, 4, 0}, 1.0f);
    EXPECT_TRUE(Collision3D::TestCapsuleVsCapsule(a, b));
}

TEST(CapsuleTest, CapsuleVsCapsule_Perpendicular)
{
    // 垂直カプセルと水平カプセルの交差
    Capsule a({0, 0, 0}, {0, 4, 0}, 0.5f);
    Capsule b({-2, 2, 0}, {2, 2, 0}, 0.5f);
    EXPECT_TRUE(Collision3D::TestCapsuleVsCapsule(a, b));
}

// ============================================================================
// カプセル vs 球
// ============================================================================

TEST(CapsuleTest, CapsuleVsSphere_Overlap)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Sphere s({1.5f, 2, 0}, 1.0f);
    EXPECT_TRUE(Collision3D::TestCapsuleVsSphere(c, s));
}

TEST(CapsuleTest, CapsuleVsSphere_Separated)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Sphere s({10.0f, 2, 0}, 1.0f);
    EXPECT_FALSE(Collision3D::TestCapsuleVsSphere(c, s));
}

TEST(CapsuleTest, CapsuleVsSphere_NearEnd)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Sphere s({0, 5.5f, 0}, 1.0f);
    EXPECT_TRUE(Collision3D::TestCapsuleVsSphere(c, s));
}

// ============================================================================
// カプセル vs AABB
// ============================================================================

TEST(CapsuleTest, CapsuleVsAABB_Overlap)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    AABB3D aabb({-2, -2, -2}, {0.5f, 2, 0.5f});
    EXPECT_TRUE(Collision3D::TestCapsuleVsAABB(c, aabb));
}

TEST(CapsuleTest, CapsuleVsAABB_Separated)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    AABB3D aabb({10, 10, 10}, {15, 15, 15});
    EXPECT_FALSE(Collision3D::TestCapsuleVsAABB(c, aabb));
}

// ============================================================================
// カプセルレイキャスト
// ============================================================================

TEST(CapsuleTest, RaycastCapsule_Hit)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Ray ray;
    ray.origin = {-5, 2, 0};
    ray.direction = {1, 0, 0};
    float t = 0;
    EXPECT_TRUE(Collision3D::RaycastCapsule(ray, c, t));
    EXPECT_GT(t, 0.0f);
    EXPECT_LT(t, 10.0f);
}

TEST(CapsuleTest, RaycastCapsule_Miss)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Ray ray;
    ray.origin = {-5, 10, 0};
    ray.direction = {1, 0, 0};
    float t = 0;
    EXPECT_FALSE(Collision3D::RaycastCapsule(ray, c, t));
}

TEST(CapsuleTest, RaycastCapsule_HitCap)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    // 上端キャップに向かって下向きのレイ
    Ray ray;
    ray.origin = {0, 10, 0};
    ray.direction = {0, -1, 0};
    float t = 0;
    EXPECT_TRUE(Collision3D::RaycastCapsule(ray, c, t));
    // カプセル上端でヒット: y = 4 + 半径 = 5、したがって t ≈ 5
    EXPECT_NEAR(t, 5.0f, 0.5f);
}

// ============================================================================
// IntersectCapsuleVsSphere（MTV）
// ============================================================================

TEST(CapsuleTest, IntersectCapsuleVsSphere_Hit)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Sphere s({1.5f, 2, 0}, 1.0f);
    auto result = Collision3D::IntersectCapsuleVsSphere(c, s);
    EXPECT_TRUE(result.hit);
    EXPECT_GT(result.depth, 0.0f);
}

TEST(CapsuleTest, IntersectCapsuleVsSphere_NoHit)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Sphere s({10.0f, 2, 0}, 1.0f);
    auto result = Collision3D::IntersectCapsuleVsSphere(c, s);
    EXPECT_FALSE(result.hit);
}

// ============================================================================
// カプセル上の最近傍点
// ============================================================================

TEST(CapsuleTest, ClosestPointOnCapsule_Side)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Vector3 pt = Collision3D::ClosestPointOnCapsule({5, 2, 0}, c);
    // 側面上の最近傍点は x=1（半径）、y=2 のはず
    EXPECT_NEAR(pt.x, 1.0f, 0.1f);
    EXPECT_NEAR(pt.y, 2.0f, 0.1f);
    EXPECT_NEAR(pt.z, 0.0f, 0.1f);
}

TEST(CapsuleTest, ClosestPointOnCapsule_End)
{
    Capsule c({0, 0, 0}, {0, 4, 0}, 1.0f);
    Vector3 pt = Collision3D::ClosestPointOnCapsule({0, 10, 0}, c);
    // 最近傍は上端キャップ: (0, 5, 0)
    EXPECT_NEAR(pt.y, 5.0f, 0.1f);
}

// ============================================================================
// OBB vs OBB MTV（IntersectOBBVsOBB）
// ============================================================================

TEST(OBBExtendedTest, IntersectOBBVsOBB_Overlap)
{
    OBB a({0, 0, 0}, {2, 2, 2}, Matrix4x4{});  // 単位回転
    OBB b({3, 0, 0}, {2, 2, 2}, Matrix4x4{});
    auto result = Collision3D::IntersectOBBVsOBB(a, b);
    EXPECT_TRUE(result.hit);
    EXPECT_GT(result.depth, 0.0f);
    // 貫通深度は約1.0のはず（半径+半径-距離 = 2+2-3 = 1）
    EXPECT_NEAR(result.depth, 1.0f, 0.2f);
}

TEST(OBBExtendedTest, IntersectOBBVsOBB_Separated)
{
    OBB a({0, 0, 0}, {2, 2, 2}, Matrix4x4{});
    OBB b({10, 0, 0}, {2, 2, 2}, Matrix4x4{});
    auto result = Collision3D::IntersectOBBVsOBB(a, b);
    EXPECT_FALSE(result.hit);
}

TEST(OBBExtendedTest, IntersectOBBVsOBB_NormalDirection)
{
    OBB a({0, 0, 0}, {2, 2, 2}, Matrix4x4{});
    OBB b({3, 0, 0}, {2, 2, 2}, Matrix4x4{});
    auto result = Collision3D::IntersectOBBVsOBB(a, b);
    EXPECT_TRUE(result.hit);
    // 押し出し法線はおおよそX軸方向（AからBへの方向）を向くはず
    float nx = std::abs(result.normal.x);
    EXPECT_GT(nx, 0.5f);
}

// ============================================================================
// GJK/EPA
// ============================================================================

// 軸平行ボックスのサポート関数
static auto MakeBoxSupport(const Vector3& center, const Vector3& halfExtents)
{
    return [center, halfExtents](const Vector3& dir) -> Vector3
    {
        return {
            center.x + (dir.x >= 0.0f ? halfExtents.x : -halfExtents.x),
            center.y + (dir.y >= 0.0f ? halfExtents.y : -halfExtents.y),
            center.z + (dir.z >= 0.0f ? halfExtents.z : -halfExtents.z)
        };
    };
}

TEST(GJKTest, TestGJK_BoxesOverlap)
{
    // 非退化シンプレックスを生成するため非軸平行オフセットを使用
    auto supportA = MakeBoxSupport({0, 0, 0}, {2, 2, 2});
    auto supportB = MakeBoxSupport({2, 1, 0.5f}, {2, 2, 2});
    EXPECT_TRUE(Collision3D::TestGJK(supportA, supportB));
}

TEST(GJKTest, TestGJK_BoxesSeparated)
{
    auto supportA = MakeBoxSupport({0, 0, 0}, {2, 2, 2});
    auto supportB = MakeBoxSupport({10, 5, 3}, {2, 2, 2});
    EXPECT_FALSE(Collision3D::TestGJK(supportA, supportB));
}

TEST(GJKTest, TestGJK_BoxesContained)
{
    // 小さなボックスが大きなボックスの完全に内側
    auto supportA = MakeBoxSupport({0, 0, 0}, {5, 5, 5});
    auto supportB = MakeBoxSupport({1, 1, 1}, {1, 1, 1});
    EXPECT_TRUE(Collision3D::TestGJK(supportA, supportB));
}

TEST(GJKTest, IntersectGJKEPA_BoxesOverlap)
{
    // GJKの確実な収束のため非軸平行オーバーラップを使用
    auto supportA = MakeBoxSupport({0, 0, 0}, {2, 2, 2});
    auto supportB = MakeBoxSupport({2, 1, 0.5f}, {2, 2, 2});
    auto result = Collision3D::IntersectGJKEPA(supportA, supportB);
    EXPECT_TRUE(result.hit);
    EXPECT_GT(result.depth, 0.0f);
}

TEST(GJKTest, IntersectGJKEPA_BoxesSeparated)
{
    auto supportA = MakeBoxSupport({0, 0, 0}, {2, 2, 2});
    auto supportB = MakeBoxSupport({10, 5, 3}, {2, 2, 2});
    auto result = Collision3D::IntersectGJKEPA(supportA, supportB);
    EXPECT_FALSE(result.hit);
}

TEST(GJKTest, IntersectGJKEPA_BoxesDeepOverlap)
{
    // 大きく重なる2つのボックス
    auto supportA = MakeBoxSupport({0, 0, 0}, {3, 3, 3});
    auto supportB = MakeBoxSupport({2, 1, 0.5f}, {3, 3, 3});
    auto result = Collision3D::IntersectGJKEPA(supportA, supportB);
    EXPECT_TRUE(result.hit);
    EXPECT_GT(result.depth, 0.0f);
}
