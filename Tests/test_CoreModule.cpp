/// @file test_CoreModule.cpp
/// @brief Core module utility tests: StringUtils, ObjectPool, SettingsManager, SaveSystem, EventBus, GameStateMachine

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/StringUtils.h"
#include "Core/ObjectPool.h"
#include "Core/SettingsManager.h"
#include "Core/SaveSystem.h"
#include "Core/EventBus.h"
#include "Core/GameState.h"
#include <filesystem>

using namespace gx;

// ============================================================================
// StringUtils
// ============================================================================

TEST(CoreModuleTest, StringTrim)
{
    EXPECT_EQ(StringUtils::Trim("  hello  "), "hello");
    EXPECT_EQ(StringUtils::Trim("\t\n test \r\n"), "test");
    EXPECT_EQ(StringUtils::Trim(""), "");
    EXPECT_EQ(StringUtils::Trim("   "), "");
    EXPECT_EQ(StringUtils::Trim("nospaces"), "nospaces");
}

TEST(CoreModuleTest, StringSplit)
{
    auto parts = StringUtils::Split("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");

    // Split with string delimiter
    auto parts2 = StringUtils::Split("one::two::three", gx::String("::"));
    ASSERT_EQ(parts2.size(), 3u);
    EXPECT_EQ(parts2[0], "one");
    EXPECT_EQ(parts2[1], "two");
    EXPECT_EQ(parts2[2], "three");
}

TEST(CoreModuleTest, StringReplace)
{
    EXPECT_EQ(StringUtils::Replace("hello world", "world", "earth"), "hello earth");
    EXPECT_EQ(StringUtils::Replace("aaa", "a", "bb"), "bbbbbb");
    // Empty 'from' returns original
    EXPECT_EQ(StringUtils::Replace("abc", "", "x"), "abc");
    // No match returns original
    EXPECT_EQ(StringUtils::Replace("abc", "z", "x"), "abc");
}

TEST(CoreModuleTest, StringToUpper)
{
    EXPECT_EQ(StringUtils::ToUpper("hello"), "HELLO");
    EXPECT_EQ(StringUtils::ToUpper("Hello World"), "HELLO WORLD");
    EXPECT_EQ(StringUtils::ToUpper(""), "");
    EXPECT_EQ(StringUtils::ToUpper("123abc"), "123ABC");
}

TEST(CoreModuleTest, StringToLower)
{
    EXPECT_EQ(StringUtils::ToLower("HELLO"), "hello");
    EXPECT_EQ(StringUtils::ToLower("Hello World"), "hello world");
    EXPECT_EQ(StringUtils::ToLower(""), "");
    EXPECT_EQ(StringUtils::ToLower("ABC123"), "abc123");
}

TEST(CoreModuleTest, StringStartsWith)
{
    EXPECT_TRUE(StringUtils::StartsWith("hello world", "hello"));
    EXPECT_TRUE(StringUtils::StartsWith("abc", "abc"));
    EXPECT_FALSE(StringUtils::StartsWith("hello", "world"));
    EXPECT_TRUE(StringUtils::StartsWith("anything", ""));
    EXPECT_FALSE(StringUtils::StartsWith("short", "longer_than_input"));
}

TEST(CoreModuleTest, StringEndsWith)
{
    EXPECT_TRUE(StringUtils::EndsWith("hello world", "world"));
    EXPECT_TRUE(StringUtils::EndsWith("abc", "abc"));
    EXPECT_FALSE(StringUtils::EndsWith("hello", "world"));
    EXPECT_TRUE(StringUtils::EndsWith("anything", ""));
    EXPECT_FALSE(StringUtils::EndsWith("short", "longer_than_input"));
}

TEST(CoreModuleTest, StringContains)
{
    EXPECT_TRUE(StringUtils::Contains("hello world", "lo wo"));
    EXPECT_TRUE(StringUtils::Contains("abcdef", "cde"));
    EXPECT_FALSE(StringUtils::Contains("hello", "xyz"));
    EXPECT_TRUE(StringUtils::Contains("anything", ""));
}

