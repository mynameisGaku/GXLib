/// @file replay_mode_test.cpp
/// @brief Story 002: SetReplayMode replay suppression tests
#include <gtest/gtest.h>
#include "Core/EventBus.h"

namespace {

struct TestEvent { int value = 0; };

class ReplayModeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto& bus = gx::EventBus::Instance();
        bus.Clear();
        bus.SetReplayMode(false);
    }
    void TearDown() override
    {
        auto& bus = gx::EventBus::Instance();
        bus.SetReplayMode(false);
        bus.Clear();
    }
};

TEST_F(ReplayModeTest, DefaultReplayModeIsFalse)
{
    EXPECT_FALSE(gx::EventBus::Instance().IsReplayMode());
}

TEST_F(ReplayModeTest, SetReplayModeToggles)
{
    auto& bus = gx::EventBus::Instance();
    bus.SetReplayMode(true);
    EXPECT_TRUE(bus.IsReplayMode());
    bus.SetReplayMode(false);
    EXPECT_FALSE(bus.IsReplayMode());
}

TEST_F(ReplayModeTest, ReplayModeSkipsSideEffect)
{
    auto& bus = gx::EventBus::Instance();
    int idempotentCount = 0;
    int sideEffectCount = 0;

    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { idempotentCount++; },
        gx::HandlerCategory::Idempotent);
    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);

    bus.SetReplayMode(true);
    bus.Fire(TestEvent{});

    EXPECT_EQ(idempotentCount, 1);
    EXPECT_EQ(sideEffectCount, 0);
}

TEST_F(ReplayModeTest, NormalModeFiresBoth)
{
    auto& bus = gx::EventBus::Instance();
    int idempotentCount = 0;
    int sideEffectCount = 0;

    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { idempotentCount++; },
        gx::HandlerCategory::Idempotent);
    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);

    bus.Fire(TestEvent{});
    EXPECT_EQ(idempotentCount, 1);
    EXPECT_EQ(sideEffectCount, 1);
}

TEST_F(ReplayModeTest, ReplayThenNormalRestoresBoth)
{
    auto& bus = gx::EventBus::Instance();
    int idempotentCount = 0;
    int sideEffectCount = 0;

    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { idempotentCount++; },
        gx::HandlerCategory::Idempotent);
    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);

    bus.SetReplayMode(true);
    bus.Fire(TestEvent{});
    EXPECT_EQ(sideEffectCount, 0);

    bus.SetReplayMode(false);
    bus.Fire(TestEvent{});
    EXPECT_EQ(idempotentCount, 2);
    EXPECT_EQ(sideEffectCount, 1);
}

TEST_F(ReplayModeTest, QueueIsNoOpDuringReplay)
{
    auto& bus = gx::EventBus::Instance();
    int count = 0;
    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { count++; },
        gx::HandlerCategory::SideEffect);

    bus.SetReplayMode(true);
    bus.Queue(TestEvent{ 42 });
    bus.SetReplayMode(false);
    bus.DispatchQueued();

    EXPECT_EQ(count, 0);
}

TEST_F(ReplayModeTest, IdempotentRunsMultipleTimesInReplay)
{
    auto& bus = gx::EventBus::Instance();
    int count = 0;
    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { count++; },
        gx::HandlerCategory::Idempotent);

    bus.SetReplayMode(true);
    bus.Fire(TestEvent{});
    bus.Fire(TestEvent{});
    bus.Fire(TestEvent{});

    EXPECT_EQ(count, 3);
}

TEST_F(ReplayModeTest, DispatchQueuedSkipsSideEffectInReplay)
{
    auto& bus = gx::EventBus::Instance();
    int idempotentCount = 0;
    int sideEffectCount = 0;

    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { idempotentCount++; },
        gx::HandlerCategory::Idempotent);
    bus.Subscribe<TestEvent>(
        [&](const TestEvent&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);

    bus.Queue(TestEvent{});
    bus.SetReplayMode(true);
    bus.DispatchQueued();

    EXPECT_EQ(idempotentCount, 1);
    EXPECT_EQ(sideEffectCount, 0);
}

TEST_F(ReplayModeTest, DefaultSubscribeSkippedInReplay)
{
    auto& bus = gx::EventBus::Instance();
    int count = 0;
    bus.Subscribe<TestEvent>([&](const TestEvent&) { count++; });

    bus.SetReplayMode(true);
    bus.Fire(TestEvent{});
    EXPECT_EQ(count, 0);
}

} // namespace
