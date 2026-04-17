/// @file queue_from_worker_test.cpp
/// @brief Story 003: QueueFromWorker thread-safe worker route tests
#include <gtest/gtest.h>
#include "Core/EventBus.h"
#include <thread>
#include <atomic>
#include <vector>

namespace {

struct IntEvent { int value = 0; };

class QueueFromWorkerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto& bus = gx::EventBus::Instance();
        bus.SetReplayMode(false);
        bus.Clear();
    }
    void TearDown() override
    {
        auto& bus = gx::EventBus::Instance();
        bus.SetReplayMode(false);
        bus.Clear();
    }
};

TEST_F(QueueFromWorkerTest, SingleWorkerQueueAndDispatch)
{
    auto& bus = gx::EventBus::Instance();
    int count = 0;
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { count++; },
        gx::HandlerCategory::Idempotent);

    std::thread worker([&]() {
        bus.QueueFromWorker(IntEvent{ 42 });
    });
    worker.join();

    bus.DispatchQueued();
    EXPECT_EQ(count, 1);
}

TEST_F(QueueFromWorkerTest, MultipleWorkersNoCrash)
{
    auto& bus = gx::EventBus::Instance();
    std::atomic<int> count{ 0 };
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { count.fetch_add(1, std::memory_order_relaxed); },
        gx::HandlerCategory::Idempotent);

    constexpr int NUM_WORKERS = 4;
    constexpr int EVENTS_PER_WORKER = 100;
    std::vector<std::thread> workers;
    for (int w = 0; w < NUM_WORKERS; ++w)
    {
        workers.emplace_back([&]() {
            for (int i = 0; i < EVENTS_PER_WORKER; ++i)
                bus.QueueFromWorker(IntEvent{ i });
        });
    }
    for (auto& t : workers) t.join();

    bus.DispatchQueued();
    EXPECT_EQ(count.load(), NUM_WORKERS * EVENTS_PER_WORKER);
}

TEST_F(QueueFromWorkerTest, WorkerQueueDrainedBeforeMainQueue)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;
    bus.Subscribe<IntEvent>(
        [&](const IntEvent& e) { order.push_back(e.value); },
        gx::HandlerCategory::Idempotent);

    std::thread worker([&]() {
        bus.QueueFromWorker(IntEvent{ 1 });
    });
    worker.join();

    bus.Queue(IntEvent{ 2 });
    bus.DispatchQueued();

    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}

TEST_F(QueueFromWorkerTest, WorkerEventsRespectReplayMode)
{
    auto& bus = gx::EventBus::Instance();
    int sideEffectCount = 0;
    int idempotentCount = 0;
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { idempotentCount++; },
        gx::HandlerCategory::Idempotent);

    std::thread worker([&]() {
        bus.QueueFromWorker(IntEvent{ 1 });
    });
    worker.join();

    bus.SetReplayMode(true);
    bus.DispatchQueued();

    EXPECT_EQ(sideEffectCount, 0);
    EXPECT_EQ(idempotentCount, 1);
}

TEST_F(QueueFromWorkerTest, EmptyWorkerQueueIsNoOp)
{
    auto& bus = gx::EventBus::Instance();
    int count = 0;
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { count++; },
        gx::HandlerCategory::Idempotent);

    bus.DispatchQueued();
    EXPECT_EQ(count, 0);
}

TEST_F(QueueFromWorkerTest, ClearAlsoEmptiesWorkerQueue)
{
    auto& bus = gx::EventBus::Instance();
    int count = 0;
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { count++; },
        gx::HandlerCategory::Idempotent);

    std::thread worker([&]() {
        bus.QueueFromWorker(IntEvent{ 1 });
    });
    worker.join();

    bus.Clear();
    bus.Subscribe<IntEvent>(
        [&](const IntEvent&) { count++; },
        gx::HandlerCategory::Idempotent);
    bus.DispatchQueued();
    EXPECT_EQ(count, 0);
}

} // namespace
