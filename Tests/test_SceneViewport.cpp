/// @file test_SceneViewport.cpp
/// @brief シーンビューポートエディタのテスト

#include <gtest/gtest.h>
#include "Editor/SceneViewport.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include <cmath>

using namespace gx;

// ============================================================================
// SceneViewport テスト
// ============================================================================

class SceneViewportTest : public ::testing::Test
{
protected:
    SceneViewport viewport;
    Scene scene{"TestScene"};

    void SetUp() override
    {
        viewport.SetScene(&scene);
    }
};

TEST_F(SceneViewportTest, DefaultTool)
{
    EXPECT_EQ(viewport.GetActiveTool(), ViewportTool::Select);
}

TEST_F(SceneViewportTest, SetActiveTool)
{
    viewport.SetActiveTool(ViewportTool::Move);
    EXPECT_EQ(viewport.GetActiveTool(), ViewportTool::Move);

    viewport.SetActiveTool(ViewportTool::Rotate);
    EXPECT_EQ(viewport.GetActiveTool(), ViewportTool::Rotate);

    viewport.SetActiveTool(ViewportTool::Scale);
    EXPECT_EQ(viewport.GetActiveTool(), ViewportTool::Scale);

    viewport.SetActiveTool(ViewportTool::Rect);
    EXPECT_EQ(viewport.GetActiveTool(), ViewportTool::Rect);
}

TEST_F(SceneViewportTest, DefaultViewMode)
{
    EXPECT_EQ(viewport.GetViewMode(), ViewportMode::Perspective);
}

TEST_F(SceneViewportTest, SetViewMode)
{
    viewport.SetViewMode(ViewportMode::Top);
    EXPECT_EQ(viewport.GetViewMode(), ViewportMode::Top);

    viewport.SetViewMode(ViewportMode::Front);
    EXPECT_EQ(viewport.GetViewMode(), ViewportMode::Front);

    viewport.SetViewMode(ViewportMode::Right);
    EXPECT_EQ(viewport.GetViewMode(), ViewportMode::Right);

    viewport.SetViewMode(ViewportMode::Orthographic);
    EXPECT_EQ(viewport.GetViewMode(), ViewportMode::Orthographic);
}

TEST_F(SceneViewportTest, DefaultCamera)
{
    const auto& cam = viewport.GetCamera();
    EXPECT_FLOAT_EQ(cam.fov, 60.0f);
    EXPECT_FLOAT_EQ(cam.nearPlane, 0.1f);
    EXPECT_FLOAT_EQ(cam.farPlane, 1000.0f);
}

TEST_F(SceneViewportTest, SetCamera)
{
    ViewportCamera cam;
    cam.fov = 90.0f;
    cam.position[0] = 5.0f;
    cam.position[1] = 10.0f;
    cam.position[2] = -20.0f;
    viewport.SetCamera(cam);

    const auto& result = viewport.GetCamera();
    EXPECT_FLOAT_EQ(result.fov, 90.0f);
    EXPECT_FLOAT_EQ(result.position[0], 5.0f);
    EXPECT_FLOAT_EQ(result.position[1], 10.0f);
    EXPECT_FLOAT_EQ(result.position[2], -20.0f);
}

TEST_F(SceneViewportTest, GridConfig)
{
    GridConfig cfg;
    cfg.cellSize = 2.0f;
    cfg.visible = false;
    cfg.majorLineEvery = 5;
    viewport.SetGridConfig(cfg);

    const auto& result = viewport.GetGridConfig();
    EXPECT_FLOAT_EQ(result.cellSize, 2.0f);
    EXPECT_FALSE(result.visible);
    EXPECT_EQ(result.majorLineEvery, 5);
}

TEST_F(SceneViewportTest, SelectEntity)
{
    auto* e = scene.CreateEntity("Player");
    viewport.Select(e->GetID());
    EXPECT_TRUE(viewport.IsSelected(e->GetID()));
    EXPECT_EQ(viewport.GetSelectedCount(), 1u);
}

