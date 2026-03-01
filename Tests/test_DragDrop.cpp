/// @file test_DragDrop.cpp
/// @brief DragDropManagerのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/DragDropManager.h"
#include "GUI/Widgets/Button.h"
#include "GUI/Widgets/Panel.h"

using namespace gx::GUI;

// ============================================================================
// ヘルパー: テスト用の具体的なウィジェットを作成
// ============================================================================

class DragDropTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_btn1 = std::make_unique<Button>();
        m_btn1->id = "btn1";
        m_btn2 = std::make_unique<Button>();
        m_btn2->id = "btn2";
        m_btn3 = std::make_unique<Button>();
        m_btn3->id = "btn3";
        m_panel = std::make_unique<Panel>();
        m_panel->id = "panel";
    }

    std::unique_ptr<Button> m_btn1;
    std::unique_ptr<Button> m_btn2;
    std::unique_ptr<Button> m_btn3;
    std::unique_ptr<Panel> m_panel;
    DragDropManager m_mgr;
};

// ============================================================================
// RegisterDragSource / UnregisterDragSource
// ============================================================================

TEST_F(DragDropTest, RegisterDragSourceIncreasesCount)
{
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 0u);
    m_mgr.RegisterDragSource(m_btn1.get(), "item");
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 1u);
    m_mgr.RegisterDragSource(m_btn2.get(), "item");
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 2u);
}

TEST_F(DragDropTest, UnregisterDragSourceDecreasesCount)
{
    m_mgr.RegisterDragSource(m_btn1.get(), "item");
    m_mgr.RegisterDragSource(m_btn2.get(), "item");
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 2u);
    m_mgr.UnregisterDragSource(m_btn1.get());
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 1u);
}

// ============================================================================
// RegisterDropTarget / UnregisterDropTarget
// ============================================================================

TEST_F(DragDropTest, RegisterDropTargetIncreasesCount)
{
    EXPECT_EQ(m_mgr.GetDropTargetCount(), 0u);
    DropTarget target;
    m_mgr.RegisterDropTarget(m_panel.get(), target);
    EXPECT_EQ(m_mgr.GetDropTargetCount(), 1u);
}

TEST_F(DragDropTest, UnregisterDropTargetDecreasesCount)
{
    DropTarget target;
    m_mgr.RegisterDropTarget(m_panel.get(), target);
    m_mgr.RegisterDropTarget(m_btn1.get(), target);
    EXPECT_EQ(m_mgr.GetDropTargetCount(), 2u);
    m_mgr.UnregisterDropTarget(m_panel.get());
    EXPECT_EQ(m_mgr.GetDropTargetCount(), 1u);
}

// ============================================================================
// BeginDrag / IsDragging
// ============================================================================

TEST_F(DragDropTest, BeginDragSetsDragging)
{
    EXPECT_FALSE(m_mgr.IsDragging());

    DragPayload payload;
    payload.type = "test";
    int val = 42;
    payload.data.assign(reinterpret_cast<uint8_t*>(&val),
                        reinterpret_cast<uint8_t*>(&val) + sizeof(val));
    m_mgr.BeginDrag(m_btn1.get(), payload, 10.0f, 20.0f);

    EXPECT_TRUE(m_mgr.IsDragging());
}

// ============================================================================
// CancelDrag
// ============================================================================

TEST_F(DragDropTest, CancelDragStopsDragging)
{
    DragPayload payload;
    payload.type = "test";
    m_mgr.BeginDrag(m_btn1.get(), payload, 0.0f, 0.0f);
    EXPECT_TRUE(m_mgr.IsDragging());

    m_mgr.CancelDrag();
    EXPECT_FALSE(m_mgr.IsDragging());
}

// ============================================================================
// EndDrag
// ============================================================================

TEST_F(DragDropTest, EndDragWithoutTargetNoCrash)
{
    DragPayload payload;
    payload.type = "test";
    m_mgr.BeginDrag(m_btn1.get(), payload, 0.0f, 0.0f);
    EXPECT_TRUE(m_mgr.IsDragging());

    // 登録されたドロップターゲットが存在しない位置でEndDrag
    m_mgr.EndDrag(100.0f, 100.0f);
    EXPECT_FALSE(m_mgr.IsDragging());
}

TEST_F(DragDropTest, EndDragSetsDraggingFalse)
{
    DragPayload payload;
    payload.type = "widget";
    m_mgr.BeginDrag(m_btn1.get(), payload, 5.0f, 5.0f);
    EXPECT_TRUE(m_mgr.IsDragging());

    m_mgr.EndDrag(50.0f, 50.0f);
    EXPECT_FALSE(m_mgr.IsDragging());
}

// ============================================================================
// Clear
// ============================================================================

TEST_F(DragDropTest, ClearResetsEverything)
{
    m_mgr.RegisterDragSource(m_btn1.get(), "item");
    m_mgr.RegisterDragSource(m_btn2.get(), "item");
    DropTarget target;
    m_mgr.RegisterDropTarget(m_panel.get(), target);

    DragPayload payload;
    payload.type = "item";
    m_mgr.BeginDrag(m_btn1.get(), payload, 0, 0);

    m_mgr.Clear();
    EXPECT_FALSE(m_mgr.IsDragging());
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 0u);
    EXPECT_EQ(m_mgr.GetDropTargetCount(), 0u);
}

// ============================================================================
// GetDragSource
// ============================================================================

TEST_F(DragDropTest, GetDragSourceReturnsCurrent)
{
    DragPayload payload;
    payload.type = "color";
    payload.text = "red";
    m_mgr.BeginDrag(m_btn1.get(), payload, 15.0f, 25.0f);

    const DragSource& src = m_mgr.GetDragSource();
    EXPECT_TRUE(src.dragging);
    EXPECT_EQ(src.sourceWidget, m_btn1.get());
    EXPECT_EQ(src.payload.type, "color");
    EXPECT_FLOAT_EQ(src.startX, 15.0f);
    EXPECT_FLOAT_EQ(src.startY, 25.0f);
}

// ============================================================================
// 複数のドラッグソース
// ============================================================================

TEST_F(DragDropTest, MultipleSourcesCanBeRegistered)
{
    m_mgr.RegisterDragSource(m_btn1.get(), "type_a");
    m_mgr.RegisterDragSource(m_btn2.get(), "type_b");
    m_mgr.RegisterDragSource(m_btn3.get(), "type_c");
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 3u);
}

// ============================================================================
// SetDragPreview
// ============================================================================

TEST_F(DragDropTest, SetDragPreview)
{
    EXPECT_EQ(m_mgr.GetDragPreview(), nullptr);
    m_mgr.SetDragPreview(m_panel.get());
    EXPECT_EQ(m_mgr.GetDragPreview(), m_panel.get());
}

// ============================================================================
// RegisterNullWidgetIsIgnored
// ============================================================================

TEST_F(DragDropTest, RegisterNullWidgetIsIgnored)
{
    m_mgr.RegisterDragSource(nullptr, "test");
    EXPECT_EQ(m_mgr.GetDragSourceCount(), 0u);

    DropTarget target;
    m_mgr.RegisterDropTarget(nullptr, target);
    EXPECT_EQ(m_mgr.GetDropTargetCount(), 0u);
}

// ============================================================================
// nullソースでのBeginDragは無視される
// ============================================================================

TEST_F(DragDropTest, BeginDragNullSourceIgnored)
{
    DragPayload payload;
    payload.type = "test";
    m_mgr.BeginDrag(nullptr, payload, 0, 0);
    EXPECT_FALSE(m_mgr.IsDragging());
}
