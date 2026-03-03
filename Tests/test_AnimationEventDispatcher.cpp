/// @file test_AnimationEventDispatcher.cpp
/// @brief AnimationEventDispatcherのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/AnimationEventDispatcher.h"
#include "Graphics/3D/AnimationClip.h"

using namespace gx;

// ヘルパー: 特定の時刻にイベントを持つクリップを作成
static AnimationClip CreateClipWithEvents()
{
    AnimationClip clip;
    clip.SetName("TestClip");
    clip.SetDuration(2.0f);

    AnimationEvent e1;
    e1.time = 0.5f;
    e1.name = "footstep";
    e1.intParam = 1;
    clip.AddEvent(e1);

    AnimationEvent e2;
    e2.time = 1.0f;
    e2.name = "attack_hit";
    e2.floatParam = 10.0f;
    clip.AddEvent(e2);

    AnimationEvent e3;
    e3.time = 1.5f;
    e3.name = "footstep";
    e3.intParam = 2;
    clip.AddEvent(e3);

    return clip;
}

TEST(AnimationEventDispatcher, DefaultState)
{
    AnimationEventDispatcher dispatcher;
    EXPECT_EQ(dispatcher.GetHandlerCount(), 0u);
    EXPECT_EQ(dispatcher.GetFiredEventCount(), 0u);
}

TEST(AnimationEventDispatcher, RegisterHandler)
{
    AnimationEventDispatcher dispatcher;
    dispatcher.RegisterHandler("footstep", [](const AnimationEvent&) {});
    EXPECT_EQ(dispatcher.GetHandlerCount(), 1u);
    EXPECT_TRUE(dispatcher.HasHandler("footstep"));
}

TEST(AnimationEventDispatcher, UnregisterHandler)
{
    AnimationEventDispatcher dispatcher;
    dispatcher.RegisterHandler("footstep", [](const AnimationEvent&) {});
    EXPECT_TRUE(dispatcher.HasHandler("footstep"));

    dispatcher.UnregisterHandler("footstep");
    EXPECT_FALSE(dispatcher.HasHandler("footstep"));
    EXPECT_EQ(dispatcher.GetHandlerCount(), 0u);
}

TEST(AnimationEventDispatcher, HasHandler_ReturnsFalseForUnregistered)
{
    AnimationEventDispatcher dispatcher;
    EXPECT_FALSE(dispatcher.HasHandler("nonexistent"));
}

TEST(AnimationEventDispatcher, ClearAllHandlers)
{
    AnimationEventDispatcher dispatcher;
    dispatcher.RegisterHandler("footstep", [](const AnimationEvent&) {});
    dispatcher.RegisterHandler("attack_hit", [](const AnimationEvent&) {});
    EXPECT_EQ(dispatcher.GetHandlerCount(), 2u);

    dispatcher.ClearAllHandlers();
    EXPECT_EQ(dispatcher.GetHandlerCount(), 0u);
    EXPECT_FALSE(dispatcher.HasHandler("footstep"));
    EXPECT_FALSE(dispatcher.HasHandler("attack_hit"));
}

TEST(AnimationEventDispatcher, GetHandlerCount)
{
    AnimationEventDispatcher dispatcher;
    EXPECT_EQ(dispatcher.GetHandlerCount(), 0u);

    dispatcher.RegisterHandler("a", [](const AnimationEvent&) {});
    EXPECT_EQ(dispatcher.GetHandlerCount(), 1u);

    dispatcher.RegisterHandler("b", [](const AnimationEvent&) {});
    EXPECT_EQ(dispatcher.GetHandlerCount(), 2u);

    dispatcher.RegisterHandler("c", [](const AnimationEvent&) {});
    EXPECT_EQ(dispatcher.GetHandlerCount(), 3u);
}

TEST(AnimationEventDispatcher, Update_FiresEventInTimeRange)
{
    AnimationEventDispatcher dispatcher;
    AnimationClip clip = CreateClipWithEvents();

    int footstepCount = 0;
    dispatcher.RegisterHandler("footstep", [&](const AnimationEvent& e) {
        footstepCount++;
    });

    // 時刻0.0から0.6まで更新 -- t=0.5でfootstepが発火するはず
    dispatcher.Update(&clip, 0.0f, 0.6f);
    EXPECT_EQ(footstepCount, 1);
    EXPECT_EQ(dispatcher.GetFiredEventCount(), 1u);
}