TEST_F(SceneViewportTest, DeselectEntity)
{
    auto* e = scene.CreateEntity("Player");
    viewport.Select(e->GetID());
    viewport.Deselect(e->GetID());
    EXPECT_FALSE(viewport.IsSelected(e->GetID()));
    EXPECT_EQ(viewport.GetSelectedCount(), 0u);
}

TEST_F(SceneViewportTest, ClearSelection)
{
    auto* e1 = scene.CreateEntity("A");
    auto* e2 = scene.CreateEntity("B");
    viewport.Select(e1->GetID());
    viewport.Select(e2->GetID());
    EXPECT_EQ(viewport.GetSelectedCount(), 2u);

    viewport.ClearSelection();
    EXPECT_EQ(viewport.GetSelectedCount(), 0u);
}

TEST_F(SceneViewportTest, IsSelected)
{
    auto* e = scene.CreateEntity("X");
    EXPECT_FALSE(viewport.IsSelected(e->GetID()));
    viewport.Select(e->GetID());
    EXPECT_TRUE(viewport.IsSelected(e->GetID()));
}

TEST_F(SceneViewportTest, GetSelectedCount)
{
    EXPECT_EQ(viewport.GetSelectedCount(), 0u);
    auto* e1 = scene.CreateEntity("A");
    auto* e2 = scene.CreateEntity("B");
    viewport.Select(e1->GetID());
    EXPECT_EQ(viewport.GetSelectedCount(), 1u);
    viewport.Select(e2->GetID());
    EXPECT_EQ(viewport.GetSelectedCount(), 2u);
}

TEST_F(SceneViewportTest, MultiSelect)
{
    auto* e1 = scene.CreateEntity("A");
    auto* e2 = scene.CreateEntity("B");
    auto* e3 = scene.CreateEntity("C");

    viewport.Select(e1->GetID());
    viewport.Select(e2->GetID());
    viewport.Select(e3->GetID());

    EXPECT_EQ(viewport.GetSelectedCount(), 3u);
    EXPECT_TRUE(viewport.IsSelected(e1->GetID()));
    EXPECT_TRUE(viewport.IsSelected(e2->GetID()));
    EXPECT_TRUE(viewport.IsSelected(e3->GetID()));
}

TEST_F(SceneViewportTest, SelectAll)
{
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    scene.CreateEntity("C");

    viewport.SelectAll();
    EXPECT_EQ(viewport.GetSelectedCount(), 3u);
}

TEST_F(SceneViewportTest, SelectAllEmptyScene)
{
    Scene emptyScene{"Empty"};
    SceneViewport vp;
    vp.SetScene(&emptyScene);
    vp.SelectAll();
    EXPECT_EQ(vp.GetSelectedCount(), 0u);
}

TEST_F(SceneViewportTest, DragDropBeginEnd)
{
    DragDropPayload payload;
    payload.type = DragDropPayload::Type::Asset;
    payload.name = "model.fbx";

    viewport.BeginDragDrop(payload);
    EXPECT_TRUE(viewport.IsDragging());

    viewport.EndDragDrop();
    EXPECT_FALSE(viewport.IsDragging());
}

TEST_F(SceneViewportTest, IsDraggingFalse)
{
    EXPECT_FALSE(viewport.IsDragging());
}

TEST_F(SceneViewportTest, ScreenToWorldBasic)
{
    viewport.SetViewportSize(1280, 720);

    float wx, wy, wz;
    // 画面中央をアンプロジェクト
    viewport.ScreenToWorld(640.0f, 360.0f, wx, wy, wz);
    // カメラ位置のX付近に投影されるはず
    EXPECT_NEAR(wx, viewport.GetCamera().position[0], 1.0f);
}

TEST_F(SceneViewportTest, ViewportSize)
{
    viewport.SetViewportSize(1920, 1080);
    EXPECT_EQ(viewport.GetViewportWidth(), 1920u);
    EXPECT_EQ(viewport.GetViewportHeight(), 1080u);
}

