/// @file test_Camera2D.cpp
/// @brief Camera2D 位置/ズーム/回転/VP行列のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/Rendering/Camera2D.h"

using namespace gx;

static constexpr float kEps = 1e-5f;
static constexpr float kPi  = 3.14159265f;

// ==================== デフォルト値 ====================

TEST(Camera2DTest, Default_Position)
{
    Camera2D cam;
    EXPECT_NEAR(cam.GetPositionX(), 0.0f, kEps);
    EXPECT_NEAR(cam.GetPositionY(), 0.0f, kEps);
}

TEST(Camera2DTest, Default_Zoom)
{
    Camera2D cam;
    EXPECT_NEAR(cam.GetZoom(), 1.0f, kEps);
}

TEST(Camera2DTest, Default_Rotation)
{
    Camera2D cam;
    EXPECT_NEAR(cam.GetRotation(), 0.0f, kEps);
}

// ==================== セッター ====================

TEST(Camera2DTest, SetPosition)
{
    Camera2D cam;
    cam.SetPosition(100.0f, -200.0f);
    EXPECT_NEAR(cam.GetPositionX(), 100.0f, kEps);
    EXPECT_NEAR(cam.GetPositionY(), -200.0f, kEps);
}

TEST(Camera2DTest, SetZoom)
{
    Camera2D cam;
    cam.SetZoom(2.5f);
    EXPECT_NEAR(cam.GetZoom(), 2.5f, kEps);
}

TEST(Camera2DTest, SetRotation)
{
    Camera2D cam;
    cam.SetRotation(1.57f);
    EXPECT_NEAR(cam.GetRotation(), 1.57f, kEps);
}

TEST(Camera2DTest, SetPosition_Multiple)
{
    Camera2D cam;
    cam.SetPosition(10.0f, 20.0f);
    cam.SetPosition(30.0f, 40.0f);
    EXPECT_NEAR(cam.GetPositionX(), 30.0f, kEps);
    EXPECT_NEAR(cam.GetPositionY(), 40.0f, kEps);
}

// ==================== VP行列 ====================

TEST(Camera2DTest, VP_Default_IsOrthographic)
{
    Camera2D cam;
    XMMATRIX vp = cam.GetViewProjectionMatrix(800, 600);
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, vp);
    // 原点にあるzoom=1のデフォルトカメラは標準正射影を生成すべき
    // 行列がすべてゼロであってはならない
    bool allZero = true;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (fabsf((&m._11)[i * 4 + j]) > kEps) allZero = false;
    EXPECT_FALSE(allZero);
}

TEST(Camera2DTest, VP_OriginMapsToTopLeft)
{
    Camera2D cam;
    XMMATRIX vp = cam.GetViewProjectionMatrix(800, 600);
    // 原点をVP行列で変換
    XMVECTOR origin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR result = XMVector4Transform(origin, vp);
    XMFLOAT4 r;
    XMStoreFloat4(&r, result);
    // 2D正射影: 原点は左上(-1, 1)にマッピングされる
    EXPECT_NEAR(r.x, -1.0f, 0.01f);
    EXPECT_NEAR(r.y, 1.0f, 0.01f);
}

TEST(Camera2DTest, VP_ZoomAffectsScale)
{
    Camera2D cam1;
    cam1.SetZoom(1.0f);
    XMMATRIX vp1 = cam1.GetViewProjectionMatrix(800, 600);

    Camera2D cam2;
    cam2.SetZoom(2.0f);
    XMMATRIX vp2 = cam2.GetViewProjectionMatrix(800, 600);

    // (100,0)の点はzoom=2で中心からより遠くに表示されるべき
    XMVECTOR pt = XMVectorSet(100.0f, 0.0f, 0.0f, 1.0f);
    XMFLOAT4 r1, r2;
    XMStoreFloat4(&r1, XMVector4Transform(pt, vp1));
    XMStoreFloat4(&r2, XMVector4Transform(pt, vp2));
    EXPECT_GT(fabsf(r2.x), fabsf(r1.x) * 1.5f);  // 2倍ズームでスクリーン位置はおよそ2倍になる
}

TEST(Camera2DTest, VP_TranslationMovesView)
{
    Camera2D cam;
    cam.SetPosition(400.0f, 300.0f);  // 800x600の中心
    XMMATRIX vp = cam.GetViewProjectionMatrix(800, 600);
    // カメラ位置はビューの原点にマッピングされる
    // カメラが(400,300)にあるとき、(400,300)の点は左上にマッピングされるべき
    XMVECTOR pt = XMVectorSet(400.0f, 300.0f, 0.0f, 1.0f);
    XMFLOAT4 r;
    XMStoreFloat4(&r, XMVector4Transform(pt, vp));
    EXPECT_NEAR(r.x, -1.0f, 0.01f);
}

TEST(Camera2DTest, VP_DifferentScreenSizes)
{
    Camera2D cam;
    XMMATRIX vp1 = cam.GetViewProjectionMatrix(1920, 1080);
    XMMATRIX vp2 = cam.GetViewProjectionMatrix(800, 600);
    XMFLOAT4X4 m1, m2;
    XMStoreFloat4x4(&m1, vp1);
    XMStoreFloat4x4(&m2, vp2);
    // 異なるスクリーンサイズは異なる行列を生成すべき
    bool different = fabsf(m1._11 - m2._11) > kEps || fabsf(m1._22 - m2._22) > kEps;
    EXPECT_TRUE(different);
}

TEST(Camera2DTest, VP_RotationAffectsMatrix)
{
    Camera2D cam1;
    cam1.SetRotation(0.0f);
    XMMATRIX vp1 = cam1.GetViewProjectionMatrix(800, 600);

    Camera2D cam2;
    cam2.SetRotation(kPi / 4.0f);
    XMMATRIX vp2 = cam2.GetViewProjectionMatrix(800, 600);

    XMFLOAT4X4 m1, m2;
    XMStoreFloat4x4(&m1, vp1);
    XMStoreFloat4x4(&m2, vp2);
    // 回転したカメラは異なる行列を生成すべき
    bool different = false;
    for (int i = 0; i < 16; ++i)
        if (fabsf((&m1._11)[i] - (&m2._11)[i]) > kEps) different = true;
    EXPECT_TRUE(different);
}

// ==================== エッジケース ====================

TEST(Camera2DTest, Zoom_VerySmall)
{
    Camera2D cam;
    cam.SetZoom(0.01f);
    EXPECT_NEAR(cam.GetZoom(), 0.01f, kEps);
    // それでも有効な行列を生成すべき
    XMMATRIX vp = cam.GetViewProjectionMatrix(800, 600);
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, vp);
    EXPECT_FALSE(std::isnan(m._11));
}

TEST(Camera2DTest, Zoom_VeryLarge)
{
    Camera2D cam;
    cam.SetZoom(100.0f);
    EXPECT_NEAR(cam.GetZoom(), 100.0f, kEps);
}

TEST(Camera2DTest, Position_Negative)
{
    Camera2D cam;
    cam.SetPosition(-1000.0f, -2000.0f);
    EXPECT_NEAR(cam.GetPositionX(), -1000.0f, kEps);
    EXPECT_NEAR(cam.GetPositionY(), -2000.0f, kEps);
}
