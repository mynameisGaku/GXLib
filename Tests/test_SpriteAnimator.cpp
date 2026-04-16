/// @file test_SpriteAnimator.cpp
/// @brief SpriteAnimator テスト — クリップ管理、再生、遷移、イベント

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/Rendering/SpriteAnimator.h"

using namespace gx;

// ==================== クリップ管理 ====================

TEST(SpriteAnimatorTest, AddClip_StoresClip)
{
    SpriteAnimator animator;
    SpriteAnimClip clip;
    clip.name = "idle";
    clip.startFrame = 0;
    clip.endFrame = 3;
    clip.fps = 10.0f;
    clip.loop = true;
    animator.AddClip(clip);
    EXPECT_NE(animator.GetClip("idle"), nullptr);
}

TEST(SpriteAnimatorTest, AddClip_MultipleClips)
{
    SpriteAnimator animator;
    SpriteAnimClip c1; c1.name = "idle"; c1.startFrame = 0; c1.endFrame = 3; c1.fps = 10.0f;
    SpriteAnimClip c2; c2.name = "walk"; c2.startFrame = 4; c2.endFrame = 11; c2.fps = 12.0f;
    animator.AddClip(c1);
    animator.AddClip(c2);
    EXPECT_NE(animator.GetClip("idle"), nullptr);
    EXPECT_NE(animator.GetClip("walk"), nullptr);
}

TEST(SpriteAnimatorTest, RemoveClip_RemovesIt)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 0; clip.endFrame = 3; clip.fps = 10.0f;
    animator.AddClip(clip);
    animator.RemoveClip("idle");
    EXPECT_EQ(animator.GetClip("idle"), nullptr);
}

TEST(SpriteAnimatorTest, GetClip_UnknownReturnsNull)
{
    SpriteAnimator animator;
    EXPECT_EQ(animator.GetClip("nonexistent"), nullptr);
}

TEST(SpriteAnimatorTest, GetClip_ReturnsCorrectData)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "run"; clip.startFrame = 5; clip.endFrame = 12; clip.fps = 15.0f; clip.loop = false;
    animator.AddClip(clip);
    auto* c = animator.GetClip("run");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->startFrame, 5u);
    EXPECT_EQ(c->endFrame, 12u);
    EXPECT_NEAR(c->fps, 15.0f, 1e-5f);
    EXPECT_FALSE(c->loop);
}

// ==================== 再生制御 ====================

TEST(SpriteAnimatorTest, DefaultState_NotPlaying)
{
    SpriteAnimator animator;
    EXPECT_FALSE(animator.IsPlaying());
}

TEST(SpriteAnimatorTest, DefaultState_CurrentFrameZero)
{
    SpriteAnimator animator;
    EXPECT_EQ(animator.GetCurrentFrame(), 0u);
}

TEST(SpriteAnimatorTest, Play_StartsPlaying)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 0; clip.endFrame = 3; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("idle");
    EXPECT_TRUE(animator.IsPlaying());
}

TEST(SpriteAnimatorTest, Play_SetsCurrentClipName)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "walk"; clip.startFrame = 0; clip.endFrame = 7; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("walk");
    EXPECT_EQ(animator.GetCurrentClipName(), "walk");
}

TEST(SpriteAnimatorTest, Play_StartsFromFirstFrame)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 4; clip.endFrame = 8; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("idle");
    EXPECT_EQ(animator.GetCurrentFrame(), 4u);
}

TEST(SpriteAnimatorTest, Update_AdvancesFrame)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 0; clip.endFrame = 9; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("idle");
    animator.Update(0.1f); // 10fps * 0.1s = 1 frame
    EXPECT_EQ(animator.GetCurrentFrame(), 1u);
}

TEST(SpriteAnimatorTest, Update_LoopWraps)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 0; clip.endFrame = 3; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("idle");
    animator.Update(0.5f); // 10fps * 0.5s = 5 frames, wraps in range [0,3] -> (5 % 4) = 1
    uint32_t frame = animator.GetCurrentFrame();
    EXPECT_GE(frame, 0u);
    EXPECT_LE(frame, 3u);
}

TEST(SpriteAnimatorTest, Update_NonLoopStopsAtEnd)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "die"; clip.startFrame = 0; clip.endFrame = 3; clip.fps = 10.0f; clip.loop = false;
    animator.AddClip(clip);
    animator.Play("die");
    animator.Update(1.0f); // Way past end
    EXPECT_EQ(animator.GetCurrentFrame(), 3u);
}

// ==================== 速度と方向 ====================

TEST(SpriteAnimatorTest, DefaultSpeed_IsOne)
{
    SpriteAnimator animator;
    EXPECT_NEAR(animator.GetSpeed(), 1.0f, 1e-5f);
}

TEST(SpriteAnimatorTest, SetSpeed_ChangesPlaybackRate)
{
    SpriteAnimator animator;
    animator.SetSpeed(2.0f);
    EXPECT_NEAR(animator.GetSpeed(), 2.0f, 1e-5f);
}

TEST(SpriteAnimatorTest, SetSpeed_DoubleAdvancesTwiceAsFast)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 0; clip.endFrame = 19; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("idle");
    animator.SetSpeed(2.0f);
    animator.Update(0.1f); // 10fps * 2.0 speed * 0.1s = 2 frames
    EXPECT_EQ(animator.GetCurrentFrame(), 2u);
}

TEST(SpriteAnimatorTest, DefaultDirection_IsForward)
{
    SpriteAnimator animator;
    EXPECT_EQ(animator.GetDirection(), 1);
}