// ============================================================================
// ObjectPool
// ============================================================================

struct TestPoolItem { int value = 0; };

TEST(CoreModuleTest, ObjectPoolAlloc)
{
    ObjectPool<TestPoolItem> pool(8, false);
    EXPECT_EQ(pool.GetCapacity(), 8u);
    EXPECT_EQ(pool.GetActiveCount(), 0u);

    TestPoolItem* item = pool.Get();
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 1u);
    item->value = 42;
    EXPECT_EQ(item->value, 42);
}

TEST(CoreModuleTest, ObjectPoolFree)
{
    ObjectPool<TestPoolItem> pool(8, false);
    TestPoolItem* a = pool.Get();
    TestPoolItem* b = pool.Get();
    EXPECT_EQ(pool.GetActiveCount(), 2u);

    pool.Release(a);
    EXPECT_EQ(pool.GetActiveCount(), 1u);

    pool.Release(b);
    EXPECT_EQ(pool.GetActiveCount(), 0u);

    // Release nullptr should be safe
    pool.Release(nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(CoreModuleTest, ObjectPoolReset)
{
    ObjectPool<TestPoolItem> pool(16);
    pool.Get();
    pool.Get();
    pool.Get();
    pool.Get();
    pool.Get();
    EXPECT_EQ(pool.GetActiveCount(), 5u);

    pool.ReleaseAll();
    EXPECT_EQ(pool.GetActiveCount(), 0u);
    EXPECT_EQ(pool.GetFreeCount(), pool.GetCapacity());
}

// ============================================================================
// SettingsManager
// ============================================================================

class CoreModuleSettingsTest : public ::testing::Test
{
protected:
    void SetUp() override { SettingsManager::Instance().Clear(); }
    void TearDown() override { SettingsManager::Instance().Clear(); }
};

TEST_F(CoreModuleSettingsTest, SettingsGetSetString)
{
    auto& sm = SettingsManager::Instance();
    sm.SetString("game", "title", "MyGame");
    EXPECT_EQ(sm.GetString("game", "title"), "MyGame");
}

TEST_F(CoreModuleSettingsTest, SettingsGetSetInt)
{
    auto& sm = SettingsManager::Instance();
    sm.SetInt("video", "width", 1920);
    sm.SetInt("video", "height", 1080);
    EXPECT_EQ(sm.GetInt("video", "width"), 1920);
    EXPECT_EQ(sm.GetInt("video", "height"), 1080);
}

TEST_F(CoreModuleSettingsTest, SettingsGetSetFloat)
{
    auto& sm = SettingsManager::Instance();
    sm.SetFloat("audio", "masterVolume", 0.75f);
    EXPECT_FLOAT_EQ(sm.GetFloat("audio", "masterVolume"), 0.75f);
}

TEST_F(CoreModuleSettingsTest, SettingsGetSetBool)
{
    auto& sm = SettingsManager::Instance();
    sm.SetBool("video", "vsync", true);
    EXPECT_TRUE(sm.GetBool("video", "vsync"));
    sm.SetBool("video", "vsync", false);
    EXPECT_FALSE(sm.GetBool("video", "vsync"));
}

TEST_F(CoreModuleSettingsTest, SettingsDefault)
{
    auto& sm = SettingsManager::Instance();
    // Non-existent keys return default values
    EXPECT_EQ(sm.GetInt("missing", "key", 999), 999);
    EXPECT_FLOAT_EQ(sm.GetFloat("missing", "key", 2.5f), 2.5f);
    EXPECT_EQ(sm.GetString("missing", "key", "fallback"), "fallback");
    EXPECT_TRUE(sm.GetBool("missing", "key", true));
}

// ============================================================================
// SaveSystem (in-memory key-value only, no disk writes)
// ============================================================================

class CoreModuleSaveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_saveDir = "test_core_saves_" + std::to_string(::GetCurrentProcessId());
    }
    void TearDown() override
    {
        std::filesystem::remove_all(std::filesystem::path(m_saveDir.c_str()));
    }
    gx::String m_saveDir;
};

