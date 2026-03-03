/// @file test_EditorPanels.cpp
/// @brief Editor パネル (SceneHierarchyPanel, PropertyInspector, AssetBrowser,
///        ConsoleWindow, TimelineEditor) の構築・状態管理テスト
///
/// ImGui 描画はヘッドレス環境では呼べないため、Draw() 以外の
/// 状態管理メソッドと構築/デフォルト値を検証する。

#include <gtest/gtest.h>
#include "Editor/SceneHierarchyPanel.h"
#include "Editor/PropertyInspector.h"
#include "Editor/AssetBrowser.h"
#include "Editor/ConsoleWindow.h"
#include "Editor/TimelineEditor.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Core/GameConsole.h"
#include "Core/Timeline.h"

using namespace gx;

// ============================================================================
// SceneHierarchyPanel
// ============================================================================

TEST(SceneHierarchyPanel, DefaultState)
{
    SceneHierarchyPanel panel;
    EXPECT_TRUE(panel.IsVisible());
    EXPECT_EQ(panel.GetSelectedEntity(), nullptr);
}

TEST(SceneHierarchyPanel, SetSelectedEntity)
{
    SceneHierarchyPanel panel;
    Scene scene("test");
    Entity* e = scene.CreateEntity("TestEntity");

    panel.SetSelectedEntity(e);
    EXPECT_EQ(panel.GetSelectedEntity(), e);

    panel.SetSelectedEntity(nullptr);
    EXPECT_EQ(panel.GetSelectedEntity(), nullptr);
}

TEST(SceneHierarchyPanel, ToggleVisibility)
{
    SceneHierarchyPanel panel;
    EXPECT_TRUE(panel.IsVisible());

    panel.Toggle();
    EXPECT_FALSE(panel.IsVisible());

    panel.Toggle();
    EXPECT_TRUE(panel.IsVisible());
}

TEST(SceneHierarchyPanel, SetVisible)
{
    SceneHierarchyPanel panel;
    panel.SetVisible(false);
    EXPECT_FALSE(panel.IsVisible());

    panel.SetVisible(true);
    EXPECT_TRUE(panel.IsVisible());
}

// ============================================================================
// PropertyInspector
// ============================================================================

TEST(PropertyInspector, DefaultState)
{
    PropertyInspector inspector;
    EXPECT_TRUE(inspector.IsVisible());
}

TEST(PropertyInspector, ToggleVisibility)
{
    PropertyInspector inspector;
    EXPECT_TRUE(inspector.IsVisible());

    inspector.Toggle();
    EXPECT_FALSE(inspector.IsVisible());

    inspector.Toggle();
    EXPECT_TRUE(inspector.IsVisible());
}

TEST(PropertyInspector, SetVisible)
{
    PropertyInspector inspector;
    inspector.SetVisible(false);
    EXPECT_FALSE(inspector.IsVisible());
}

// ============================================================================
// AssetBrowser
// ============================================================================

TEST(AssetBrowser, DefaultState)
{
    AssetBrowser browser;
    EXPECT_TRUE(browser.IsVisible());
    EXPECT_EQ(browser.GetCurrentDirectory(), "Assets");
    EXPECT_TRUE(browser.GetSelectedAssetPath().empty());
}

TEST(AssetBrowser, SetCurrentDirectory)
{
    AssetBrowser browser;
    browser.SetCurrentDirectory("Assets/Textures");
    EXPECT_EQ(browser.GetCurrentDirectory(), "Assets/Textures");
}

TEST(AssetBrowser, ToggleVisibility)
{
    AssetBrowser browser;
    browser.Toggle();
    EXPECT_FALSE(browser.IsVisible());

    browser.Toggle();
    EXPECT_TRUE(browser.IsVisible());
}

TEST(AssetBrowser, SetVisible)
{
    AssetBrowser browser;
    browser.SetVisible(false);
    EXPECT_FALSE(browser.IsVisible());
}

// ============================================================================
// ConsoleWindow
// ============================================================================

TEST(ConsoleWindow, DefaultState)
{
    ConsoleWindow console;
    EXPECT_TRUE(console.IsVisible());
    EXPECT_TRUE(console.IsAutoScroll());
    EXPECT_TRUE(console.IsShowInfo());
    EXPECT_TRUE(console.IsShowWarn());
    EXPECT_TRUE(console.IsShowError());
}

TEST(ConsoleWindow, ToggleVisibility)
{
    ConsoleWindow console;
    console.Toggle();
    EXPECT_FALSE(console.IsVisible());

    console.Toggle();
    EXPECT_TRUE(console.IsVisible());
}

TEST(ConsoleWindow, FilterSettings)
{
    ConsoleWindow console;

    console.SetShowInfo(false);
    EXPECT_FALSE(console.IsShowInfo());
    EXPECT_TRUE(console.IsShowWarn());
    EXPECT_TRUE(console.IsShowError());

    console.SetShowWarn(false);
    EXPECT_FALSE(console.IsShowWarn());

    console.SetShowError(false);
    EXPECT_FALSE(console.IsShowError());
}