TEST_F(SceneViewportTest, OrbitCamera)
{
    float initialYaw = viewport.GetCamera().rotation[1];
    float initialPitch = viewport.GetCamera().rotation[0];

    viewport.Orbit(10.0f, 5.0f);

    EXPECT_NE(viewport.GetCamera().rotation[1], initialYaw);
    EXPECT_NE(viewport.GetCamera().rotation[0], initialPitch);
}

TEST_F(SceneViewportTest, PanCamera)
{
    float initialX = viewport.GetCamera().position[0];
    float initialY = viewport.GetCamera().position[1];

    viewport.Pan(1.0f, 1.0f);

    EXPECT_NE(viewport.GetCamera().position[0], initialX);
    EXPECT_NE(viewport.GetCamera().position[1], initialY);
}

TEST_F(SceneViewportTest, ZoomCamera)
{
    float initialZ = viewport.GetCamera().position[2];

    viewport.Zoom(1.0f);

    EXPECT_NE(viewport.GetCamera().position[2], initialZ);
}

TEST_F(SceneViewportTest, SnappingToggle)
{
    EXPECT_FALSE(viewport.IsSnapping());

    viewport.SetSnapping(true, 0.5f);
    EXPECT_TRUE(viewport.IsSnapping());
    EXPECT_FLOAT_EQ(viewport.GetGridSnap(), 0.5f);

    viewport.SetSnapping(false);
    EXPECT_FALSE(viewport.IsSnapping());
}

TEST_F(SceneViewportTest, FocusSelection)
{
    auto* e = scene.CreateEntity("Target");
    e->GetTransform().SetPosition(100.0f, 50.0f, 200.0f);
    viewport.Select(e->GetID());

    viewport.FocusSelection();

    // カメラがエンティティの近くに移動しているはず
    const auto& cam = viewport.GetCamera();
    EXPECT_NEAR(cam.position[0], 100.0f, 1.0f);
}

TEST_F(SceneViewportTest, DuplicateSelected)
{
    auto* e = scene.CreateEntity("Original");
    e->GetTransform().SetPosition(5.0f, 10.0f, 15.0f);
    viewport.Select(e->GetID());

    auto newIds = viewport.DuplicateSelected();
    ASSERT_EQ(newIds.size(), 1u);

    Entity* clone = scene.FindEntityByID(newIds[0]);
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetName(), "Original_copy");
    EXPECT_FLOAT_EQ(clone->GetTransform().GetPosition().x, 5.0f);
    EXPECT_FLOAT_EQ(clone->GetTransform().GetPosition().y, 10.0f);
    EXPECT_FLOAT_EQ(clone->GetTransform().GetPosition().z, 15.0f);
}

TEST_F(SceneViewportTest, SelectionPrimary)
{
    auto* e1 = scene.CreateEntity("A");
    auto* e2 = scene.CreateEntity("B");

    viewport.Select(e1->GetID());
    viewport.Select(e2->GetID());

    auto sel = viewport.GetSelection();
    EXPECT_EQ(sel.primarySelection, e2->GetID()); // 最後に選択されたのがprimary
}

TEST_F(SceneViewportTest, DeselectUpdatesPrimary)
{
    auto* e1 = scene.CreateEntity("A");
    auto* e2 = scene.CreateEntity("B");

    viewport.Select(e1->GetID());
    viewport.Select(e2->GetID());
    viewport.Deselect(e2->GetID());

    auto sel = viewport.GetSelection();
    EXPECT_EQ(sel.primarySelection, e1->GetID());
}

TEST_F(SceneViewportTest, DoubleSelectIgnored)
{
    auto* e = scene.CreateEntity("A");
    viewport.Select(e->GetID());
    viewport.Select(e->GetID()); // 重複は無視される
    EXPECT_EQ(viewport.GetSelectedCount(), 1u);
}
