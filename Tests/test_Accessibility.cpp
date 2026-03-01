/// @file test_Accessibility.cpp
/// @brief Accessibility system unit tests

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/Accessibility.h"

using namespace gx::GUI;

// ============================================================================
// Configuration
// ============================================================================

TEST(AccessibilityTest, DefaultConfig)
{
    AccessibilityManager mgr;
    const auto& cfg = mgr.GetConfig();
    EXPECT_TRUE(cfg.enableKeyboardNav);
    EXPECT_TRUE(cfg.enableScreenReader);
    EXPECT_FALSE(cfg.enableHighContrast);
    EXPECT_FALSE(cfg.enableReducedMotion);
    EXPECT_FLOAT_EQ(cfg.fontScaleFactor, 1.0f);
    EXPECT_FLOAT_EQ(cfg.minimumTouchTarget, 44.0f);
}

TEST(AccessibilityTest, SetConfig)
{
    AccessibilityManager mgr;
    AccessibilityConfig cfg;
    cfg.enableHighContrast = true;
    cfg.fontScaleFactor = 1.5f;
    mgr.SetConfig(cfg);

    EXPECT_TRUE(mgr.GetConfig().enableHighContrast);
    EXPECT_FLOAT_EQ(mgr.GetConfig().fontScaleFactor, 1.5f);
}

// ============================================================================
// Element management
// ============================================================================

TEST(AccessibilityTest, RegisterElement)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.role = AccessibilityRole::Button;
    elem.label = "Submit";

    uint32_t id = mgr.RegisterElement(elem);
    EXPECT_GT(id, 0u);
}

TEST(AccessibilityTest, UnregisterElement)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.label = "Cancel";
    uint32_t id = mgr.RegisterElement(elem);

    EXPECT_TRUE(mgr.UnregisterElement(id));
    EXPECT_FALSE(mgr.UnregisterElement(id));  // Already removed
}

TEST(AccessibilityTest, GetElement)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.role = AccessibilityRole::Slider;
    elem.label = "Volume";
    elem.hint = "Adjust volume";
    uint32_t id = mgr.RegisterElement(elem);

    const AccessibleElement* result = mgr.GetElement(id);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->role, AccessibilityRole::Slider);
    EXPECT_EQ(result->label, "Volume");
    EXPECT_EQ(result->hint, "Adjust volume");

    EXPECT_EQ(mgr.GetElement(9999), nullptr);
}

TEST(AccessibilityTest, ElementCount)
{
    AccessibilityManager mgr;
    EXPECT_EQ(mgr.GetElementCount(), 0u);

    AccessibleElement e1; e1.label = "A";
    AccessibleElement e2; e2.label = "B";
    mgr.RegisterElement(e1);
    mgr.RegisterElement(e2);
    EXPECT_EQ(mgr.GetElementCount(), 2u);
}

TEST(AccessibilityTest, HasElement)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    uint32_t id = mgr.RegisterElement(elem);
    EXPECT_TRUE(mgr.HasElement(id));
    EXPECT_FALSE(mgr.HasElement(9999));
}

TEST(AccessibilityTest, UpdateElement)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.label = "Progress";
    elem.state.valueCurrent = 0.0f;
    uint32_t id = mgr.RegisterElement(elem);

    AccessibilityState newState;
    newState.valueCurrent = 50.0f;
    newState.isEnabled = true;
    EXPECT_TRUE(mgr.UpdateElement(id, newState));

    const auto* updated = mgr.GetElement(id);
    ASSERT_NE(updated, nullptr);
    EXPECT_FLOAT_EQ(updated->state.valueCurrent, 50.0f);

    EXPECT_FALSE(mgr.UpdateElement(9999, newState));
}

// ============================================================================
// Focus management
// ============================================================================

TEST(AccessibilityTest, SetFocus)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.label = "Button1";
    uint32_t id = mgr.RegisterElement(elem);

    EXPECT_TRUE(mgr.SetFocus(id));
    EXPECT_EQ(mgr.GetFocusedElement(), id);

    // Element should be marked as focused
    const auto* e = mgr.GetElement(id);
    ASSERT_NE(e, nullptr);
    EXPECT_TRUE(e->state.isFocused);
}

TEST(AccessibilityTest, GetFocusedElement)
{
    AccessibilityManager mgr;
    EXPECT_EQ(mgr.GetFocusedElement(), 0u);

    AccessibleElement elem;
    elem.label = "Focus Target";
    uint32_t id = mgr.RegisterElement(elem);
    mgr.SetFocus(id);
    EXPECT_EQ(mgr.GetFocusedElement(), id);
}

