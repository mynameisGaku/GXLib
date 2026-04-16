/// @file test_GUIWidget.cpp
/// @brief Phase 5: GUIウィジェット生成、階層、LayoutRect、WidgetTypeテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/Widget.h"
#include "GUI/UIContext.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/Widgets/Button.h"
#include "GUI/Widgets/TextWidget.h"
#include "GUI/Widgets/Slider.h"
#include "GUI/Widgets/CheckBox.h"
#include "GUI/Widgets/Spacer.h"

using namespace gx::GUI;

// ============================================================================
// LayoutRect
// ============================================================================

TEST(LayoutRectTest, DefaultValues)
{
    LayoutRect rect;
    EXPECT_FLOAT_EQ(rect.x, 0.0f);
    EXPECT_FLOAT_EQ(rect.y, 0.0f);
    EXPECT_FLOAT_EQ(rect.width, 0.0f);
    EXPECT_FLOAT_EQ(rect.height, 0.0f);
}

TEST(LayoutRectTest, ContainsInside)
{
    LayoutRect rect;
    rect.x = 10.0f;
    rect.y = 20.0f;
    rect.width = 100.0f;
    rect.height = 50.0f;

    EXPECT_TRUE(rect.Contains(50.0f, 40.0f));
    EXPECT_TRUE(rect.Contains(10.0f, 20.0f));   // 左上隅
    EXPECT_TRUE(rect.Contains(110.0f, 70.0f));   // 右下隅
}

TEST(LayoutRectTest, ContainsOutside)
{
    LayoutRect rect;
    rect.x = 10.0f;
    rect.y = 20.0f;
    rect.width = 100.0f;
    rect.height = 50.0f;

    EXPECT_FALSE(rect.Contains(0.0f, 0.0f));
    EXPECT_FALSE(rect.Contains(9.9f, 30.0f));    // 矩形のすぐ左
    EXPECT_FALSE(rect.Contains(50.0f, 71.0f));   // 矩形のすぐ下
    EXPECT_FALSE(rect.Contains(111.0f, 30.0f));  // 矩形のすぐ右
}

// ============================================================================
// WidgetType列挙型の値
// ============================================================================

TEST(WidgetTypeTest, EnumValues)
{
    // 期待されるすべてのWidgetType値が存在することを検証
    EXPECT_NE(static_cast<int>(WidgetType::Panel), static_cast<int>(WidgetType::Button));
    EXPECT_NE(static_cast<int>(WidgetType::Text), static_cast<int>(WidgetType::Image));
    EXPECT_NE(static_cast<int>(WidgetType::Slider), static_cast<int>(WidgetType::CheckBox));
    EXPECT_NE(static_cast<int>(WidgetType::RadioButton), static_cast<int>(WidgetType::DropDown));
    EXPECT_NE(static_cast<int>(WidgetType::ListView), static_cast<int>(WidgetType::ScrollView));
    EXPECT_NE(static_cast<int>(WidgetType::ProgressBar), static_cast<int>(WidgetType::TabView));
    EXPECT_NE(static_cast<int>(WidgetType::Dialog), static_cast<int>(WidgetType::Canvas));
    EXPECT_NE(static_cast<int>(WidgetType::Spacer), static_cast<int>(WidgetType::Panel));
}

// ============================================================================
// Panelウィジェット
// ============================================================================

TEST(WidgetTest, PanelCreation)
{
    Panel panel;
    EXPECT_EQ(panel.GetType(), WidgetType::Panel);
    EXPECT_TRUE(panel.visible);
    EXPECT_TRUE(panel.enabled);
    EXPECT_FLOAT_EQ(panel.opacity, 1.0f);
}

TEST(WidgetTest, PanelDefaultNoParent)
{
    Panel panel;
    EXPECT_EQ(panel.GetParent(), nullptr);
    EXPECT_EQ(panel.GetChildren().size(), 0u);
}

// ============================================================================
// Buttonウィジェット
// ============================================================================

TEST(WidgetTest, ButtonCreation)
{
    Button button;
    EXPECT_EQ(button.GetType(), WidgetType::Button);
    EXPECT_TRUE(button.GetText().empty());
}

TEST(WidgetTest, ButtonSetText)
{
    Button button;
    button.SetText(L"Click Me");
    EXPECT_EQ(button.GetText(), L"Click Me");
}

TEST(WidgetTest, ButtonFontHandle)
{
    Button button;
    EXPECT_EQ(button.GetFontHandle(), -1);
    button.SetFontHandle(42);
    EXPECT_EQ(button.GetFontHandle(), 42);
}

