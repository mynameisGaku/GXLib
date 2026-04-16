#include "pch_graphics.h"
#include "Editor/Gizmo.h"
#include "Editor/EntityPicker.h"
#include <gtest/gtest.h>

using namespace gx;
using namespace DirectX;

class GizmoTest : public ::testing::Test
{
protected:
    Gizmo gizmo;
};

TEST_F(GizmoTest, DefaultMode)
{
    EXPECT_EQ(gizmo.GetMode(), GizmoMode::Translate);
}

TEST_F(GizmoTest, SetMode)
{
    gizmo.SetMode(GizmoMode::Rotate);
    EXPECT_EQ(gizmo.GetMode(), GizmoMode::Rotate);
    gizmo.SetMode(GizmoMode::Scale);
    EXPECT_EQ(gizmo.GetMode(), GizmoMode::Scale);
    gizmo.SetMode(GizmoMode::Translate);
    EXPECT_EQ(gizmo.GetMode(), GizmoMode::Translate);
}

TEST_F(GizmoTest, DefaultScale)
{
    EXPECT_FLOAT_EQ(gizmo.GetScale(), 1.0f);
}

TEST_F(GizmoTest, SetScale)
{
    gizmo.SetScale(2.5f);
    EXPECT_FLOAT_EQ(gizmo.GetScale(), 2.5f);
}

TEST_F(GizmoTest, NotDraggingByDefault)
{
    EXPECT_FALSE(gizmo.IsDragging());
    EXPECT_EQ(gizmo.GetDragAxis(), GizmoAxis::None);
}

TEST_F(GizmoTest, EnableDisable)
{
    EXPECT_TRUE(gizmo.IsEnabled());
    gizmo.SetEnabled(false);
    EXPECT_FALSE(gizmo.IsEnabled());
    gizmo.SetEnabled(true);
    EXPECT_TRUE(gizmo.IsEnabled());
}

TEST_F(GizmoTest, HitTestNoHit)
{
    Transform3D transform;
    // すべての軸から離れる方向のレイ
    Vector3 origin = { 100.0f, 100.0f, 100.0f };
    Vector3 dir = { 0.0f, 0.0f, 1.0f };
    auto axis = gizmo.HitTest(origin, dir, transform);
    EXPECT_EQ(axis, GizmoAxis::None);
}

TEST_F(GizmoTest, BeginEndDrag)
{
    Vector3 origin = { 0.0f, 0.0f, 0.0f };
    Vector3 dir = { 1.0f, 0.0f, 0.0f };
    gizmo.BeginDrag(GizmoAxis::X, origin, dir);
    EXPECT_TRUE(gizmo.IsDragging());
    EXPECT_EQ(gizmo.GetDragAxis(), GizmoAxis::X);
    gizmo.EndDrag();
    EXPECT_FALSE(gizmo.IsDragging());
    EXPECT_EQ(gizmo.GetDragAxis(), GizmoAxis::None);
}

// EntityPickerテスト
TEST(EntityPickerTest, RayAABBHit)
{
    Vector3 origin = { -5.0f, 0.0f, 0.0f };
    Vector3 dir = { 1.0f, 0.0f, 0.0f };
    Vector3 aabbMin = { -1.0f, -1.0f, -1.0f };
    Vector3 aabbMax = { 1.0f, 1.0f, 1.0f };

    float t = EntityPicker::RayAABBIntersect(origin, dir, aabbMin, aabbMax);
    EXPECT_GT(t, 0.0f);
    EXPECT_NEAR(t, 4.0f, 0.01f);
}

TEST(EntityPickerTest, RayAABBMiss)
{
    Vector3 origin = { -5.0f, 5.0f, 0.0f };
    Vector3 dir = { 1.0f, 0.0f, 0.0f };
    Vector3 aabbMin = { -1.0f, -1.0f, -1.0f };
    Vector3 aabbMax = { 1.0f, 1.0f, 1.0f };

    float t = EntityPicker::RayAABBIntersect(origin, dir, aabbMin, aabbMax);
    EXPECT_LT(t, 0.0f);
}

TEST(EntityPickerTest, RayInsideAABB)
{
    Vector3 origin = { 0.0f, 0.0f, 0.0f };
    Vector3 dir = { 1.0f, 0.0f, 0.0f };
    Vector3 aabbMin = { -1.0f, -1.0f, -1.0f };
    Vector3 aabbMax = { 1.0f, 1.0f, 1.0f };

    float t = EntityPicker::RayAABBIntersect(origin, dir, aabbMin, aabbMax);
    EXPECT_GE(t, 0.0f);
}

TEST(EntityPickerTest, ScreenToRay)
{
    // 単位ビュー・プロジェクション行列: 画面中央はおおよそ(0,0,-1)方向を返すはず
    XMMATRIX vp = XMMatrixIdentity();
    Vector3 origin, dir;
    EntityPicker::ScreenToRay(400.0f, 300.0f, 800.0f, 600.0f, vp, origin, dir);
    // 方向はアンプロジェクトに応じて大まかに-Zまたは+Z方向になるはず
    EXPECT_TRUE(std::isfinite(dir.x) && std::isfinite(dir.y) && std::isfinite(dir.z));
}
