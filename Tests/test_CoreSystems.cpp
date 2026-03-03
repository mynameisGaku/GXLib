/// @file test_CoreSystems.cpp
/// @brief Phase 2: EventBus、ObjectPool、ActionScheduler、GameStateMachine、SaveSystem、SettingsManagerのテスト

#include <gtest/gtest.h>
#include "Core/EventBus.h"
#include "Core/ObjectPool.h"
#include "Core/ActionScheduler.h"
#include "Core/GameState.h"
#include "Core/SaveSystem.h"
#include "Core/SettingsManager.h"
#include <filesystem>

using namespace gx;

// ============================================================================
// EventBus
// ============================================================================

struct TestEvent { int value = 0; };
struct OtherEvent { float data = 0.0f; };

class EventBusTest : public ::testing::Test
{
protected:
    void SetUp() override { EventBus::Instance().Clear(); }
    void TearDown() override { EventBus::Instance().Clear(); }
};

TEST_F(EventBusTest, SubscribeAndFire)
{
    int received = 0;
    EventBus::Instance().Subscribe<TestEvent>([&](const TestEvent& e) {
        received = e.value;
    });
    EventBus::Instance().Fire(TestEvent{ 42 });
    EXPECT_EQ(received, 42);
}

TEST_F(EventBusTest, MultipleSubscribers)
{
    int count = 0;
    EventBus::Instance().Subscribe<TestEvent>([&](const TestEvent&) { count++; });
    EventBus::Instance().Subscribe<TestEvent>([&](const TestEvent&) { count++; });
    EventBus::Instance().Fire(TestEvent{ 1 });
    EXPECT_EQ(count, 2);
}

TEST_F(EventBusTest, Unsubscribe)
{
    int count = 0;
    auto handle = EventBus::Instance().Subscribe<TestEvent>([&](const TestEvent&) { count++; });
    EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(count, 1);
    EventBus::Instance().Unsubscribe(handle);
    EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(count, 1);
}

TEST_F(EventBusTest, QueueAndDispatch)
{
    int received = 0;
    EventBus::Instance().Subscribe<TestEvent>([&](const TestEvent& e) {
        received = e.value;
    });
    EventBus::Instance().Queue(TestEvent{ 99 });
    EXPECT_EQ(received, 0);  // まだディスパッチされていない
    EventBus::Instance().DispatchQueued();
    EXPECT_EQ(received, 99);
}

TEST_F(EventBusTest, TypeIsolation)
{
    int intReceived = 0;
    float floatReceived = 0.0f;
    EventBus::Instance().Subscribe<TestEvent>([&](const TestEvent& e) { intReceived = e.value; });
    EventBus::Instance().Subscribe<OtherEvent>([&](const OtherEvent& e) { floatReceived = e.data; });

    EventBus::Instance().Fire(TestEvent{ 10 });
    EXPECT_EQ(intReceived, 10);
    EXPECT_FLOAT_EQ(floatReceived, 0.0f);

    EventBus::Instance().Fire(OtherEvent{ 3.14f });
    EXPECT_FLOAT_EQ(floatReceived, 3.14f);
}

// ============================================================================
// ObjectPool
// ============================================================================

struct Bullet { float x = 0, y = 0; bool active = false; };

TEST(ObjectPoolTest, GetAndRelease)
{
    ObjectPool<Bullet> pool(10, false);
    EXPECT_EQ(pool.GetCapacity(), 10u);
    EXPECT_EQ(pool.GetActiveCount(), 0u);

    auto* b1 = pool.Get();
    ASSERT_NE(b1, nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 1u);

    auto* b2 = pool.Get();
    ASSERT_NE(b2, nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 2u);

    pool.Release(b1);
    EXPECT_EQ(pool.GetActiveCount(), 1u);
}

TEST(ObjectPoolTest, AutoExpand)
{
    ObjectPool<int> pool(2, true);
    auto* a = pool.Get();
    auto* b = pool.Get();
    EXPECT_EQ(pool.GetCapacity(), 2u);

    auto* c = pool.Get();  // 自動拡張されるはず
    ASSERT_NE(c, nullptr);
    EXPECT_GT(pool.GetCapacity(), 2u);
}

TEST(ObjectPoolTest, NoAutoExpand)
{
    ObjectPool<int> pool(2, false);
    pool.Get();
    pool.Get();
    auto* third = pool.Get();
    EXPECT_EQ(third, nullptr);
}

TEST(ObjectPoolTest, ReleaseAll)
{
    ObjectPool<int> pool(5);
    pool.Get();
    pool.Get();
    pool.Get();
    EXPECT_EQ(pool.GetActiveCount(), 3u);
    pool.ReleaseAll();
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ObjectPoolTest, ForEachActive)
{
    ObjectPool<int> pool(5);
    auto* a = pool.Get(); *a = 10;
    auto* b = pool.Get(); *b = 20;
    pool.Get(); // default 0

    int sum = 0;
    pool.ForEachActive([&](int& v) { sum += v; });
    EXPECT_EQ(sum, 30);
}