TEST(SpriteAnimatorTest, SetDirection_Reverse)
{
    SpriteAnimator animator;
    animator.SetDirection(-1);
    EXPECT_EQ(animator.GetDirection(), -1);
}

// ==================== クロスフェード ====================

TEST(SpriteAnimatorTest, DefaultNotTransitioning)
{
    SpriteAnimator animator;
    EXPECT_FALSE(animator.IsTransitioning());
}

TEST(SpriteAnimatorTest, Transition_WithBlendDuration)
{
    SpriteAnimator animator;
    SpriteAnimClip c1; c1.name = "idle"; c1.startFrame = 0; c1.endFrame = 3; c1.fps = 10.0f; c1.loop = true;
    SpriteAnimClip c2; c2.name = "walk"; c2.startFrame = 4; c2.endFrame = 11; c2.fps = 12.0f; c2.loop = true;
    animator.AddClip(c1);
    animator.AddClip(c2);
    SpriteAnimTransition tr; tr.fromClip = "idle"; tr.toClip = "walk"; tr.exitTime = 1.0f; tr.blendDuration = 0.5f;
    animator.AddTransition(tr);
    animator.Play("idle");
    animator.Play("walk"); // Should start crossfade since transition exists
    EXPECT_TRUE(animator.IsTransitioning());
}

TEST(SpriteAnimatorTest, BlendAlpha_DefaultZero)
{
    SpriteAnimator animator;
    EXPECT_NEAR(animator.GetBlendAlpha(), 0.0f, 1e-5f);
}

TEST(SpriteAnimatorTest, BlendAlpha_DuringTransition)
{
    SpriteAnimator animator;
    SpriteAnimClip c1; c1.name = "idle"; c1.startFrame = 0; c1.endFrame = 3; c1.fps = 10.0f; c1.loop = true;
    SpriteAnimClip c2; c2.name = "walk"; c2.startFrame = 4; c2.endFrame = 11; c2.fps = 12.0f; c2.loop = true;
    animator.AddClip(c1);
    animator.AddClip(c2);
    SpriteAnimTransition tr; tr.fromClip = "idle"; tr.toClip = "walk"; tr.exitTime = 1.0f; tr.blendDuration = 1.0f;
    animator.AddTransition(tr);
    animator.Play("idle");
    animator.Play("walk");
    animator.Update(0.5f); // Half way through 1.0s blend
    float alpha = animator.GetBlendAlpha();
    EXPECT_GT(alpha, 0.0f);
    EXPECT_LT(alpha, 1.0f);
}

// ==================== アニメーションイベント ====================

TEST(SpriteAnimatorTest, Event_FiresAtFrame)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "attack"; clip.startFrame = 0; clip.endFrame = 9; clip.fps = 10.0f; clip.loop = false;
    bool eventFired = false;
    SpriteAnimEvent evt; evt.frame = 5; evt.name = "hit"; evt.callback = [&]() { eventFired = true; };
    clip.events.push_back(evt);
    animator.AddClip(clip);
    animator.Play("attack");
    animator.Update(0.3f); // Frame 3, not yet
    EXPECT_FALSE(eventFired);
    animator.Update(0.3f); // Frame 6, passed frame 5
    EXPECT_TRUE(eventFired);
}

TEST(SpriteAnimatorTest, Event_MultipleEvents)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "combo"; clip.startFrame = 0; clip.endFrame = 19; clip.fps = 10.0f; clip.loop = false;
    int count = 0;
    SpriteAnimEvent e1; e1.frame = 3; e1.name = "hit1"; e1.callback = [&]() { count++; };
    SpriteAnimEvent e2; e2.frame = 8; e2.name = "hit2"; e2.callback = [&]() { count++; };
    clip.events.push_back(e1);
    clip.events.push_back(e2);
    animator.AddClip(clip);
    animator.Play("combo");
    animator.Update(1.0f); // Advance past both events
    EXPECT_EQ(count, 2);
}

// ==================== 遷移ルール ====================

TEST(SpriteAnimatorTest, Transition_ExitTimeTriggersAutoTransition)
{
    SpriteAnimator animator;
    SpriteAnimClip c1; c1.name = "idle"; c1.startFrame = 0; c1.endFrame = 3; c1.fps = 10.0f; c1.loop = true;
    SpriteAnimClip c2; c2.name = "walk"; c2.startFrame = 4; c2.endFrame = 11; c2.fps = 12.0f; c2.loop = true;
    animator.AddClip(c1);
    animator.AddClip(c2);
    SpriteAnimTransition tr; tr.fromClip = "idle"; tr.toClip = "walk"; tr.exitTime = 0.5f; tr.blendDuration = 0.0f;
    animator.AddTransition(tr);
    animator.Play("idle");
    // exitTime = 0.5 of clip duration. Clip has 4 frames at 10fps = 0.4s duration. exitTime at 0.2s.
    animator.Update(0.3f); // Past exit time
    // After auto-transition completes, should be playing "walk"
    EXPECT_EQ(animator.GetCurrentClipName(), "walk");
}

TEST(SpriteAnimatorTest, Play_UnknownClipDoesNothing)
{
    SpriteAnimator animator;
    animator.Play("nonexistent");
    EXPECT_FALSE(animator.IsPlaying());
}

TEST(SpriteAnimatorTest, Play_SameClipResets)
{
    SpriteAnimator animator;
    SpriteAnimClip clip; clip.name = "idle"; clip.startFrame = 0; clip.endFrame = 9; clip.fps = 10.0f; clip.loop = true;
    animator.AddClip(clip);
    animator.Play("idle");
    animator.Update(0.5f); // Advance some frames
    animator.Play("idle"); // Reset
    EXPECT_EQ(animator.GetCurrentFrame(), 0u);
}
