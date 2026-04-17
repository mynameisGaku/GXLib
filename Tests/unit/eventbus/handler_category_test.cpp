/// @file handler_category_test.cpp
/// @brief Story 001: HandlerCategory enum + Subscribe overload tests
#include <gtest/gtest.h>
#include "Core/EventBus.h"

namespace {

struct TestEvent { int value = 0; };
struct OtherEvent { float data = 0.0f; };

class HandlerCategoryTest : public ::testing::Test
{
protected:
    void SetUp() override { gx::EventBus::Instance().Clear(); }
    void TearDown() override { gx::EventBus::Instance().Clear(); }
};

TEST_F(HandlerCategoryTest, EnumValuesExist)
{
    gx::HandlerCategory a = gx::HandlerCategory::Idempotent;
    gx::HandlerCategory b = gx::HandlerCategory::SideEffect;
    EXPECT_NE(static_cast<int>(a), static_cast<int>(b));
}

TEST_F(HandlerCategoryTest, TwoArgSubscribeWithIdempotent)
{
    int count = 0;
    auto handle = gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { count++; },
        gx::HandlerCategory::Idempotent);
    EXPECT_TRUE(static_cast<bool>(handle));
    gx::EventBus::Instance().Fire(TestEvent{ 42 });
    EXPECT_EQ(count, 1);
}

TEST_F(HandlerCategoryTest, TwoArgSubscribeWithSideEffect)
{
    int count = 0;
    auto handle = gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { count++; },
        gx::HandlerCategory::SideEffect);
    EXPECT_TRUE(static_cast<bool>(handle));
    gx::EventBus::Instance().Fire(TestEvent{ 42 });
    EXPECT_EQ(count, 1);
}

TEST_F(HandlerCategoryTest, OneArgSubscribeDefaultsToSideEffect)
{
    int count = 0;
    gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { count++; });
    gx::EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(count, 1);
}

TEST_F(HandlerCategoryTest, MixedCategoriesAllFire)
{
    int idempotentCount = 0;
    int sideEffectCount = 0;
    gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { idempotentCount++; },
        gx::HandlerCategory::Idempotent);
    gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);

    gx::EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(idempotentCount, 1);
    EXPECT_EQ(sideEffectCount, 1);
}

TEST_F(HandlerCategoryTest, UnsubscribeWorksWithCategorisedHandler)
{
    int count = 0;
    auto handle = gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { count++; },
        gx::HandlerCategory::Idempotent);

    gx::EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(count, 1);

    gx::EventBus::Instance().Unsubscribe(handle);
    gx::EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(count, 1);
}

TEST_F(HandlerCategoryTest, TypeIsolationPreservedWithCategories)
{
    int testCount = 0;
    int otherCount = 0;
    gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent&) { testCount++; },
        gx::HandlerCategory::Idempotent);
    gx::EventBus::Instance().Subscribe<OtherEvent>(
        [&](const OtherEvent&) { otherCount++; },
        gx::HandlerCategory::SideEffect);

    gx::EventBus::Instance().Fire(TestEvent{});
    EXPECT_EQ(testCount, 1);
    EXPECT_EQ(otherCount, 0);

    gx::EventBus::Instance().Fire(OtherEvent{});
    EXPECT_EQ(testCount, 1);
    EXPECT_EQ(otherCount, 1);
}

TEST_F(HandlerCategoryTest, QueueAndDispatchWithCategories)
{
    int count = 0;
    gx::EventBus::Instance().Subscribe<TestEvent>(
        [&](const TestEvent& e) { count += e.value; },
        gx::HandlerCategory::Idempotent);

    gx::EventBus::Instance().Queue(TestEvent{ 10 });
    EXPECT_EQ(count, 0);
    gx::EventBus::Instance().DispatchQueued();
    EXPECT_EQ(count, 10);
}

} // namespace
