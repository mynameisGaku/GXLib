/// @file test_ActionMapping.cpp
/// @brief ActionMapping structure tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Input/ActionMapping.h"

using namespace GX;

TEST(ActionMappingTest, DefineAction_Exists)
{
    ActionMapping am;
    am.DefineAction("Jump", { InputBinding::Key(VK_SPACE) });
    const auto& state = am.GetAction("Jump");
    // Initial state should be all false/zero
    EXPECT_FALSE(state.pressed);
    EXPECT_FALSE(state.triggered);
    EXPECT_FALSE(state.released);
    EXPECT_NEAR(state.value, 0.0f, 1e-5f);
}

TEST(ActionMappingTest, GetActionValue_Undefined)
{
    ActionMapping am;
    float val = am.GetActionValue("NonExistent");
    EXPECT_NEAR(val, 0.0f, 1e-5f);
}

TEST(ActionMappingTest, IsActionPressed_Undefined)
{
    ActionMapping am;
    EXPECT_FALSE(am.IsActionPressed("NonExistent"));
    EXPECT_FALSE(am.IsActionTriggered("NonExistent"));
    EXPECT_FALSE(am.IsActionReleased("NonExistent"));
}

TEST(ActionMappingTest, RemoveAction_Removes)
{
    ActionMapping am;
    am.DefineAction("Fire", { InputBinding::Key('F') });
    am.RemoveAction("Fire");
    EXPECT_NEAR(am.GetActionValue("Fire"), 0.0f, 1e-5f);
    EXPECT_FALSE(am.IsActionPressed("Fire"));
}

TEST(ActionMappingTest, Clear_RemovesAll)
{
    ActionMapping am;
    am.DefineAction("Jump", { InputBinding::Key(VK_SPACE) });
    am.DefineAction("Fire", { InputBinding::Key('F') });
    am.DefineAction("Move", { InputBinding::KeyAxis('W', 1.0f) });
    am.Clear();
    EXPECT_NEAR(am.GetActionValue("Jump"), 0.0f, 1e-5f);
    EXPECT_NEAR(am.GetActionValue("Fire"), 0.0f, 1e-5f);
    EXPECT_NEAR(am.GetActionValue("Move"), 0.0f, 1e-5f);
}

TEST(ActionMappingTest, KeyBinding_Factory)
{
    InputBinding b = InputBinding::Key(VK_SPACE);
    EXPECT_EQ(b.type, InputBindingType::KeyboardKey);
    EXPECT_EQ(b.keyCode, VK_SPACE);
    EXPECT_NEAR(b.scale, 1.0f, 1e-5f);
}

TEST(ActionMappingTest, KeyAxisBinding_Factory)
{
    InputBinding b = InputBinding::KeyAxis('W', 1.0f);
    EXPECT_EQ(b.type, InputBindingType::KeyboardKey);
    EXPECT_EQ(b.keyCode, 'W');
    EXPECT_NEAR(b.scale, 1.0f, 1e-5f);
}

TEST(ActionMappingTest, PadBtnBinding_Factory)
{
    InputBinding b = InputBinding::PadBtn(0, 1);
    EXPECT_EQ(b.type, InputBindingType::GamepadButton);
    EXPECT_EQ(b.keyCode, 0);
    EXPECT_EQ(b.padIndex, 1);
}