TEST(ObjectPoolTest, ReleaseIf)
{
    ObjectPool<Bullet> pool(10);
    auto* b1 = pool.Get(); b1->active = true;
    auto* b2 = pool.Get(); b2->active = false;
    auto* b3 = pool.Get(); b3->active = true;

    pool.ReleaseIf([](const Bullet& b) { return !b.active; });
    EXPECT_EQ(pool.GetActiveCount(), 2u);
}

// ============================================================================
// ActionScheduler
// ============================================================================

TEST(ActionSchedulerTest, RunAfter)
{
    ActionScheduler sched;
    bool fired = false;
    sched.RunAfter(0.5f, [&]() { fired = true; });

    sched.Update(0.3f);
    EXPECT_FALSE(fired);
    sched.Update(0.3f);
    EXPECT_TRUE(fired);
    EXPECT_EQ(sched.GetActiveCount(), 0);
}

TEST(ActionSchedulerTest, RunEvery)
{
    ActionScheduler sched;
    int count = 0;
    sched.RunEvery(0.1f, [&]() { count++; }, 3);

    for (int i = 0; i < 5; ++i)
        sched.Update(0.1f);

    EXPECT_EQ(count, 3);
}

TEST(ActionSchedulerTest, RunEveryInfinite)
{
    ActionScheduler sched;
    int count = 0;
    sched.RunEvery(0.1f, [&]() { count++; }, 0);

    for (int i = 0; i < 10; ++i)
        sched.Update(0.1f);

    EXPECT_EQ(count, 10);
}

TEST(ActionSchedulerTest, Cancel)
{
    ActionScheduler sched;
    bool fired = false;
    int id = sched.RunAfter(1.0f, [&]() { fired = true; });
    EXPECT_TRUE(sched.IsActive(id));
    sched.Cancel(id);
    sched.Update(2.0f);
    EXPECT_FALSE(fired);
}

TEST(ActionSchedulerTest, RunSequence)
{
    ActionScheduler sched;
    gx::Vector<int> order;

    gx::Vector<SequenceStep> steps = {
        { 0.0f, [&]() { order.push_back(1); } },
        { 0.1f, [&]() { order.push_back(2); } },
        { 0.1f, [&]() { order.push_back(3); } },
    };
    sched.RunSequence(steps);

    sched.Update(0.01f);
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], 1);

    sched.Update(0.1f);
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[1], 2);

    sched.Update(0.1f);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[2], 3);
}

// ============================================================================
// GameStateMachine
// ============================================================================

TEST(GameStateMachineTest, BasicFlow)
{
    GameStateMachine gsm;
    gx::String log;

    auto titleId = gsm.RegisterState({
        "Title",
        [&]() { log += "enter_title;"; },
        [&](float) { log += "update_title;"; },
        [&]() { log += "draw_title;"; },
        [&]() { log += "exit_title;"; }
    });

    auto gameId = gsm.RegisterState({
        "Game",
        [&]() { log += "enter_game;"; },
        [&](float) { log += "update_game;"; },
        nullptr,
        [&]() { log += "exit_game;"; }
    });

    gsm.ChangeState(titleId);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Title");
    EXPECT_TRUE(log.find("enter_title;") != gx::String::npos);

    gsm.Update(0.016f);
    EXPECT_TRUE(log.find("update_title;") != gx::String::npos);

    gsm.Draw();
    EXPECT_TRUE(log.find("draw_title;") != gx::String::npos);

    gsm.ChangeState(gameId);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Game");
    EXPECT_TRUE(log.find("exit_title;") != gx::String::npos);
    EXPECT_TRUE(log.find("enter_game;") != gx::String::npos);
}

TEST(GameStateMachineTest, PushPopState)
{
    GameStateMachine gsm;
    gx::String current;

    auto gameId = gsm.RegisterState({ "Game", nullptr, nullptr, nullptr, nullptr });
    auto pauseId = gsm.RegisterState({ "Pause", nullptr, nullptr, nullptr, nullptr });

    gsm.ChangeState(gameId);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Game");

    gsm.PushState(pauseId);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Pause");
    EXPECT_EQ(gsm.GetStackDepth(), 2u);

    gsm.PopState();
    EXPECT_EQ(gsm.GetCurrentStateName(), "Game");
    EXPECT_EQ(gsm.GetStackDepth(), 1u);
}

