/// @file test_LODGroup.cpp
/// @brief LODGroup LOD選択ロジックのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/LODGroup.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"

using namespace gx;

// 識別可能なダミーポインタを使用（デリファレンスしない）
static Model* const dummyLOD0 = reinterpret_cast<Model*>(static_cast<uintptr_t>(0x10));
static Model* const dummyLOD1 = reinterpret_cast<Model*>(static_cast<uintptr_t>(0x20));
static Model* const dummyLOD2 = reinterpret_cast<Model*>(static_cast<uintptr_t>(0x30));

static Camera3D MakeTestCamera(float x, float y, float z)
{
    Camera3D cam;
    cam.SetPerspective(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
    cam.SetPosition(x, y, z);
    return cam;
}

TEST(LODGroupTest, Empty_ReturnsNull)
{
    LODGroup lod;
    Camera3D cam = MakeTestCamera(0, 0, -10);
    Transform3D t;
    EXPECT_EQ(lod.SelectLOD(cam, t, 1.0f), nullptr);
}

TEST(LODGroupTest, AddLevel_Count)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD0, 0.5f);
    lod.AddLevel(dummyLOD1, 0.2f);
    lod.AddLevel(dummyLOD2, 0.05f);
    EXPECT_EQ(lod.GetLevelCount(), 3);
}

TEST(LODGroupTest, AddLevel_SortedByScreenPct)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD2, 0.05f);  // 最低詳細度を最初に
    lod.AddLevel(dummyLOD0, 0.5f);   // 最高詳細度
    lod.AddLevel(dummyLOD1, 0.2f);   // 中間詳細度
    // GetLevel(0)は最大のscreenPercentageであるべき
    EXPECT_GE(lod.GetLevel(0).screenPercentage, lod.GetLevel(1).screenPercentage);
    EXPECT_GE(lod.GetLevel(1).screenPercentage, lod.GetLevel(2).screenPercentage);
}

TEST(LODGroupTest, Clear_RemovesAll)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD0, 0.5f);
    lod.AddLevel(dummyLOD1, 0.2f);
    lod.Clear();
    EXPECT_EQ(lod.GetLevelCount(), 0);
}

TEST(LODGroupTest, SelectLOD_Close)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD0, 0.5f);
    lod.AddLevel(dummyLOD1, 0.2f);
    lod.AddLevel(dummyLOD2, 0.05f);
    Camera3D cam = MakeTestCamera(0, 0, -2);  // 非常に近い
    Transform3D t;
    t.SetPosition(0, 0, 0);
    Model* selected = lod.SelectLOD(cam, t, 1.0f);
    // 近いときはLOD0（最高詳細度）を選択すべき
    EXPECT_EQ(selected, dummyLOD0);
}

TEST(LODGroupTest, SelectLOD_Far)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD0, 0.5f);
    lod.AddLevel(dummyLOD1, 0.2f);
    lod.AddLevel(dummyLOD2, 0.05f);
    Camera3D cam = MakeTestCamera(0, 0, -500);  // 非常に遠い
    Transform3D t;
    t.SetPosition(0, 0, 0);
    Model* selected = lod.SelectLOD(cam, t, 1.0f);
    // 遠いときは最低LODを選択すべき
    EXPECT_EQ(selected, dummyLOD2);
}

TEST(LODGroupTest, SelectLOD_CullDistance)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD0, 0.5f);
    lod.SetCullDistance(50.0f);
    Camera3D cam = MakeTestCamera(0, 0, -100);  // カリング距離を超えている
    Transform3D t;
    t.SetPosition(0, 0, 0);
    Model* selected = lod.SelectLOD(cam, t, 1.0f);
    EXPECT_EQ(selected, nullptr);
}

TEST(LODGroupTest, SelectLOD_NoCullDistance)
{
    LODGroup lod;
    lod.AddLevel(dummyLOD0, 0.5f);
    lod.SetCullDistance(0.0f);  // カリングなし
    Camera3D cam = MakeTestCamera(0, 0, -100);
    Transform3D t;
    t.SetPosition(0, 0, 0);
    Model* selected = lod.SelectLOD(cam, t, 1.0f);
    EXPECT_NE(selected, nullptr);
}
