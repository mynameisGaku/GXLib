/// @file dispatch_order_test.cpp
/// @brief Story 005: Dispatch order determinism verification
#include <gtest/gtest.h>
#include "Core/EventBus.h"
#include <vector>

namespace {

struct OrderEvent { int tag = 0; };

class DispatchOrderTest : public ::testing::Test
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
        gx::EventBus::Instance().SetReplayMode(false);
        gx::EventBus::Instance().Clear();
    }
};

TEST_F(DispatchOrderTest, InsertionOrderPreserved100Handlers)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;
    for (int i = 0; i < 100; ++i)
    {
        bus.Subscribe<OrderEvent>(
            [&order, i](const OrderEvent&) { order.push_back(i); },
            gx::HandlerCategory::Idempotent);
    }
    bus.Fire(OrderEvent{});

    ASSERT_EQ(order.size(), 100u);
    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(order[i], i);
}

TEST_F(DispatchOrderTest, ReverseInsertionOrderPreserved)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;
    for (int i = 99; i >= 0; --i)
    {
        bus.Subscribe<OrderEvent>(
            [&order, i](const OrderEvent&) { order.push_back(i); },
            gx::HandlerCategory::Idempotent);
    }
    bus.Fire(OrderEvent{});

    ASSERT_EQ(order.size(), 100u);
    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(order[i], 99 - i);
}

TEST_F(DispatchOrderTest, UnsubscribePreservesRemainingOrder)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;

    auto hA = bus.Subscribe<OrderEvent>(
        [&](const OrderEvent&) { order.push_back(0); },
        gx::HandlerCategory::Idempotent);
    bus.Subscribe<OrderEvent>(
        [&](const OrderEvent&) { order.push_back(1); },
        gx::HandlerCategory::Idempotent);
    bus.Subscribe<OrderEvent>(
        [&](const OrderEvent&) { order.push_back(2); },
        gx::HandlerCategory::Idempotent);

    bus.Unsubscribe(hA);
    bus.Fire(OrderEvent{});

    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}

TEST_F(DispatchOrderTest, ReentrantSubscribeDeferred)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;
    bool secondSubscribed = false;

    bus.Subscribe<OrderEvent>(
        [&](const OrderEvent&) {
            order.push_back(1);
            if (!secondSubscribed)
            {
                secondSubscribed = true;
                bus.Subscribe<OrderEvent>(
                    [&](const OrderEvent&) { order.push_back(2); },
                    gx::HandlerCategory::Idempotent);
            }
        },
        gx::HandlerCategory::Idempotent);

    bus.Fire(OrderEvent{});
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], 1);

    order.clear();
    bus.Fire(OrderEvent{});
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}

TEST_F(DispatchOrderTest, ReentrantUnsubscribeDeferred)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;
    gx::EventHandle hB;

    bus.Subscribe<OrderEvent>(
        [&](const OrderEvent&) {
            order.push_back(1);
            bus.Unsubscribe(hB);
        },
        gx::HandlerCategory::Idempotent);
    hB = bus.Subscribe<OrderEvent>(
        [&](const OrderEvent&) { order.push_back(2); },
        gx::HandlerCategory::Idempotent);

    bus.Fire(OrderEvent{});
    EXPECT_EQ(order.size(), 2u);

    order.clear();
    bus.Fire(OrderEvent{});
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], 1);
}

TEST_F(DispatchOrderTest, DispatchQueuedFIFO)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> order;
    bus.Subscribe<OrderEvent>(
        [&](const OrderEvent& e) { order.push_back(e.tag); },
        gx::HandlerCategory::Idempotent);

    bus.Queue(OrderEvent{ 10 });
    bus.Queue(OrderEvent{ 20 });
    bus.Queue(OrderEvent{ 30 });
    bus.DispatchQueued();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 10);
    EXPECT_EQ(order[1], 20);
    EXPECT_EQ(order[2], 30);
}

TEST_F(DispatchOrderTest, OrderStableAcrossReplayMode)
{
    auto& bus = gx::EventBus::Instance();
    std::vector<int> normalOrder;
    std::vector<int> replayOrder;

    for (int i = 0; i < 10; ++i)
    {
        bus.Subscribe<OrderEvent>(
            [&normalOrder, &replayOrder, i](const OrderEvent&) {
                if (gx::EventBus::Instance().IsReplayMode())
                    replayOrder.push_back(i);
                else
                    normalOrder.push_back(i);
            },
            gx::HandlerCategory::Idempotent);
    }

    bus.Fire(OrderEvent{});
    bus.SetReplayMode(true);
    bus.Fire(OrderEvent{});
    bus.SetReplayMode(false);

    ASSERT_EQ(normalOrder.size(), 10u);
    ASSERT_EQ(replayOrder.size(), 10u);
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(normalOrder[i], i);
        EXPECT_EQ(replayOrder[i], i);
    }
}

} // namespace