// ============================================================================
// TextWidget
// ============================================================================

TEST(WidgetTest, TextWidgetCreation)
{
    TextWidget text;
    EXPECT_EQ(text.GetType(), WidgetType::Text);
    EXPECT_TRUE(text.GetText().empty());
}

TEST(WidgetTest, TextWidgetSetText)
{
    TextWidget text;
    text.SetText(L"Hello World");
    EXPECT_EQ(text.GetText(), L"Hello World");
}

// ============================================================================
// Sliderウィジェット
// ============================================================================

TEST(WidgetTest, SliderCreation)
{
    Slider slider;
    EXPECT_EQ(slider.GetType(), WidgetType::Slider);
    EXPECT_FLOAT_EQ(slider.GetValue(), 0.0f);
    EXPECT_FLOAT_EQ(slider.GetMin(), 0.0f);
    EXPECT_FLOAT_EQ(slider.GetMax(), 1.0f);
    EXPECT_FLOAT_EQ(slider.GetStep(), 0.0f);
}

TEST(WidgetTest, SliderSetRange)
{
    Slider slider;
    slider.SetRange(-10.0f, 10.0f);
    EXPECT_FLOAT_EQ(slider.GetMin(), -10.0f);
    EXPECT_FLOAT_EQ(slider.GetMax(), 10.0f);
}

TEST(WidgetTest, SliderSetStep)
{
    Slider slider;
    slider.SetStep(0.1f);
    EXPECT_FLOAT_EQ(slider.GetStep(), 0.1f);
}

TEST(WidgetTest, SliderIntrinsicSize)
{
    Slider slider;
    EXPECT_GT(slider.GetIntrinsicWidth(), 0.0f);
    EXPECT_GT(slider.GetIntrinsicHeight(), 0.0f);
}

// ============================================================================
// CheckBoxウィジェット
// ============================================================================

TEST(WidgetTest, CheckBoxCreation)
{
    CheckBox cb;
    EXPECT_EQ(cb.GetType(), WidgetType::CheckBox);
    EXPECT_FALSE(cb.IsChecked());
    EXPECT_FALSE(cb.GetValue());
}

TEST(WidgetTest, CheckBoxSetChecked)
{
    CheckBox cb;
    cb.SetChecked(true);
    EXPECT_TRUE(cb.IsChecked());
    EXPECT_TRUE(cb.GetValue());

    cb.SetChecked(false);
    EXPECT_FALSE(cb.IsChecked());
}

TEST(WidgetTest, CheckBoxText)
{
    CheckBox cb;
    cb.SetText(L"Accept Terms");
    EXPECT_EQ(cb.GetText(), L"Accept Terms");
}

// ============================================================================
// ウィジェット階層
// ============================================================================

TEST(WidgetTest, AddChild)
{
    auto panel = std::make_unique<Panel>();
    auto* panelPtr = panel.get();

    auto child = std::make_unique<Button>();
    auto* childPtr = child.get();

    panelPtr->AddChild(std::move(child));

    EXPECT_EQ(panelPtr->GetChildren().size(), 1u);
    EXPECT_EQ(childPtr->GetParent(), panelPtr);
}

TEST(WidgetTest, AddMultipleChildren)
{
    Panel panel;
    panel.AddChild(std::make_unique<Button>());
    panel.AddChild(std::make_unique<TextWidget>());
    panel.AddChild(std::make_unique<Slider>());

    EXPECT_EQ(panel.GetChildren().size(), 3u);

    EXPECT_EQ(panel.GetChildren()[0]->GetType(), WidgetType::Button);
    EXPECT_EQ(panel.GetChildren()[1]->GetType(), WidgetType::Text);
    EXPECT_EQ(panel.GetChildren()[2]->GetType(), WidgetType::Slider);
}

TEST(WidgetTest, RemoveChild)
{
    Panel panel;
    auto child = std::make_unique<Button>();
    auto* childPtr = child.get();
    panel.AddChild(std::move(child));
    EXPECT_EQ(panel.GetChildren().size(), 1u);

    panel.RemoveChild(childPtr);
    EXPECT_EQ(panel.GetChildren().size(), 0u);
}

// ============================================================================
// ウィジェットプロパティ
// ============================================================================

TEST(WidgetTest, IdAndClassName)
{
    Panel panel;
    panel.id = "main-panel";
    panel.className = "container";

    EXPECT_EQ(panel.id, "main-panel");
    EXPECT_EQ(panel.className, "container");
}

