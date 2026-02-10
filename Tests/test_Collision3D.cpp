/// @file test_Collision3D.cpp
/// @brief 3D 衝突判定 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Collision/Collision3D.h"

using namespace GX;

// ============================================================================
// AABB3D vs AABB3D（軸平行境界ボックス同士）
// ============================================================================

TEST(Collision3DTest, AABBvsAABB_Overlap)
{
    AABB3D a({0, 0, 0}, {10, 10, 10});
    AABB3D b({5, 5, 5}, {15, 15, 15});
    EXPECT_TRUE(Collision3D::TestAABBVsAABB(a, b));
}

TEST(Collision3DTest, AABBvsAABB_Separated)
{
    AABB3D a({0, 0, 0}, {5, 5, 5});
    AABB3D b({10, 10, 10}, {15, 15, 15});
    EXPECT_FALSE(Collision3D::TestAABBVsAABB(a, b));
}

TEST(Collision3DTest, AABBvsAABB_Touching)
{
    AABB3D a({0, 0, 0}, {5, 5, 5});
    AABB3D b({5, 0, 0}, {10, 5, 5});
    EXPECT_TRUE(Collision3D::TestAABBVsAABB(a, b));
}

// ============================================================================
// Sphere vs Sphere（球同士）
// ============================================================================

TEST(Collision3DTest, SphereVsSphere_Overlap)
{
    Sphere a({0, 0, 0}, 5.0f);
    Sphere b({7, 0, 0}, 5.0f);
    EXPECT_TRUE(Collision3D::TestSphereVsSphere(a, b));
}

TEST(Collision3DTest, SphereVsSphere_Separated)
{
    Sphere a({0, 0, 0}, 2.0f);
    Sphere b({10, 0, 0}, 2.0f);
    EXPECT_FALSE(Collision3D::TestSphereVsSphere(a, b));
}

TEST(Collision3DTest, SphereVsSphere_Touching)
{
    Sphere a({0, 0, 0}, 5.0f);
    Sphere b({10, 0, 0}, 5.0f);
    EXPECT_TRUE(Collision3D::TestSphereVsSphere(a, b));
}

// ============================================================================
// Sphere vs AABB（球とボックス）
// ============================================================================

TEST(Collision3DTest, SphereVsAABB_Overlap)
{
    Sphere sphere({12, 5, 5}, 3.0f);
    AABB3D aabb({0, 0, 0}, {10, 10, 10});
    EXPECT_TRUE(Collision3D::TestSphereVsAABB(sphere, aabb));
}

TEST(Collision3DTest, SphereVsAABB_Separated)
{
    Sphere sphere({20, 20, 20}, 2.0f);
    AABB3D aabb({0, 0, 0}, {10, 10, 10});
    EXPECT_FALSE(Collision3D::TestSphereVsAABB(sphere, aabb));
}

// ============================================================================
// 点の判定
// ============================================================================

TEST(Collision3DTest, PointInSphere)
{
    Sphere sphere({0, 0, 0}, 5.0f);
    EXPECT_TRUE(Collision3D::TestPointInSphere({1, 1, 1}, sphere));
    EXPECT_FALSE(Collision3D::TestPointInSphere({10, 0, 0}, sphere));
}

TEST(Collision3DTest, PointInAABB)
{
    AABB3D aabb({0, 0, 0}, {10, 10, 10});
    EXPECT_TRUE(Collision3D::TestPointInAABB({5, 5, 5}, aabb));
    EXPECT_FALSE(Collision3D::TestPointInAABB({-1, 5, 5}, aabb));
}

// ============================================================================
// レイとプリミティブ
// ============================================================================

TEST(Collision3DTest, RaycastSphere_Hit)
{
    Ray ray({-10, 0, 0}, {1, 0, 0});
    Sphere sphere({0, 0, 0}, 3.0f);
    float t;
    EXPECT_TRUE(Collision3D::RaycastSphere(ray, sphere, t));
    EXPECT_NEAR(t, 7.0f, 1e-3f); // x = -3 でヒット
}

TEST(Collision3DTest, RaycastSphere_Miss)
{
    Ray ray({-10, 10, 0}, {1, 0, 0});
    Sphere sphere({0, 0, 0}, 3.0f);
    float t;
    EXPECT_FALSE(Collision3D::RaycastSphere(ray, sphere, t));
}

TEST(Collision3DTest, RaycastAABB_Hit)
{
    Ray ray({-10, 5, 5}, {1, 0, 0});
    AABB3D aabb({0, 0, 0}, {10, 10, 10});
    float t;
    EXPECT_TRUE(Collision3D::RaycastAABB(ray, aabb, t));
    EXPECT_NEAR(t, 10.0f, 1e-3f);
}

TEST(Collision3DTest, RaycastAABB_Miss)
{
    Ray ray({-10, 20, 0}, {1, 0, 0});
    AABB3D aabb({0, 0, 0}, {10, 10, 10});
    float t;
    EXPECT_FALSE(Collision3D::RaycastAABB(ray, aabb, t));
}

