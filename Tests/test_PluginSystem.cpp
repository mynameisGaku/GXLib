/// @file test_PluginSystem.cpp
/// @brief PluginSystem 単体テスト — プラグイン登録・初期化・更新・優先度

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/PluginSystem.h"

using namespace gx;

// =============================================================================
// MockPlugin — テスト用プラグイン実装
// =============================================================================

class MockPlugin : public IPlugin
{
public:
    MockPlugin(const std::string& name, int priority = 0)
        : m_name(name), m_priority(priority) {}

    PluginInfo GetInfo() const override
    {
        return { m_name, "1.0.0", "TestAuthor", "A mock plugin" };
    }

    bool Initialize() override
    {
        initCount++;
        return true;
    }

    void Shutdown() override
    {
        shutdownCount++;
    }

    void Update(float deltaTime) override
    {
        updateCount++;
        lastDeltaTime = deltaTime;
    }

    int GetPriority() const override
    {
        return m_priority;
    }

    int   initCount     = 0;
    int   shutdownCount = 0;
    int   updateCount   = 0;
    float lastDeltaTime = 0.0f;

private:
    std::string m_name;
    int         m_priority;
};

// =============================================================================
// 基本テスト
// =============================================================================

TEST(PluginSystemTest, ConstructDestruct)
{
    PluginSystem system;
    EXPECT_FALSE(system.IsInitialized());
    EXPECT_EQ(system.GetPluginCount(), 0u);
}

TEST(PluginSystemTest, InitializeShutdown)
{
    PluginSystem system;

    system.Initialize();
    EXPECT_TRUE(system.IsInitialized());

    system.Shutdown();
    EXPECT_FALSE(system.IsInitialized());
    EXPECT_EQ(system.GetPluginCount(), 0u);
}

// =============================================================================
// 登録/解除テスト
// =============================================================================

TEST(PluginSystemTest, RegisterPlugin)
{
    PluginSystem system;
    system.Initialize();

    auto plugin = std::make_unique<MockPlugin>("TestPlugin");
    EXPECT_TRUE(system.RegisterPlugin(std::move(plugin)));
    EXPECT_EQ(system.GetPluginCount(), 1u);
    EXPECT_TRUE(system.HasPlugin("TestPlugin"));

    // Duplicate registration should fail
    auto duplicate = std::make_unique<MockPlugin>("TestPlugin");
    EXPECT_FALSE(system.RegisterPlugin(std::move(duplicate)));
    EXPECT_EQ(system.GetPluginCount(), 1u);

    system.Shutdown();
}

TEST(PluginSystemTest, UnregisterPlugin)
{
    PluginSystem system;
    system.Initialize();

    auto plugin = std::make_unique<MockPlugin>("TestPlugin");
    system.RegisterPlugin(std::move(plugin));

    EXPECT_TRUE(system.UnregisterPlugin("TestPlugin"));
    EXPECT_FALSE(system.HasPlugin("TestPlugin"));
    EXPECT_EQ(system.GetPluginCount(), 0u);

    // Unregister non-existent
    EXPECT_FALSE(system.UnregisterPlugin("NoSuchPlugin"));

    system.Shutdown();
}

TEST(PluginSystemTest, GetPlugin)
{
    PluginSystem system;
    system.Initialize();

    auto plugin = std::make_unique<MockPlugin>("MyPlugin");
    MockPlugin* rawPtr = plugin.get();
    system.RegisterPlugin(std::move(plugin));

    IPlugin* found = system.GetPlugin("MyPlugin");
    EXPECT_EQ(found, rawPtr);
    EXPECT_EQ(system.GetPlugin("Missing"), nullptr);

    system.Shutdown();
}

TEST(PluginSystemTest, HasPlugin)
{
    PluginSystem system;
    system.Initialize();

    EXPECT_FALSE(system.HasPlugin("A"));

    auto plugin = std::make_unique<MockPlugin>("A");
    system.RegisterPlugin(std::move(plugin));
    EXPECT_TRUE(system.HasPlugin("A"));
    EXPECT_FALSE(system.HasPlugin("B"));

    system.Shutdown();
}

// =============================================================================
// 一括操作テスト
// =============================================================================

TEST(PluginSystemTest, InitializeAll)
{
    PluginSystem system;
    system.Initialize();

    auto p1 = std::make_unique<MockPlugin>("P1");
    auto p2 = std::make_unique<MockPlugin>("P2");
    MockPlugin* raw1 = p1.get();
    MockPlugin* raw2 = p2.get();
    system.RegisterPlugin(std::move(p1));
    system.RegisterPlugin(std::move(p2));

    system.InitializeAll();
    EXPECT_EQ(raw1->initCount, 1);
    EXPECT_EQ(raw2->initCount, 1);

    system.Shutdown();
}

TEST(PluginSystemTest, ShutdownAll)
{
    PluginSystem system;
    system.Initialize();

    auto p1 = std::make_unique<MockPlugin>("P1");
    MockPlugin* raw1 = p1.get();
    system.RegisterPlugin(std::move(p1));

    system.InitializeAll();
    system.ShutdownAll();
    EXPECT_EQ(raw1->shutdownCount, 1);

    system.Shutdown();
}