TEST(AccessibilityTest, NavigateFocusNext)
{
    AccessibilityManager mgr;

    AccessibleElement e1; e1.label = "First"; e1.tabOrder = 0;
    AccessibleElement e2; e2.label = "Second"; e2.tabOrder = 1;
    AccessibleElement e3; e3.label = "Third"; e3.tabOrder = 2;
    uint32_t id1 = mgr.RegisterElement(e1);
    uint32_t id2 = mgr.RegisterElement(e2);
    uint32_t id3 = mgr.RegisterElement(e3);

    // Navigate from nothing -> first
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Next));
    EXPECT_EQ(mgr.GetFocusedElement(), id1);

    // Next -> second
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Next));
    EXPECT_EQ(mgr.GetFocusedElement(), id2);

    // Next -> third
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Next));
    EXPECT_EQ(mgr.GetFocusedElement(), id3);

    // Next -> wrap to first
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Next));
    EXPECT_EQ(mgr.GetFocusedElement(), id1);
}

TEST(AccessibilityTest, NavigateFocusPrev)
{
    AccessibilityManager mgr;

    AccessibleElement e1; e1.label = "First"; e1.tabOrder = 0;
    AccessibleElement e2; e2.label = "Second"; e2.tabOrder = 1;
    uint32_t id1 = mgr.RegisterElement(e1);
    uint32_t id2 = mgr.RegisterElement(e2);

    mgr.SetFocus(id1);
    // Previous from first -> wrap to last
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Previous));
    EXPECT_EQ(mgr.GetFocusedElement(), id2);
}

TEST(AccessibilityTest, ActivateFocused)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.label = "ActivateMe";
    uint32_t id = mgr.RegisterElement(elem);
    mgr.SetFocus(id);

    EXPECT_TRUE(mgr.ActivateFocused());

    // No focus -> cannot activate
    mgr.Clear();
    EXPECT_FALSE(mgr.ActivateFocused());
}

// ============================================================================
// Screen reader
// ============================================================================

TEST(AccessibilityTest, Announce)
{
    AccessibilityManager mgr;
    mgr.Announce("Page loaded", AnnouncementPriority::High);
    mgr.Announce("Item selected", AnnouncementPriority::Low);

    auto announcements = mgr.GetPendingAnnouncements();
    EXPECT_EQ(announcements.size(), 2u);
    EXPECT_EQ(announcements[0].announcement, "Page loaded");
    EXPECT_EQ(announcements[0].priority, AnnouncementPriority::High);
}

TEST(AccessibilityTest, GetPendingAnnouncements)
{
    AccessibilityManager mgr;
    auto empty = mgr.GetPendingAnnouncements();
    EXPECT_TRUE(empty.empty());

    mgr.Announce("Test");
    EXPECT_EQ(mgr.GetPendingAnnouncements().size(), 1u);
}

TEST(AccessibilityTest, ClearAnnouncements)
{
    AccessibilityManager mgr;
    mgr.Announce("A");
    mgr.Announce("B");
    mgr.ClearAnnouncements();
    EXPECT_TRUE(mgr.GetPendingAnnouncements().empty());
}

// ============================================================================
// Tab order
// ============================================================================

TEST(AccessibilityTest, TabOrder)
{
    AccessibilityManager mgr;

    AccessibleElement e1; e1.label = "C"; e1.tabOrder = 2;
    AccessibleElement e2; e2.label = "A"; e2.tabOrder = 0;
    AccessibleElement e3; e3.label = "B"; e3.tabOrder = 1;
    uint32_t id1 = mgr.RegisterElement(e1);
    uint32_t id2 = mgr.RegisterElement(e2);
    uint32_t id3 = mgr.RegisterElement(e3);

    auto order = mgr.GetTabOrder();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], id2);  // tabOrder 0
    EXPECT_EQ(order[1], id3);  // tabOrder 1
    EXPECT_EQ(order[2], id1);  // tabOrder 2
}

TEST(AccessibilityTest, SetTabOrder)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.label = "Test";
    elem.tabOrder = 5;
    uint32_t id = mgr.RegisterElement(elem);

    EXPECT_TRUE(mgr.SetTabOrder(id, 10));
    const auto* e = mgr.GetElement(id);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->tabOrder, 10);

    EXPECT_FALSE(mgr.SetTabOrder(9999, 0));
}

// ============================================================================
// Tree and search
// ============================================================================

