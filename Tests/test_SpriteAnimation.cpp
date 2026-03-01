/// @file test_SpriteAnimation.cpp
/// @brief SpriteAnimator (clip-based) and Animation2D tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/Rendering/SpriteAnimator.h"
#include "Graphics/Rendering/Animation2D.h"

using namespace gx;

// =========================================================================
// SpriteAnimClip defaults
// =========================================================================

TEST(SpriteAnimClip, DefaultValues)
{
    SpriteAnimClip clip;
    EXPECT_TRUE(clip.name.empty());
    EXPECT_EQ(clip.startFrame, 0u);
    EXPECT_EQ(clip.endFrame, 0u);
    EXPECT_FLOAT_EQ(clip.fps, 10.0f);
    EXPECT_TRUE(clip.loop);
    EXPECT_TRUE(clip.events.empty());
    EXPECT_FLOAT_EQ(clip.speed, 1.0f);
}

// =========================================================================
// SpriteAnimEvent defaults
// =========================================================================

TEST(SpriteAnimEvent, DefaultValues)
{
    SpriteAnimEvent evt;
    EXPECT_EQ(evt.frame, 0u);
    EXPECT_TRUE(evt.name.empty());
    EXPECT_EQ(evt.callback, nullptr);
}

// =========================================================================
// SpriteAnimTransition defaults
// =========================================================================

TEST(SpriteAnimTransition, DefaultValues)
{
    SpriteAnimTransition trans;
    EXPECT_TRUE(trans.fromClip.empty());
    EXPECT_TRUE(trans.toClip.empty());
    EXPECT_FLOAT_EQ(trans.exitTime, 1.0f);
    EXPECT_FLOAT_EQ(trans.blendDuration, 0.0f);
}

// =========================================================================
// SpriteAnimator tests
// =========================================================================

TEST(SpriteAnimator, DefaultState)
{
    SpriteAnimator anim;
    EXPECT_FALSE(anim.IsPlaying());
    EXPECT_FALSE(anim.IsTransitioning());
    EXPECT_EQ(anim.GetCurrentFrame(), 0u);
    EXPECT_TRUE(anim.GetCurrentClipName().empty());
    EXPECT_FLOAT_EQ(anim.GetSpeed(), 1.0f);
    EXPECT_EQ(anim.GetDirection(), 1);
}

TEST(SpriteAnimator, AddClipAndPlay)
{
    SpriteAnimator anim;
    SpriteAnimClip clip;
    clip.name = "walk";
    clip.startFrame = 0;
    clip.endFrame = 3;
    clip.fps = 10.0f;
    clip.loop = true;
    anim.AddClip(clip);

    anim.Play("walk");
    EXPECT_TRUE(anim.IsPlaying());
    EXPECT_EQ(anim.GetCurrentClipName(), "walk");
}