TEST(ConsoleWindow, AutoScrollToggle)
{
    ConsoleWindow console;
    EXPECT_TRUE(console.IsAutoScroll());

    console.SetAutoScroll(false);
    EXPECT_FALSE(console.IsAutoScroll());

    console.SetAutoScroll(true);
    EXPECT_TRUE(console.IsAutoScroll());
}

TEST(ConsoleWindow, SetVisible)
{
    ConsoleWindow console;
    console.SetVisible(false);
    EXPECT_FALSE(console.IsVisible());
}

// ============================================================================
// TimelineEditor
// ============================================================================

TEST(TimelineEditor, DefaultState)
{
    TimelineEditor editor;
    EXPECT_TRUE(editor.IsVisible());
    EXPECT_FLOAT_EQ(editor.GetZoom(), 1.0f);
    EXPECT_FLOAT_EQ(editor.GetScrollX(), 0.0f);
    EXPECT_FLOAT_EQ(editor.GetCurrentTime(), 0.0f);
    EXPECT_EQ(editor.GetSelectedTrack(), -1);
    EXPECT_EQ(editor.GetSelectedKey(), -1);
}

TEST(TimelineEditor, ToggleVisibility)
{
    TimelineEditor editor;
    editor.Toggle();
    EXPECT_FALSE(editor.IsVisible());

    editor.Toggle();
    EXPECT_TRUE(editor.IsVisible());
}

TEST(TimelineEditor, SetZoom)
{
    TimelineEditor editor;
    editor.SetZoom(2.5f);
    EXPECT_FLOAT_EQ(editor.GetZoom(), 2.5f);
}

TEST(TimelineEditor, SetScrollX)
{
    TimelineEditor editor;
    editor.SetScrollX(100.0f);
    EXPECT_FLOAT_EQ(editor.GetScrollX(), 100.0f);
}

TEST(TimelineEditor, SetCurrentTime)
{
    TimelineEditor editor;
    editor.SetCurrentTime(3.5f);
    EXPECT_FLOAT_EQ(editor.GetCurrentTime(), 3.5f);
}

TEST(TimelineEditor, SetVisible)
{
    TimelineEditor editor;
    editor.SetVisible(false);
    EXPECT_FALSE(editor.IsVisible());
}

// ============================================================================
// 統合テスト: パネル同士の連携パターン
// ============================================================================

TEST(EditorPanelIntegration, HierarchyAndInspector)
{
    SceneHierarchyPanel hierarchy;
    PropertyInspector inspector;

    Scene scene("IntegrationTest");
    Entity* e1 = scene.CreateEntity("Player");
    Entity* e2 = scene.CreateEntity("Enemy");

    // 階層パネルでエンティティを選択
    hierarchy.SetSelectedEntity(e1);
    EXPECT_EQ(hierarchy.GetSelectedEntity(), e1);
    EXPECT_EQ(hierarchy.GetSelectedEntity()->GetName(), "Player");

    // 別のエンティティに切り替え
    hierarchy.SetSelectedEntity(e2);
    EXPECT_EQ(hierarchy.GetSelectedEntity()->GetName(), "Enemy");

    // 選択解除
    hierarchy.SetSelectedEntity(nullptr);
    EXPECT_EQ(hierarchy.GetSelectedEntity(), nullptr);
}

TEST(EditorPanelIntegration, AllPanelsConstruction)
{
    // 全パネルが問題なく構築できることを確認
    SceneHierarchyPanel hierarchy;
    PropertyInspector inspector;
    AssetBrowser browser;
    ConsoleWindow console;
    TimelineEditor timeline;

    EXPECT_TRUE(hierarchy.IsVisible());
    EXPECT_TRUE(inspector.IsVisible());
    EXPECT_TRUE(browser.IsVisible());
    EXPECT_TRUE(console.IsVisible());
    EXPECT_TRUE(timeline.IsVisible());
}

TEST(EditorPanelIntegration, TimelineEditorWithTimeline)
{
    TimelineEditor editor;
    Timeline timeline;

    // トラックを追加
    auto track = std::make_unique<FloatTrack>();
    track->AddKeyframe(0.0f, 0.0f);
    track->AddKeyframe(1.0f, 1.0f);
    track->AddKeyframe(2.0f, 0.5f);
    timeline.AddTrack(std::move(track));

    EXPECT_EQ(timeline.GetTrackCount(), 1u);
    EXPECT_FLOAT_EQ(timeline.GetDuration(), 2.0f);

    // エディタの状態を設定
    editor.SetCurrentTime(1.0f);
    EXPECT_FLOAT_EQ(editor.GetCurrentTime(), 1.0f);

    editor.SetZoom(2.0f);
    EXPECT_FLOAT_EQ(editor.GetZoom(), 2.0f);
}

TEST(EditorPanelIntegration, ConsoleWithGameConsole)
{
    ConsoleWindow window;
    GameConsole console;

    console.RegisterBuiltinCommands();

    // コマンドの実行
    gx::String result = console.Execute("echo Hello");
    EXPECT_FALSE(console.GetHistory().empty());

    // フィルタ設定
    window.SetShowInfo(true);
    window.SetShowWarn(false);
    window.SetShowError(true);
    EXPECT_TRUE(window.IsShowInfo());
    EXPECT_FALSE(window.IsShowWarn());
    EXPECT_TRUE(window.IsShowError());
}