TEST(AnimationEventDispatcher, Update_DoesNotFireOutsideRange)
{
    AnimationEventDispatcher dispatcher;
    AnimationClip clip = CreateClipWithEvents();

    int callCount = 0;
    dispatcher.RegisterHandler("footstep", [&](const AnimationEvent&) {
        callCount++;
    });

    // 時刻0.6から0.9まで更新 -- この範囲にイベントなし
    dispatcher.Update(&clip, 0.6f, 0.9f);
    EXPECT_EQ(callCount, 0);
}

TEST(AnimationEventDispatcher, GlobalHandler_ReceivesAllEvents)
{
    AnimationEventDispatcher dispatcher;
    AnimationClip clip = CreateClipWithEvents();

    int globalCount = 0;
    gx::Vector<gx::String> receivedNames;
    dispatcher.RegisterGlobalHandler([&](const AnimationEvent& e) {
        globalCount++;
        receivedNames.push_back(e.name);
    });

    // 0.0から2.0まで更新 -- 全3イベントが発火するはず
    dispatcher.Update(&clip, 0.0f, 2.0f);
    EXPECT_EQ(globalCount, 3);
    EXPECT_EQ(receivedNames.size(), 3u);
}

TEST(AnimationEventDispatcher, FiredEventCount_TracksCorrectly)
{
    AnimationEventDispatcher dispatcher;
    AnimationClip clip = CreateClipWithEvents();

    dispatcher.RegisterGlobalHandler([](const AnimationEvent&) {});

    EXPECT_EQ(dispatcher.GetFiredEventCount(), 0u);

    // 範囲[0, 1.1]でイベント発火 -- t=0.5とt=1.0のイベントがヒットするはず
    dispatcher.Update(&clip, 0.0f, 1.1f);
    EXPECT_EQ(dispatcher.GetFiredEventCount(), 2u);

    // 範囲[1.1, 1.6]でもう1つ発火 -- t=1.5のイベントがヒットするはず
    dispatcher.Update(&clip, 1.1f, 1.6f);
    EXPECT_EQ(dispatcher.GetFiredEventCount(), 3u);
}

TEST(AnimationEventDispatcher, ResetFiredCount)
{
    AnimationEventDispatcher dispatcher;
    AnimationClip clip = CreateClipWithEvents();

    dispatcher.RegisterGlobalHandler([](const AnimationEvent&) {});
    dispatcher.Update(&clip, 0.0f, 2.0f);
    EXPECT_GT(dispatcher.GetFiredEventCount(), 0u);

    dispatcher.ResetFiredCount();
    EXPECT_EQ(dispatcher.GetFiredEventCount(), 0u);
}

TEST(AnimationEventDispatcher, NamedHandler_TakesPriority)
{
    AnimationEventDispatcher dispatcher;
    AnimationClip clip = CreateClipWithEvents();

    bool namedCalled = false;
    bool globalCalled = false;

    dispatcher.RegisterHandler("footstep", [&](const AnimationEvent&) {
        namedCalled = true;
    });
    dispatcher.RegisterGlobalHandler([&](const AnimationEvent&) {
        globalCalled = true;
    });

    // t=0.5のfootstepイベントを超えて更新
    dispatcher.Update(&clip, 0.0f, 0.6f);

    // 名前付きハンドラが呼ばれているはず
    EXPECT_TRUE(namedCalled);
    // グローバルハンドラも呼ばれることを期待
    EXPECT_TRUE(globalCalled);
}

TEST(AnimationEventDispatcher, AnimationClip_AddAndGetEvents)
{
    AnimationClip clip;
    clip.SetName("Test");
    clip.SetDuration(3.0f);

    EXPECT_EQ(clip.GetEvents().size(), 0u);

    AnimationEvent e;
    e.time = 1.0f;
    e.name = "hit";
    e.floatParam = 5.0f;
    e.intParam = 42;
    e.stringParam = "effect_blood";
    clip.AddEvent(e);

    ASSERT_EQ(clip.GetEvents().size(), 1u);
    EXPECT_EQ(clip.GetEvents()[0].name, "hit");
    EXPECT_FLOAT_EQ(clip.GetEvents()[0].time, 1.0f);
    EXPECT_FLOAT_EQ(clip.GetEvents()[0].floatParam, 5.0f);
    EXPECT_EQ(clip.GetEvents()[0].intParam, 42);
    EXPECT_EQ(clip.GetEvents()[0].stringParam, "effect_blood");
}

TEST(AnimationEventDispatcher, AnimationClip_ClearEvents)
{
    AnimationClip clip;
    AnimationEvent e;
    e.time = 0.5f;
    e.name = "step";
    clip.AddEvent(e);
    clip.AddEvent(e);
    EXPECT_EQ(clip.GetEvents().size(), 2u);

    clip.ClearEvents();
    EXPECT_EQ(clip.GetEvents().size(), 0u);
}