TEST(GameStateMachineTest, FindState)
{
    GameStateMachine gsm;
    gsm.RegisterState({ "Alpha", nullptr, nullptr, nullptr, nullptr });
    gsm.RegisterState({ "Beta", nullptr, nullptr, nullptr, nullptr });

    EXPECT_NE(gsm.FindState("Alpha"), UINT32_MAX);
    EXPECT_NE(gsm.FindState("Beta"), UINT32_MAX);
    EXPECT_EQ(gsm.FindState("Gamma"), UINT32_MAX);
}

// ============================================================================
// SaveSystem
// ============================================================================

class SaveSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_saveDir = "test_saves_" + std::to_string(::GetCurrentProcessId());
    }
    void TearDown() override
    {
        std::filesystem::remove_all(std::filesystem::path(m_saveDir.c_str()));
    }
    gx::String m_saveDir;
};

TEST_F(SaveSystemTest, SetAndGet)
{
    SaveSystem save(m_saveDir);
    save.SetInt("score", 1000);
    save.SetFloat("time", 12.5f);
    save.SetString("name", "Player1");
    save.SetBool("completed", true);

    EXPECT_EQ(save.GetInt("score"), 1000);
    EXPECT_FLOAT_EQ(save.GetFloat("time"), 12.5f);
    EXPECT_EQ(save.GetString("name"), "Player1");
    EXPECT_TRUE(save.GetBool("completed"));
}

TEST_F(SaveSystemTest, SaveAndLoad)
{
    {
        SaveSystem save(m_saveDir);
        save.SetInt("level", 5);
        save.SetString("hero", "Knight");
        EXPECT_TRUE(save.SaveToSlot(0, "Chapter 1", 3600.0f));
    }

    {
        SaveSystem save(m_saveDir);
        EXPECT_TRUE(save.LoadFromSlot(0));
        EXPECT_EQ(save.GetInt("level"), 5);
        EXPECT_EQ(save.GetString("hero"), "Knight");
    }
}

TEST_F(SaveSystemTest, SlotList)
{
    SaveSystem save(m_saveDir);
    save.SetInt("x", 1);
    save.SaveToSlot(0, "Save A", 100.0f);
    save.SaveToSlot(2, "Save C", 300.0f);

    auto slots = save.GetSlotList(5);
    ASSERT_EQ(slots.size(), 5u);
    EXPECT_TRUE(slots[0].exists);
    EXPECT_FALSE(slots[1].exists);
    EXPECT_TRUE(slots[2].exists);
    EXPECT_EQ(slots[0].label, "Save A");
}

TEST_F(SaveSystemTest, DeleteSlot)
{
    SaveSystem save(m_saveDir);
    save.SetInt("x", 1);
    save.SaveToSlot(0);
    EXPECT_TRUE(save.DeleteSlot(0));

    auto slots = save.GetSlotList(1);
    EXPECT_FALSE(slots[0].exists);
}

// ============================================================================
// SettingsManager
// ============================================================================

class SettingsManagerTest : public ::testing::Test
{
protected:
    void SetUp() override { SettingsManager::Instance().Clear(); }
    void TearDown() override
    {
        SettingsManager::Instance().Clear();
        std::filesystem::remove("test_settings.json");
    }
};

TEST_F(SettingsManagerTest, SetAndGet)
{
    auto& sm = SettingsManager::Instance();
    sm.SetInt("video", "width", 1920);
    sm.SetFloat("audio", "volume", 0.8f);
    sm.SetString("game", "language", "ja");
    sm.SetBool("video", "fullscreen", true);

    EXPECT_EQ(sm.GetInt("video", "width"), 1920);
    EXPECT_FLOAT_EQ(sm.GetFloat("audio", "volume"), 0.8f);
    EXPECT_EQ(sm.GetString("game", "language"), "ja");
    EXPECT_TRUE(sm.GetBool("video", "fullscreen"));
}

TEST_F(SettingsManagerTest, SaveAndLoad)
{
    auto& sm = SettingsManager::Instance();
    sm.SetInt("video", "width", 1280);
    sm.SetString("game", "name", "TestGame");
    EXPECT_TRUE(sm.SaveToFile("test_settings.json"));

    sm.Clear();
    EXPECT_EQ(sm.GetInt("video", "width"), 0);

    EXPECT_TRUE(sm.LoadFromFile("test_settings.json"));
    EXPECT_EQ(sm.GetString("video", "width"), "1280");
    EXPECT_EQ(sm.GetString("game", "name"), "TestGame");
}

TEST_F(SettingsManagerTest, DefaultValues)
{
    auto& sm = SettingsManager::Instance();
    EXPECT_EQ(sm.GetInt("none", "none", 42), 42);
    EXPECT_FLOAT_EQ(sm.GetFloat("none", "none", 3.14f), 3.14f);
    EXPECT_EQ(sm.GetString("none", "none", "default"), "default");
    EXPECT_TRUE(sm.GetBool("none", "none", true));
}