TEST(PluginSystemTest, UpdateAll)
{
    PluginSystem system;
    system.Initialize();

    auto p1 = std::make_unique<MockPlugin>("P1");
    auto p2 = std::make_unique<MockPlugin>("P2");
    MockPlugin* raw1 = p1.get();
    MockPlugin* raw2 = p2.get();
    system.RegisterPlugin(std::move(p1));
    system.RegisterPlugin(std::move(p2));

    system.InitializeAll();
    system.UpdateAll(0.016f);

    EXPECT_EQ(raw1->updateCount, 1);
    EXPECT_FLOAT_EQ(raw1->lastDeltaTime, 0.016f);
    EXPECT_EQ(raw2->updateCount, 1);

    system.Shutdown();
}

// =============================================================================
// 情報取得テスト
// =============================================================================

TEST(PluginSystemTest, GetLoadedPlugins)
{
    PluginSystem system;
    system.Initialize();

    system.RegisterPlugin(std::make_unique<MockPlugin>("Alpha"));
    system.RegisterPlugin(std::make_unique<MockPlugin>("Beta"));

    auto infos = system.GetLoadedPlugins();
    EXPECT_EQ(infos.size(), 2u);

    // Check names exist (order may vary due to priority sort)
    bool foundAlpha = false, foundBeta = false;
    for (const auto& info : infos)
    {
        if (info.name == "Alpha") foundAlpha = true;
        if (info.name == "Beta")  foundBeta = true;
    }
    EXPECT_TRUE(foundAlpha);
    EXPECT_TRUE(foundBeta);

    system.Shutdown();
}

TEST(PluginSystemTest, PluginCount)
{
    PluginSystem system;
    system.Initialize();

    EXPECT_EQ(system.GetPluginCount(), 0u);
    system.RegisterPlugin(std::make_unique<MockPlugin>("A"));
    EXPECT_EQ(system.GetPluginCount(), 1u);
    system.RegisterPlugin(std::make_unique<MockPlugin>("B"));
    EXPECT_EQ(system.GetPluginCount(), 2u);
    system.UnregisterPlugin("A");
    EXPECT_EQ(system.GetPluginCount(), 1u);

    system.Shutdown();
}

// =============================================================================
// 有効/無効テスト
// =============================================================================

TEST(PluginSystemTest, PluginEnabled)
{
    PluginSystem system;
    system.Initialize();

    auto p = std::make_unique<MockPlugin>("P");
    MockPlugin* raw = p.get();
    system.RegisterPlugin(std::move(p));

    EXPECT_TRUE(system.IsPluginEnabled("P"));

    system.SetPluginEnabled("P", false);
    EXPECT_FALSE(system.IsPluginEnabled("P"));

    // Disabled plugin should not be initialized
    system.InitializeAll();
    EXPECT_EQ(raw->initCount, 0);

    // Re-enable and initialize
    system.SetPluginEnabled("P", true);
    system.InitializeAll();
    EXPECT_EQ(raw->initCount, 1);

    // Disabled plugin should not be updated
    system.SetPluginEnabled("P", false);
    system.UpdateAll(0.016f);
    EXPECT_EQ(raw->updateCount, 0);

    system.Shutdown();
}

// =============================================================================
// 優先度テスト
// =============================================================================

TEST(PluginSystemTest, PluginPriority)
{
    PluginSystem system;
    system.Initialize();

    // Higher priority (lower number) should be initialized first
    auto pLow  = std::make_unique<MockPlugin>("Low", 10);
    auto pHigh = std::make_unique<MockPlugin>("High", 1);
    auto pMid  = std::make_unique<MockPlugin>("Mid", 5);

    system.RegisterPlugin(std::move(pLow));
    system.RegisterPlugin(std::move(pHigh));
    system.RegisterPlugin(std::move(pMid));

    system.InitializeAll();

    // All should be initialized
    auto infos = system.GetLoadedPlugins();
    ASSERT_EQ(infos.size(), 3u);
    // After InitializeAll, plugins are sorted by priority
    EXPECT_EQ(infos[0].name, "High");
    EXPECT_EQ(infos[1].name, "Mid");
    EXPECT_EQ(infos[2].name, "Low");

    system.Shutdown();
}

// =============================================================================
// コールバックテスト
// =============================================================================

TEST(PluginSystemTest, OnPluginLoadedCallback)
{
    PluginSystem system;
    system.Initialize();

    std::string loadedName;
    std::string unloadedName;

    system.OnPluginLoaded = [&](const std::string& name) { loadedName = name; };
    system.OnPluginUnloaded = [&](const std::string& name) { unloadedName = name; };

    system.RegisterPlugin(std::make_unique<MockPlugin>("CallbackTest"));
    EXPECT_EQ(loadedName, "CallbackTest");

    system.UnregisterPlugin("CallbackTest");
    EXPECT_EQ(unloadedName, "CallbackTest");

    system.Shutdown();
}