TEST_F(CoreModuleSaveTest, SaveSystemSetGetInt)
{
    SaveSystem save(m_saveDir);
    save.SetInt("score", 5000);
    EXPECT_EQ(save.GetInt("score"), 5000);
    // Default for missing key
    EXPECT_EQ(save.GetInt("missing", -1), -1);
}

TEST_F(CoreModuleSaveTest, SaveSystemSetGetFloat)
{
    SaveSystem save(m_saveDir);
    save.SetFloat("playtime", 123.45f);
    EXPECT_FLOAT_EQ(save.GetFloat("playtime"), 123.45f);
    EXPECT_FLOAT_EQ(save.GetFloat("missing", 0.0f), 0.0f);
}

TEST_F(CoreModuleSaveTest, SaveSystemSetGetString)
{
    SaveSystem save(m_saveDir);
    save.SetString("playerName", "Hero");
    EXPECT_EQ(save.GetString("playerName"), "Hero");
    EXPECT_EQ(save.GetString("missing", "none"), "none");
}

TEST_F(CoreModuleSaveTest, SaveSystemHasKey)
{
    SaveSystem save(m_saveDir);
    save.SetInt("level", 10);
    EXPECT_TRUE(save.HasKey("level"));
    EXPECT_FALSE(save.HasKey("nonexistent"));
}

TEST_F(CoreModuleSaveTest, SaveSystemRemoveKey)
{
    SaveSystem save(m_saveDir);
    save.SetString("weapon", "Sword");
    EXPECT_TRUE(save.HasKey("weapon"));
    save.RemoveKey("weapon");
    EXPECT_FALSE(save.HasKey("weapon"));
}

// ============================================================================
// EventBus
// ============================================================================

namespace
{
    struct CoreTestEvent { int code = 0; };
    struct CoreOtherEvent { gx::String message; };
}

class CoreModuleEventBusTest : public ::testing::Test
{
protected:
    void SetUp() override { EventBus::Instance().Clear(); }
    void TearDown() override { EventBus::Instance().Clear(); }
};

TEST_F(CoreModuleEventBusTest, EventBusSubscribeFire)
{
    int received = -1;
    EventBus::Instance().Subscribe<CoreTestEvent>([&](const CoreTestEvent& e) {
        received = e.code;
    });
    EventBus::Instance().Fire(CoreTestEvent{ 777 });
    EXPECT_EQ(received, 777);
}

TEST_F(CoreModuleEventBusTest, EventBusUnsubscribe)
{
    int count = 0;
    auto handle = EventBus::Instance().Subscribe<CoreTestEvent>([&](const CoreTestEvent&) {
        count++;
    });

    EventBus::Instance().Fire(CoreTestEvent{ 1 });
    EXPECT_EQ(count, 1);

    EventBus::Instance().Unsubscribe(handle);
    EventBus::Instance().Fire(CoreTestEvent{ 2 });
    EXPECT_EQ(count, 1); // Should not increment after unsubscribe
}

// ============================================================================
// GameStateMachine
// ============================================================================

TEST(CoreModuleTest, GameStatePush)
{
    GameStateMachine gsm;
    bool entered = false;
    auto id = gsm.RegisterState({
        "Title",
        [&]() { entered = true; },
        nullptr, nullptr, nullptr
    });
    gsm.PushState(id);
    EXPECT_TRUE(entered);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Title");
    EXPECT_EQ(gsm.GetStackDepth(), 1u);
}

TEST(CoreModuleTest, GameStatePop)
{
    GameStateMachine gsm;
    bool titleExited = false;
    auto titleId = gsm.RegisterState({
        "Title",
        nullptr, nullptr, nullptr,
        [&]() { titleExited = true; }
    });
    auto pauseId = gsm.RegisterState({
        "Pause",
        nullptr, nullptr, nullptr, nullptr
    });

    gsm.PushState(titleId);
    gsm.PushState(pauseId);
    EXPECT_EQ(gsm.GetStackDepth(), 2u);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Pause");

    gsm.PopState();
    EXPECT_EQ(gsm.GetStackDepth(), 1u);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Title");
}