TEST(WidgetTest, FindById)
{
    Panel root;
    root.id = "root";

    auto child = std::make_unique<Button>();
    child->id = "submit-btn";
    auto* childPtr = child.get();
    root.AddChild(std::move(child));

    auto* found = root.FindById("submit-btn");
    EXPECT_EQ(found, childPtr);

    auto* notFound = root.FindById("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST(WidgetTest, FindByIdNested)
{
    Panel root;
    root.id = "root";

    auto inner = std::make_unique<Panel>();
    inner->id = "inner";

    auto button = std::make_unique<Button>();
    button->id = "deep-button";
    auto* buttonPtr = button.get();

    inner->AddChild(std::move(button));
    root.AddChild(std::move(inner));

    auto* found = root.FindById("deep-button");
    EXPECT_EQ(found, buttonPtr);
}

TEST(WidgetTest, Opacity)
{
    Panel panel;
    EXPECT_FLOAT_EQ(panel.opacity, 1.0f);

    panel.opacity = 0.5f;
    EXPECT_FLOAT_EQ(panel.opacity, 0.5f);
}

TEST(WidgetTest, VisibleAndEnabled)
{
    Button button;
    EXPECT_TRUE(button.visible);
    EXPECT_TRUE(button.enabled);

    button.visible = false;
    button.enabled = false;
    EXPECT_FALSE(button.visible);
    EXPECT_FALSE(button.enabled);
}

TEST(WidgetTest, ButtonHoverAndPressStyles)
{
    Button button;
    // Button has hoverStyle, pressedStyle, disabledStyle members
    button.hoverStyle.backgroundColor = StyleColor(0.8f, 0.8f, 0.8f, 1.0f);
    button.pressedStyle.backgroundColor = StyleColor(0.6f, 0.6f, 0.6f, 1.0f);
    button.disabledStyle.backgroundColor = StyleColor(0.4f, 0.4f, 0.4f, 1.0f);

    EXPECT_FLOAT_EQ(button.hoverStyle.backgroundColor.r, 0.8f);
    EXPECT_FLOAT_EQ(button.pressedStyle.backgroundColor.r, 0.6f);
    EXPECT_FLOAT_EQ(button.disabledStyle.backgroundColor.r, 0.4f);
}

// ============================================================================
// UIContext
// ============================================================================

TEST(UIContextTest, DefaultConstruction)
{
    UIContext context;
    EXPECT_EQ(context.GetRoot(), nullptr);
    EXPECT_EQ(context.GetFocusedWidget(), nullptr);
    EXPECT_EQ(context.GetRenderer(), nullptr);
}

TEST(UIContextTest, SetRoot)
{
    UIContext context;
    auto root = std::make_unique<Panel>();
    auto* rootPtr = root.get();
    context.SetRoot(std::move(root));

    EXPECT_EQ(context.GetRoot(), rootPtr);
}

TEST(UIContextTest, ReplaceRoot)
{
    UIContext context;
    context.SetRoot(std::make_unique<Panel>());
    auto* firstRoot = context.GetRoot();

    auto newRoot = std::make_unique<Panel>();
    auto* newRootPtr = newRoot.get();
    context.SetRoot(std::move(newRoot));

    EXPECT_EQ(context.GetRoot(), newRootPtr);
    EXPECT_NE(context.GetRoot(), firstRoot);
}

TEST(UIContextTest, FindByIdInContext)
{
    UIContext context;
    auto root = std::make_unique<Panel>();
    root->id = "root";

    auto btn = std::make_unique<Button>();
    btn->id = "my-button";
    auto* btnPtr = btn.get();
    root->AddChild(std::move(btn));

    context.SetRoot(std::move(root));

    EXPECT_EQ(context.FindById("my-button"), btnPtr);
    EXPECT_EQ(context.FindById("nonexistent"), nullptr);
}

// ============================================================================
// UIEventデフォルト値
// ============================================================================

TEST(UIEventTest, Defaults)
{
    UIEvent event;
    EXPECT_EQ(event.type, UIEventType::MouseMove);
    EXPECT_EQ(event.phase, UIEventPhase::Target);
    EXPECT_FLOAT_EQ(event.mouseX, 0.0f);
    EXPECT_FLOAT_EQ(event.mouseY, 0.0f);
    EXPECT_EQ(event.mouseButton, 0);
    EXPECT_EQ(event.wheelDelta, 0);
    EXPECT_EQ(event.keyCode, 0);
    EXPECT_EQ(event.target, nullptr);
    EXPECT_FALSE(event.handled);
    EXPECT_FALSE(event.stopPropagation);
}
