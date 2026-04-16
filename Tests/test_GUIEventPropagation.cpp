/// @file test_GUIEventPropagation.cpp
/// @brief GUIイベントディスパッチ順序、stopPropagation、フォーカス遷移、ヒットテスト、z-order

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/Widget.h"

using namespace gx::GUI;

static constexpr float kEps = 1e-5f;

// ============================================================================
// EventPhaseOrder: Capture -> Target -> Bubble
// ============================================================================

TEST(EventPhaseOrder, Capture_IsFirst)
{
    EXPECT_EQ(static_cast<int>(UIEventPhase::Capture), 0);
}

TEST(EventPhaseOrder, Target_IsSecond)
{
    EXPECT_EQ(static_cast<int>(UIEventPhase::Target), 1);
}

TEST(EventPhaseOrder, Bubble_IsThird)
{
    EXPECT_EQ(static_cast<int>(UIEventPhase::Bubble), 2);
}

TEST(EventPhaseOrder, Capture_LessThan_Target)
{
    EXPECT_LT(static_cast<int>(UIEventPhase::Capture),
              static_cast<int>(UIEventPhase::Target));
}

TEST(EventPhaseOrder, Target_LessThan_Bubble)
{
    EXPECT_LT(static_cast<int>(UIEventPhase::Target),
              static_cast<int>(UIEventPhase::Bubble));
}

// ============================================================================
// UIEvent Defaults
// ============================================================================

TEST(UIEventDefaults, HandledDefault_False)
{
    UIEvent ev{};
    EXPECT_FALSE(ev.handled);
}

TEST(UIEventDefaults, StopPropagationDefault_False)
{
    UIEvent ev{};
    EXPECT_FALSE(ev.stopPropagation);
}

TEST(UIEventDefaults, TargetDefault_Null)
{
    UIEvent ev{};
    EXPECT_EQ(ev.target, nullptr);
}

TEST(UIEventDefaults, PhaseDefault_Target)
{
    UIEvent ev{};
    EXPECT_EQ(ev.phase, UIEventPhase::Target);
}

TEST(UIEventDefaults, MouseCoords_DefaultZero)
{
    UIEvent ev{};
    EXPECT_NEAR(ev.mouseX, 0.0f, kEps);
    EXPECT_NEAR(ev.mouseY, 0.0f, kEps);
}

TEST(UIEventDefaults, LocalCoords_DefaultZero)
{
    UIEvent ev{};
    EXPECT_NEAR(ev.localX, 0.0f, kEps);
    EXPECT_NEAR(ev.localY, 0.0f, kEps);
}

TEST(UIEventDefaults, KeyCode_DefaultZero)
{
    UIEvent ev{};
    EXPECT_EQ(ev.keyCode, 0);
}

// ============================================================================
// StopPropagation
// ============================================================================

TEST(StopPropagation, SetHandled)
{
    UIEvent ev{};
    ev.handled = true;
    EXPECT_TRUE(ev.handled);
}

TEST(StopPropagation, SetStopPropagation)
{
    UIEvent ev{};
    ev.stopPropagation = true;
    EXPECT_TRUE(ev.stopPropagation);
}

TEST(StopPropagation, BothFlags_Independent)
{
    UIEvent ev{};
    ev.handled = true;
    ev.stopPropagation = false;
    EXPECT_TRUE(ev.handled);
    EXPECT_FALSE(ev.stopPropagation);
}

// ============================================================================
// WidgetHitTest: LayoutRect::Contains
// ============================================================================

TEST(WidgetHitTest, Contains_InsideCenter)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_TRUE(rect.Contains(60, 45));
}

TEST(WidgetHitTest, Contains_TopLeftCorner)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_TRUE(rect.Contains(10, 20));
}

TEST(WidgetHitTest, Contains_BottomRightCorner)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_TRUE(rect.Contains(110, 70));
}

TEST(WidgetHitTest, Contains_OutsideLeft)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_FALSE(rect.Contains(5, 45));
}

TEST(WidgetHitTest, Contains_OutsideRight)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_FALSE(rect.Contains(115, 45));
}

TEST(WidgetHitTest, Contains_OutsideTop)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_FALSE(rect.Contains(60, 15));
}

TEST(WidgetHitTest, Contains_OutsideBottom)
{
    LayoutRect rect{10, 20, 100, 50};
    EXPECT_FALSE(rect.Contains(60, 75));
}

TEST(WidgetHitTest, Contains_ZeroSize_OnlyOrigin)
{
    LayoutRect rect{50, 50, 0, 0};
    EXPECT_TRUE(rect.Contains(50, 50));
    EXPECT_FALSE(rect.Contains(51, 50));
}

TEST(WidgetHitTest, Contains_LargeRect)
{
    LayoutRect rect{0, 0, 1920, 1080};
    EXPECT_TRUE(rect.Contains(960, 540));
    EXPECT_TRUE(rect.Contains(0, 0));
    EXPECT_TRUE(rect.Contains(1920, 1080));
    EXPECT_FALSE(rect.Contains(-1, 0));
}

// ============================================================================
// Focus Transition
// ============================================================================

// Minimal concrete widget for testing (Widget is abstract via GetType)
class TestWidget : public Widget
{
public:
    WidgetType GetType() const override { return WidgetType::Panel; }
};

TEST(FocusTransition, DefaultStates_AllFalse)
{
    TestWidget w;
    EXPECT_FALSE(w.focused);
    EXPECT_FALSE(w.hovered);
    EXPECT_FALSE(w.pressed);
}