TEST(AccessibilityTest, FindByLabel)
{
    AccessibilityManager mgr;

    AccessibleElement e1; e1.label = "Save";
    AccessibleElement e2; e2.label = "Load";
    AccessibleElement e3; e3.label = "Save";  // Duplicate label
    mgr.RegisterElement(e1);
    mgr.RegisterElement(e2);
    mgr.RegisterElement(e3);

    auto saveResults = mgr.FindByLabel("Save");
    EXPECT_EQ(saveResults.size(), 2u);

    auto loadResults = mgr.FindByLabel("Load");
    EXPECT_EQ(loadResults.size(), 1u);

    auto noneResults = mgr.FindByLabel("Quit");
    EXPECT_TRUE(noneResults.empty());
}

// ============================================================================
// Visual accessibility
// ============================================================================

TEST(AccessibilityTest, HighContrastColors)
{
    AccessibilityManager mgr;
    AccessibilityConfig cfg;
    cfg.enableHighContrast = true;
    mgr.SetConfig(cfg);

    auto colors = mgr.GetHighContrastColors();
    EXPECT_EQ(colors.fg, 0xFFFFFFFF);  // White
    EXPECT_EQ(colors.bg, 0xFF000000);  // Black
}

TEST(AccessibilityTest, ReducedMotion)
{
    AccessibilityManager mgr;
    EXPECT_FALSE(mgr.ShouldReduceMotion());

    AccessibilityConfig cfg;
    cfg.enableReducedMotion = true;
    mgr.SetConfig(cfg);
    EXPECT_TRUE(mgr.ShouldReduceMotion());
}

TEST(AccessibilityTest, FontScale)
{
    AccessibilityManager mgr;
    EXPECT_FLOAT_EQ(mgr.GetFontScale(), 1.0f);

    AccessibilityConfig cfg;
    cfg.fontScaleFactor = 2.0f;
    mgr.SetConfig(cfg);
    EXPECT_FLOAT_EQ(mgr.GetFontScale(), 2.0f);
}

// ============================================================================
// Clear
// ============================================================================

TEST(AccessibilityTest, ClearAll)
{
    AccessibilityManager mgr;
    AccessibleElement elem;
    elem.label = "Test";
    uint32_t id = mgr.RegisterElement(elem);
    mgr.SetFocus(id);
    mgr.Announce("Hello");

    mgr.Clear();
    EXPECT_EQ(mgr.GetElementCount(), 0u);
    EXPECT_EQ(mgr.GetFocusedElement(), 0u);
    EXPECT_TRUE(mgr.GetPendingAnnouncements().empty());
}

// ============================================================================
// Role enum
// ============================================================================

TEST(AccessibilityTest, RoleEnum)
{
    // Verify all role values are distinct
    EXPECT_NE(static_cast<int>(AccessibilityRole::None), static_cast<int>(AccessibilityRole::Button));
    EXPECT_NE(static_cast<int>(AccessibilityRole::Button), static_cast<int>(AccessibilityRole::TextInput));
    EXPECT_NE(static_cast<int>(AccessibilityRole::Slider), static_cast<int>(AccessibilityRole::Checkbox));
    EXPECT_NE(static_cast<int>(AccessibilityRole::Menu), static_cast<int>(AccessibilityRole::MenuItem));
    EXPECT_NE(static_cast<int>(AccessibilityRole::Dialog), static_cast<int>(AccessibilityRole::Alert));
    EXPECT_NE(static_cast<int>(AccessibilityRole::ProgressBar), static_cast<int>(AccessibilityRole::ScrollBar));
}

// ============================================================================
// Focus navigation directions
// ============================================================================

TEST(AccessibilityTest, FocusNavigation)
{
    AccessibilityManager mgr;

    AccessibleElement e1; e1.label = "A"; e1.tabOrder = 0;
    AccessibleElement e2; e2.label = "B"; e2.tabOrder = 1;
    uint32_t id1 = mgr.RegisterElement(e1);
    uint32_t id2 = mgr.RegisterElement(e2);

    mgr.SetFocus(id1);

    // Down acts like Next
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Down));
    EXPECT_EQ(mgr.GetFocusedElement(), id2);

    // Up acts like Previous
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Up));
    EXPECT_EQ(mgr.GetFocusedElement(), id1);

    // Right acts like Next
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Right));
    EXPECT_EQ(mgr.GetFocusedElement(), id2);

    // Left acts like Previous
    EXPECT_TRUE(mgr.NavigateFocus(FocusNavigationDir::Left));
    EXPECT_EQ(mgr.GetFocusedElement(), id1);
}
