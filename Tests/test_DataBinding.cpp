/// @file test_DataBinding.cpp
/// @brief Data binding system unit tests

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/DataBinding.h"

using namespace gx::GUI;

// ============================================================================
// DataProperty
// ============================================================================

TEST(DataBindingTest, DataPropertySetGet)
{
    DataProperty prop("name");
    prop.Set("Hello");
    EXPECT_EQ(prop.Get(), "Hello");
    EXPECT_EQ(prop.GetName(), "name");
}

TEST(DataBindingTest, DataPropertyCallback)
{
    DataProperty prop("val");
    std::string oldVal, newVal;
    prop.OnChanged = [&](const std::string& o, const std::string& n) {
        oldVal = o;
        newVal = n;
    };

    prop.Set("first");
    EXPECT_EQ(oldVal, "");
    EXPECT_EQ(newVal, "first");

    prop.Set("second");
    EXPECT_EQ(oldVal, "first");
    EXPECT_EQ(newVal, "second");
}

TEST(DataBindingTest, DataPropertySilent)
{
    DataProperty prop("val");
    bool called = false;
    prop.OnChanged = [&](const std::string&, const std::string&) { called = true; };

    prop.SetSilent("silent");
    EXPECT_FALSE(called);
    EXPECT_EQ(prop.Get(), "silent");
}

TEST(DataBindingTest, DataPropertyInt)
{
    DataProperty prop("count");
    prop.SetInt(42);
    EXPECT_EQ(prop.GetInt(), 42);
    EXPECT_EQ(prop.Get(), "42");
}

TEST(DataBindingTest, DataPropertyFloat)
{
    DataProperty prop("speed");
    prop.SetFloat(3.14f);
    EXPECT_NEAR(prop.GetFloat(), 3.14f, 0.01f);
}

TEST(DataBindingTest, DataPropertyBool)
{
    DataProperty prop("flag");
    prop.SetBool(true);
    EXPECT_TRUE(prop.GetBool());
    EXPECT_EQ(prop.Get(), "true");

    prop.SetBool(false);
    EXPECT_FALSE(prop.GetBool());
    EXPECT_EQ(prop.Get(), "false");
}

// ============================================================================
// DataContext
// ============================================================================

TEST(DataBindingTest, DataContextSetGet)
{
    DataContext ctx;
    ctx.SetProperty("name", "Alice");
    EXPECT_EQ(ctx.GetProperty("name"), "Alice");
    EXPECT_EQ(ctx.GetProperty("missing"), "");
}

TEST(DataBindingTest, DataContextHas)
{
    DataContext ctx;
    EXPECT_FALSE(ctx.HasProperty("x"));
    ctx.SetProperty("x", "10");
    EXPECT_TRUE(ctx.HasProperty("x"));
}

TEST(DataBindingTest, DataContextRemove)
{
    DataContext ctx;
    ctx.SetProperty("temp", "value");
    EXPECT_TRUE(ctx.RemoveProperty("temp"));
    EXPECT_FALSE(ctx.HasProperty("temp"));
    EXPECT_FALSE(ctx.RemoveProperty("temp"));
}

TEST(DataBindingTest, DataContextPropertyCount)
{
    DataContext ctx;
    EXPECT_EQ(ctx.GetPropertyCount(), 0u);
    ctx.SetProperty("a", "1");
    ctx.SetProperty("b", "2");
    EXPECT_EQ(ctx.GetPropertyCount(), 2u);
}

TEST(DataBindingTest, DataContextPropertyNames)
{
    DataContext ctx;
    ctx.SetProperty("alpha", "1");
    ctx.SetProperty("beta", "2");

    auto names = ctx.GetPropertyNames();
    EXPECT_EQ(names.size(), 2u);

    // Both names should be present (order not guaranteed)
    bool hasAlpha = false, hasBeta = false;
    for (const auto& n : names) {
        if (n == "alpha") hasAlpha = true;
        if (n == "beta") hasBeta = true;
    }
    EXPECT_TRUE(hasAlpha);
    EXPECT_TRUE(hasBeta);
}

TEST(DataBindingTest, DataContextOnChanged)
{
    DataContext ctx;
    std::string lastPropName, lastPropValue;
    ctx.OnPropertyChanged = [&](const std::string& n, const std::string& v) {
        lastPropName = n;
        lastPropValue = v;
    };

    ctx.SetProperty("score", "100");
    EXPECT_EQ(lastPropName, "score");
    EXPECT_EQ(lastPropValue, "100");
}

// ============================================================================
// Binding
// ============================================================================

TEST(DataBindingTest, BindOneWay)
{
    DataBindingManager mgr;
    uint32_t id = mgr.Bind("health", "healthBar", "value", BindingMode::OneWay);
    EXPECT_GT(id, 0u);

    const Binding* b = mgr.GetBinding(id);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sourceProperty, "health");
    EXPECT_EQ(b->targetId, "healthBar");
    EXPECT_EQ(b->mode, BindingMode::OneWay);
}

