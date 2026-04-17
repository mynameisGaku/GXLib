/// @file animation_bridge_test.cpp
/// @brief Story 004: AnimationEventDispatcher global bus bridge tests
#include <gtest/gtest.h>
#include "Core/EventBus.h"
#include "Graphics/3D/AnimationEventDispatcher.h"
#include "Graphics/3D/AnimationClip.h"

namespace {

class AnimationBridgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto& bus = gx::EventBus::Instance();
        bus.SetReplayMode(false);
        bus.Clear();
        m_dispatcher.ClearAllHandlers();
        m_dispatcher.ResetFiredCount();
        m_dispatcher.SetGlobalBusBridge(false);
    }
    void TearDown() override
    {
        gx::EventBus::Instance().SetReplayMode(false);
        gx::EventBus::Instance().Clear();
    }

    gx::AnimationEventDispatcher m_dispatcher;

    gx::AnimationClip MakeClipWithEvent(const gx::String& name, float time)
    {
        gx::AnimationClip clip;
        gx::AnimationEvent evt;
        evt.name = name;
        evt.time = time;
        evt.floatParam = 1.0f;
        evt.intParam = 42;
        clip.AddEvent(evt);
        clip.SetDuration(2.0f);
        return clip;
    }
};

TEST_F(AnimationBridgeTest, BridgeOffNoGlobalEmission)
{
    auto clip = MakeClipWithEvent("footstep", 0.5f);
    int globalCount = 0;
    gx::EventBus::Instance().Subscribe<gx::AnimationEventFired>(
        [&](const gx::AnimationEventFired&) { globalCount++; },
        gx::HandlerCategory::Idempotent);

    m_dispatcher.SetGlobalBusBridge(false);
    m_dispatcher.Update(&clip, 0.0f, 1.0f);

    EXPECT_EQ(globalCount, 0);
}

TEST_F(AnimationBridgeTest, BridgeOnEmitsToGlobalBus)
{
    auto clip = MakeClipWithEvent("footstep", 0.5f);
    gx::String receivedName;
    gx::EventBus::Instance().Subscribe<gx::AnimationEventFired>(
        [&](const gx::AnimationEventFired& e) { receivedName = e.eventName; },
        gx::HandlerCategory::Idempotent);

    m_dispatcher.SetGlobalBusBridge(true);
    m_dispatcher.Update(&clip, 0.0f, 1.0f);

    EXPECT_EQ(receivedName, "footstep");
}

TEST_F(AnimationBridgeTest, BridgeEmitsCorrectParams)
{
    auto clip = MakeClipWithEvent("attack_hit", 0.3f);
    float receivedFloat = 0.0f;
    int receivedInt = 0;
    gx::EventBus::Instance().Subscribe<gx::AnimationEventFired>(
        [&](const gx::AnimationEventFired& e) {
            receivedFloat = e.floatParam;
            receivedInt = e.intParam;
        },
        gx::HandlerCategory::Idempotent);

    m_dispatcher.SetGlobalBusBridge(true);
    m_dispatcher.Update(&clip, 0.0f, 1.0f);

    EXPECT_FLOAT_EQ(receivedFloat, 1.0f);
    EXPECT_EQ(receivedInt, 42);
}

TEST_F(AnimationBridgeTest, BridgeRespectsReplayMode)
{
    auto clip = MakeClipWithEvent("footstep", 0.5f);
    int sideEffectCount = 0;
    gx::EventBus::Instance().Subscribe<gx::AnimationEventFired>(
        [&](const gx::AnimationEventFired&) { sideEffectCount++; },
        gx::HandlerCategory::SideEffect);

    m_dispatcher.SetGlobalBusBridge(true);
    gx::EventBus::Instance().SetReplayMode(true);
    m_dispatcher.Update(&clip, 0.0f, 1.0f);

    EXPECT_EQ(sideEffectCount, 0);
}

TEST_F(AnimationBridgeTest, LocalHandlerStillFiresWithBridge)
{
    auto clip = MakeClipWithEvent("footstep", 0.5f);
    int localCount = 0;
    int globalCount = 0;
    m_dispatcher.RegisterHandler("footstep",
        [&](const gx::AnimationEvent&) { localCount++; });
    gx::EventBus::Instance().Subscribe<gx::AnimationEventFired>(
        [&](const gx::AnimationEventFired&) { globalCount++; },
        gx::HandlerCategory::Idempotent);

    m_dispatcher.SetGlobalBusBridge(true);
    m_dispatcher.Update(&clip, 0.0f, 1.0f);

    EXPECT_EQ(localCount, 1);
    EXPECT_EQ(globalCount, 1);
}

TEST_F(AnimationBridgeTest, IsGlobalBusBridgedReflectsState)
{
    EXPECT_FALSE(m_dispatcher.IsGlobalBusBridged());
    m_dispatcher.SetGlobalBusBridge(true);
    EXPECT_TRUE(m_dispatcher.IsGlobalBusBridged());
    m_dispatcher.SetGlobalBusBridge(false);
    EXPECT_FALSE(m_dispatcher.IsGlobalBusBridged());
}

} // namespace