TEST(SpriteAnimator, GetClip)
{
    SpriteAnimator anim;
    SpriteAnimClip clip;
    clip.name = "idle";
    clip.startFrame = 0;
    clip.endFrame = 2;
    anim.AddClip(clip);

    const SpriteAnimClip* found = anim.GetClip("idle");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "idle");
    EXPECT_EQ(found->endFrame, 2u);

    const SpriteAnimClip* notFound = anim.GetClip("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST(SpriteAnimator, RemoveClip)
{
    SpriteAnimator anim;
    SpriteAnimClip clip;
    clip.name = "run";
    clip.startFrame = 0;
    clip.endFrame = 5;
    anim.AddClip(clip);

    ASSERT_NE(anim.GetClip("run"), nullptr);
    anim.RemoveClip("run");
    EXPECT_EQ(anim.GetClip("run"), nullptr);
}

TEST(SpriteAnimator, SetSpeed)
{
    SpriteAnimator anim;
    EXPECT_FLOAT_EQ(anim.GetSpeed(), 1.0f);
    anim.SetSpeed(2.0f);
    EXPECT_FLOAT_EQ(anim.GetSpeed(), 2.0f);
    anim.SetSpeed(0.5f);
    EXPECT_FLOAT_EQ(anim.GetSpeed(), 0.5f);
}

TEST(SpriteAnimator, SetDirection)
{
    SpriteAnimator anim;
    EXPECT_EQ(anim.GetDirection(), 1);
    anim.SetDirection(-1);
    EXPECT_EQ(anim.GetDirection(), -1);
    anim.SetDirection(1);
    EXPECT_EQ(anim.GetDirection(), 1);
}

TEST(SpriteAnimator, UpdateAdvancesFrame)
{
    SpriteAnimator anim;
    SpriteAnimClip clip;
    clip.name = "walk";
    clip.startFrame = 0;
    clip.endFrame = 3;
    clip.fps = 10.0f;
    clip.loop = true;
    anim.AddClip(clip);

    anim.Play("walk");
    EXPECT_EQ(anim.GetCurrentFrame(), 0u);

    // At 10 fps, each frame is 0.1 sec. After 0.15 sec, should be on frame 1
    anim.Update(0.15f);
    EXPECT_EQ(anim.GetCurrentFrame(), 1u);
}

TEST(SpriteAnimator, PlayNonexistentClipNoCrash)
{
    SpriteAnimator anim;
    // Playing a nonexistent clip should not crash
    anim.Play("nonexistent");
    SUCCEED();
}

TEST(SpriteAnimator, BlendAlphaDefault)
{
    SpriteAnimator anim;
    EXPECT_FLOAT_EQ(anim.GetBlendAlpha(), 0.0f);
}

TEST(SpriteAnimator, AddTransitionNoCrash)
{
    SpriteAnimator anim;
    SpriteAnimTransition trans;
    trans.fromClip = "walk";
    trans.toClip = "idle";
    trans.exitTime = 0.8f;
    trans.blendDuration = 0.2f;
    anim.AddTransition(trans);
    SUCCEED();
}

// =========================================================================
// Animation2D tests
// =========================================================================

TEST(Animation2D, DefaultState)
{
    Animation2D anim;
    EXPECT_EQ(anim.GetCurrentHandle(), -1);
    EXPECT_EQ(anim.GetCurrentFrameIndex(), 0);
    EXPECT_FALSE(anim.IsFinished());
}

TEST(Animation2D, AddFramesAndUpdate)
{
    Animation2D anim;
    int handles[] = { 10, 20, 30 };
    anim.AddFrames(handles, 3, 0.1f);

    EXPECT_EQ(anim.GetCurrentHandle(), 10);

    anim.Update(0.15f); // past first frame
    EXPECT_EQ(anim.GetCurrentHandle(), 20);
}

TEST(Animation2D, LoopByDefault)
{
    Animation2D anim;
    int handles[] = { 10, 20 };
    anim.AddFrames(handles, 2, 0.1f);

    // Update past all frames
    anim.Update(0.25f);
    EXPECT_FALSE(anim.IsFinished()); // still playing (looped)
}

TEST(Animation2D, NonLoopFinishes)
{
    Animation2D anim;
    anim.SetLoop(false);
    int handles[] = { 10, 20 };
    anim.AddFrames(handles, 2, 0.1f);

    anim.Update(0.25f);
    EXPECT_TRUE(anim.IsFinished());
}

TEST(Animation2D, Reset)
{
    Animation2D anim;
    int handles[] = { 10, 20, 30 };
    anim.AddFrames(handles, 3, 0.1f);
    anim.Update(0.15f);
    EXPECT_EQ(anim.GetCurrentFrameIndex(), 1);

    anim.Reset();
    EXPECT_EQ(anim.GetCurrentFrameIndex(), 0);
    EXPECT_EQ(anim.GetCurrentHandle(), 10);
}

TEST(Animation2D, SetSpeed)
{
    Animation2D anim;
    int handles[] = { 10, 20, 30 };
    anim.AddFrames(handles, 3, 0.1f);

    anim.SetSpeed(2.0f);
    // At 2x speed, 0.05 real sec = 0.1 anim sec -> should advance 1 frame
    anim.Update(0.05f);
    EXPECT_EQ(anim.GetCurrentFrameIndex(), 1);
}