TEST(FocusTransition, SetFocused)
{
    TestWidget w;
    w.focused = true;
    EXPECT_TRUE(w.focused);
}

TEST(FocusTransition, SetHovered)
{
    TestWidget w;
    w.hovered = true;
    EXPECT_TRUE(w.hovered);
}

TEST(FocusTransition, SetPressed)
{
    TestWidget w;
    w.pressed = true;
    EXPECT_TRUE(w.pressed);
}

TEST(FocusTransition, FocusThenUnfocus)
{
    TestWidget w;
    w.focused = true;
    EXPECT_TRUE(w.focused);
    w.focused = false;
    EXPECT_FALSE(w.focused);
}

// ============================================================================
// ZOrder
// ============================================================================

TEST(ZOrder, DefaultZIndex_IsZero)
{
    TestWidget w;
    EXPECT_EQ(w.zIndex, 0);
}

TEST(ZOrder, HigherZIndex_SortsAbove)
{
    TestWidget a, b;
    a.zIndex = 1;
    b.zIndex = 10;
    EXPECT_GT(b.zIndex, a.zIndex);
}

TEST(ZOrder, NegativeZIndex_SortsBelow)
{
    TestWidget a, b;
    a.zIndex = -5;
    b.zIndex = 0;
    EXPECT_LT(a.zIndex, b.zIndex);
}

// ============================================================================
// WidgetType enum: all types have distinct values
// ============================================================================

TEST(WidgetTypeEnum, AllDistinct)
{
    gx::Set<int> values;
    values.insert(static_cast<int>(WidgetType::Panel));
    values.insert(static_cast<int>(WidgetType::Text));
    values.insert(static_cast<int>(WidgetType::Button));
    values.insert(static_cast<int>(WidgetType::Image));
    values.insert(static_cast<int>(WidgetType::TextInput));
    values.insert(static_cast<int>(WidgetType::Slider));
    values.insert(static_cast<int>(WidgetType::CheckBox));
    values.insert(static_cast<int>(WidgetType::RadioButton));
    values.insert(static_cast<int>(WidgetType::DropDown));
    values.insert(static_cast<int>(WidgetType::ListView));
    values.insert(static_cast<int>(WidgetType::ScrollView));
    values.insert(static_cast<int>(WidgetType::ProgressBar));
    values.insert(static_cast<int>(WidgetType::TabView));
    values.insert(static_cast<int>(WidgetType::Dialog));
    values.insert(static_cast<int>(WidgetType::Canvas));
    values.insert(static_cast<int>(WidgetType::Spacer));

    EXPECT_EQ(values.size(), 16u);
}

// ============================================================================
// EventType enum: all event types distinct
// ============================================================================

TEST(EventTypeEnum, AllDistinct)
{
    gx::Set<int> values;
    values.insert(static_cast<int>(UIEventType::MouseDown));
    values.insert(static_cast<int>(UIEventType::MouseUp));
    values.insert(static_cast<int>(UIEventType::MouseMove));
    values.insert(static_cast<int>(UIEventType::MouseWheel));
    values.insert(static_cast<int>(UIEventType::MouseEnter));
    values.insert(static_cast<int>(UIEventType::MouseLeave));
    values.insert(static_cast<int>(UIEventType::KeyDown));
    values.insert(static_cast<int>(UIEventType::KeyUp));
    values.insert(static_cast<int>(UIEventType::CharInput));
    values.insert(static_cast<int>(UIEventType::FocusGained));
    values.insert(static_cast<int>(UIEventType::FocusLost));
    values.insert(static_cast<int>(UIEventType::Click));
    values.insert(static_cast<int>(UIEventType::ValueChanged));
    values.insert(static_cast<int>(UIEventType::Submit));

    EXPECT_EQ(values.size(), 14u);
}

// ============================================================================
// Widget properties
// ============================================================================

TEST(WidgetProps, DefaultVisible_True)
{
    TestWidget w;
    EXPECT_TRUE(w.visible);
}

TEST(WidgetProps, DefaultEnabled_True)
{
    TestWidget w;
    EXPECT_TRUE(w.enabled);
}

TEST(WidgetProps, DefaultOpacity_One)
{
    TestWidget w;
    EXPECT_NEAR(w.opacity, 1.0f, kEps);
}

TEST(WidgetProps, SetId)
{
    TestWidget w;
    w.id = "myWidget";
    EXPECT_EQ(w.id, "myWidget");
}

TEST(WidgetProps, SetClassName)
{
    TestWidget w;
    w.className = "btn-primary";
    EXPECT_EQ(w.className, "btn-primary");
}

TEST(WidgetProps, ParentDefault_Null)
{
    TestWidget w;
    EXPECT_EQ(w.GetParent(), nullptr);
}

TEST(WidgetProps, LayoutRect_Default_AllZero)
{
    TestWidget w;
    EXPECT_NEAR(w.layoutRect.x, 0.0f, kEps);
    EXPECT_NEAR(w.layoutRect.y, 0.0f, kEps);
    EXPECT_NEAR(w.layoutRect.width, 0.0f, kEps);
    EXPECT_NEAR(w.layoutRect.height, 0.0f, kEps);
}

TEST(WidgetProps, LayoutDirty_DefaultTrue)
{
    TestWidget w;
    EXPECT_TRUE(w.layoutDirty);
}