TEST(Collision3DTest, RaycastPlane_Hit)
{
    Ray ray({0, 5, 0}, {0, -1, 0});
    Plane plane({0, 1, 0}, 0.0f); // Y=0 平面
    float t;
    EXPECT_TRUE(Collision3D::RaycastPlane(ray, plane, t));
    EXPECT_NEAR(t, 5.0f, 1e-3f);
}

TEST(Collision3DTest, RaycastTriangle_Hit)
{
    Triangle tri({-5, 0, 0}, {5, 0, 0}, {0, 0, 5});
    Ray ray({0, 5, 1}, {0, -1, 0});
    float t, u, v;
    EXPECT_TRUE(Collision3D::RaycastTriangle(ray, tri, t, u, v));
    EXPECT_NEAR(t, 5.0f, 1e-3f);
}

TEST(Collision3DTest, RaycastTriangle_Miss)
{
    Triangle tri({-5, 0, 0}, {5, 0, 0}, {0, 0, 5});
    Ray ray({100, 5, 100}, {0, -1, 0});
    float t, u, v;
    EXPECT_FALSE(Collision3D::RaycastTriangle(ray, tri, t, u, v));
}

// ============================================================================
// OBB vs OBB（分離軸判定: 15軸）
// ============================================================================

TEST(Collision3DTest, OBBvsOBB_Overlap)
{
    Matrix4x4 identity = Matrix4x4::Identity();
    OBB a({0, 0, 0}, {5, 5, 5}, identity);
    OBB b({7, 0, 0}, {5, 5, 5}, identity);
    EXPECT_TRUE(Collision3D::TestOBBVsOBB(a, b));
}

TEST(Collision3DTest, OBBvsOBB_Separated)
{
    Matrix4x4 identity = Matrix4x4::Identity();
    OBB a({0, 0, 0}, {2, 2, 2}, identity);
    OBB b({20, 0, 0}, {2, 2, 2}, identity);
    EXPECT_FALSE(Collision3D::TestOBBVsOBB(a, b));
}

TEST(Collision3DTest, OBBvsOBB_Rotated)
{
    Matrix4x4 identity = Matrix4x4::Identity();
    Matrix4x4 rot45 = Matrix4x4::RotationY(MathUtil::PI / 4.0f);
    OBB a({0, 0, 0}, {5, 5, 5}, identity);
    OBB b({9, 0, 0}, {5, 5, 5}, rot45);
    EXPECT_TRUE(Collision3D::TestOBBVsOBB(a, b));
}

// ============================================================================
// Frustum（視錐台）
// ============================================================================

TEST(Collision3DTest, FrustumVsSphere)
{
    XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0, 0, -10, 1),
        XMVectorSet(0, 0, 0, 1),
        XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        MathUtil::PI / 4.0f, 1.0f, 0.1f, 100.0f);
    Frustum frustum = Frustum::FromViewProjection(XMMatrixMultiply(view, proj));

    // 原点の球は視錐台内のはず
    Sphere inside({0, 0, 0}, 1.0f);
    EXPECT_TRUE(Collision3D::TestFrustumVsSphere(frustum, inside));

    // カメラのはるか後ろは外
    Sphere behind({0, 0, -100}, 1.0f);
    EXPECT_FALSE(Collision3D::TestFrustumVsSphere(frustum, behind));
}

TEST(Collision3DTest, FrustumVsAABB)
{
    XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0, 0, -10, 1),
        XMVectorSet(0, 0, 0, 1),
        XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        MathUtil::PI / 4.0f, 1.0f, 0.1f, 100.0f);
    Frustum frustum = Frustum::FromViewProjection(XMMatrixMultiply(view, proj));

    AABB3D inside({-1, -1, -1}, {1, 1, 1});
    EXPECT_TRUE(Collision3D::TestFrustumVsAABB(frustum, inside));

    AABB3D outside({100, 100, 100}, {110, 110, 110});
    EXPECT_FALSE(Collision3D::TestFrustumVsAABB(frustum, outside));
}

// ============================================================================
// 最近接点ヘルパー
// ============================================================================

TEST(Collision3DTest, ClosestPointOnAABB)
{
    AABB3D aabb({0, 0, 0}, {10, 10, 10});
    Vector3 outside(15, 5, 5);
    Vector3 closest = Collision3D::ClosestPointOnAABB(outside, aabb);
    EXPECT_NEAR(closest.x, 10.0f, 1e-5f);
    EXPECT_NEAR(closest.y, 5.0f, 1e-5f);
    EXPECT_NEAR(closest.z, 5.0f, 1e-5f);
}

// ============================================================================
// 交差情報（深さ/法線あり）
// ============================================================================

TEST(Collision3DTest, IntersectSphereVsSphere)
{
    Sphere a({0, 0, 0}, 5.0f);
    Sphere b({7, 0, 0}, 5.0f);
    auto hit = Collision3D::IntersectSphereVsSphere(a, b);
    EXPECT_TRUE(hit.hit);
    EXPECT_NEAR(hit.depth, 3.0f, 1e-3f);
}