TEST(DataBindingTest, BindTwoWay)
{
    DataBindingManager mgr;
    uint32_t id = mgr.Bind("name", "nameInput", "text", BindingMode::TwoWay);

    const Binding* b = mgr.GetBinding(id);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->mode, BindingMode::TwoWay);
}

TEST(DataBindingTest, BindOneTime)
{
    DataBindingManager mgr;
    DataContext ctx;
    ctx.SetProperty("label", "Hello");
    mgr.SetDataContext(&ctx);

    uint32_t id = mgr.Bind("label", "titleText", "text", BindingMode::OneTime);
    EXPECT_TRUE(mgr.IsActive(id));

    // After update, OneTime binding should deactivate
    mgr.NotifyPropertyChanged("label");
    mgr.UpdateBindings();
    EXPECT_FALSE(mgr.IsActive(id));
}

TEST(DataBindingTest, Unbind)
{
    DataBindingManager mgr;
    uint32_t id = mgr.Bind("x", "widget", "x");
    EXPECT_TRUE(mgr.Unbind(id));
    EXPECT_FALSE(mgr.Unbind(id));  // Already removed
    EXPECT_EQ(mgr.GetBinding(id), nullptr);
}

TEST(DataBindingTest, UnbindAll)
{
    DataBindingManager mgr;
    mgr.Bind("a", "target1", "x");
    mgr.Bind("b", "target1", "y");
    mgr.Bind("c", "target2", "z");

    mgr.UnbindAll("target1");
    EXPECT_EQ(mgr.GetBindingCount(), 1u);
}

TEST(DataBindingTest, BindingCount)
{
    DataBindingManager mgr;
    EXPECT_EQ(mgr.GetBindingCount(), 0u);
    mgr.Bind("a", "w1", "p1");
    mgr.Bind("b", "w2", "p2");
    EXPECT_EQ(mgr.GetBindingCount(), 2u);
}

TEST(DataBindingTest, UpdateBindings)
{
    DataBindingManager mgr;
    DataContext ctx;
    ctx.SetProperty("score", "0");
    mgr.SetDataContext(&ctx);
    mgr.Bind("score", "scoreLabel", "text");

    mgr.NotifyPropertyChanged("score");
    EXPECT_NO_THROW(mgr.UpdateBindings());
}

TEST(DataBindingTest, BindWithConverter)
{
    DataBindingManager mgr;
    BindingConverter converter;
    converter.toUI = [](const std::string& v) { return "Converted: " + v; };
    converter.fromUI = [](const std::string& v) { return v.substr(11); };

    uint32_t id = mgr.BindWithConverter("raw", "display", "text", converter);
    const Binding* b = mgr.GetBinding(id);
    ASSERT_NE(b, nullptr);
    ASSERT_TRUE(b->converter.toUI != nullptr);
    EXPECT_EQ(b->converter.toUI("hello"), "Converted: hello");
}

TEST(DataBindingTest, SetActive)
{
    DataBindingManager mgr;
    uint32_t id = mgr.Bind("x", "w", "p");
    EXPECT_TRUE(mgr.IsActive(id));

    mgr.SetActive(id, false);
    EXPECT_FALSE(mgr.IsActive(id));

    mgr.SetActive(id, true);
    EXPECT_TRUE(mgr.IsActive(id));
}

TEST(DataBindingTest, IsActive)
{
    DataBindingManager mgr;
    uint32_t id = mgr.Bind("x", "w", "p");
    EXPECT_TRUE(mgr.IsActive(id));
    EXPECT_FALSE(mgr.IsActive(9999));  // Non-existent
}

TEST(DataBindingTest, GetBindingsForProperty)
{
    DataBindingManager mgr;
    mgr.Bind("health", "bar1", "val");
    mgr.Bind("health", "bar2", "val");
    mgr.Bind("mana", "bar3", "val");

    auto healthBindings = mgr.GetBindingsForProperty("health");
    auto manaBindings = mgr.GetBindingsForProperty("mana");
    EXPECT_EQ(healthBindings.size(), 2u);
    EXPECT_EQ(manaBindings.size(), 1u);
}

TEST(DataBindingTest, GetBindingsForTarget)
{
    DataBindingManager mgr;
    mgr.Bind("x", "playerWidget", "posX");
    mgr.Bind("y", "playerWidget", "posY");
    mgr.Bind("z", "enemyWidget", "posZ");

    auto playerBindings = mgr.GetBindingsForTarget("playerWidget");
    auto enemyBindings = mgr.GetBindingsForTarget("enemyWidget");
    EXPECT_EQ(playerBindings.size(), 2u);
    EXPECT_EQ(enemyBindings.size(), 1u);
}

TEST(DataBindingTest, ClearBindings)
{
    DataBindingManager mgr;
    mgr.Bind("a", "w1", "p1");
    mgr.Bind("b", "w2", "p2");

    mgr.Clear();
    EXPECT_EQ(mgr.GetBindingCount(), 0u);
}
