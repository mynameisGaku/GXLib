/// @file test_TimelineEditor.cpp
#include <gtest/gtest.h>
#include "Editor/TimelineEditor.h"
using namespace gx;

// CurveEvaluator tests
TEST(CurveEvaluatorTest, Linear) {
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Linear, 0.5f), 0.5f);
}
TEST(CurveEvaluatorTest, LinearZero) {
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Linear, 0.0f), 0.0f);
}
TEST(CurveEvaluatorTest, LinearOne) {
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Linear, 1.0f), 1.0f);
}
TEST(CurveEvaluatorTest, EaseIn) {
    float v = CurveEvaluator::EvaluateInterpolation(InterpolationMode::EaseIn, 0.5f);
    EXPECT_FLOAT_EQ(v, 0.25f); // 0.5 * 0.5
}
TEST(CurveEvaluatorTest, EaseOut) {
    float v = CurveEvaluator::EvaluateInterpolation(InterpolationMode::EaseOut, 0.5f);
    EXPECT_FLOAT_EQ(v, 0.75f); // 0.5 * (2 - 0.5)
}
TEST(CurveEvaluatorTest, EaseInOut) {
    float v = CurveEvaluator::EvaluateInterpolation(InterpolationMode::EaseInOut, 0.5f);
    EXPECT_FLOAT_EQ(v, 0.5f); // 3*(0.25) - 2*(0.125) = 0.75 - 0.25 = 0.5
}
TEST(CurveEvaluatorTest, Step) {
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Step, 0.5f), 0.0f);
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Step, 1.0f), 1.0f);
}
TEST(CurveEvaluatorTest, Bezier) {
    float v = CurveEvaluator::EvaluateInterpolation(InterpolationMode::Bezier, 0.5f);
    EXPECT_FLOAT_EQ(v, 0.5f); // same as EaseInOut (smoothstep)
}
TEST(CurveEvaluatorTest, Clamp) {
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Linear, -0.5f), 0.0f);
    EXPECT_FLOAT_EQ(CurveEvaluator::EvaluateInterpolation(InterpolationMode::Linear, 1.5f), 1.0f);
}

// TimelineEditor tests
TEST(TimelineEditorTest, DefaultState) {
    TimelineEditor te;
    EXPECT_EQ(te.GetTrackCount(), 0u);
    EXPECT_FALSE(te.IsPlaying());
    EXPECT_FALSE(te.IsPaused());
    EXPECT_FLOAT_EQ(te.GetCurrentTime(), 0.0f);
    EXPECT_FLOAT_EQ(te.GetDuration(), 10.0f);
    EXPECT_FLOAT_EQ(te.GetPlaybackSpeed(), 1.0f);
    EXPECT_FALSE(te.IsLooping());
}
TEST(TimelineEditorTest, AddTrack) {
    TimelineEditor te;
    uint32_t idx = te.AddTrack("Position", EditorTrackType::Transform);
    EXPECT_EQ(idx, 0u);
    EXPECT_EQ(te.GetTrackCount(), 1u);
    auto* t = te.GetTrack(0);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->name, "Position");
    EXPECT_EQ(t->type, EditorTrackType::Transform);
}
TEST(TimelineEditorTest, RemoveTrack) {
    TimelineEditor te;
    te.AddTrack("A", EditorTrackType::Property);
    te.AddTrack("B", EditorTrackType::Event);
    te.RemoveTrack(0);
    EXPECT_EQ(te.GetTrackCount(), 1u);
    EXPECT_EQ(te.GetTrack(0)->name, "B");
}
TEST(TimelineEditorTest, AddKeyframe) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    uint32_t ki = te.AddKeyframe(0, 1.0f, 10.0f, 20.0f, 30.0f);
    EXPECT_EQ(te.GetTrack(0)->keyframes.size(), 1u);
    EXPECT_FLOAT_EQ(te.GetTrack(0)->keyframes[ki].value[0], 10.0f);
}
TEST(TimelineEditorTest, AddKeyframeSorted) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 2.0f, 2.0f);
    te.AddKeyframe(0, 0.5f, 0.5f);
    te.AddKeyframe(0, 1.0f, 1.0f);
    auto& kfs = te.GetTrack(0)->keyframes;
    EXPECT_EQ(kfs.size(), 3u);
    EXPECT_LT(kfs[0].time, kfs[1].time);
    EXPECT_LT(kfs[1].time, kfs[2].time);
}
TEST(TimelineEditorTest, RemoveKeyframe) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 0.0f, 0.0f);
    te.AddKeyframe(0, 1.0f, 1.0f);
    te.RemoveKeyframe(0, 0);
    EXPECT_EQ(te.GetTrack(0)->keyframes.size(), 1u);
}
TEST(TimelineEditorTest, MoveKeyframe) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 0.0f, 10.0f);
    te.AddKeyframe(0, 2.0f, 30.0f);
    te.MoveKeyframe(0, 0, 1.5f);
    auto& kfs = te.GetTrack(0)->keyframes;
    EXPECT_FLOAT_EQ(kfs[0].time, 1.5f);
}
TEST(TimelineEditorTest, SetKeyframeValue) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 1.0f, 0.0f);
    te.SetKeyframeValue(0, 0, 5.0f, 6.0f, 7.0f);
    auto& kf = te.GetTrack(0)->keyframes[0];
    EXPECT_FLOAT_EQ(kf.value[0], 5.0f);
    EXPECT_FLOAT_EQ(kf.value[1], 6.0f);
    EXPECT_FLOAT_EQ(kf.value[2], 7.0f);
}
TEST(TimelineEditorTest, SetKeyframeInterpolation) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 0.0f, 0.0f);
    te.SetKeyframeInterpolation(0, 0, InterpolationMode::EaseIn);
    EXPECT_EQ(te.GetTrack(0)->keyframes[0].interpolation, InterpolationMode::EaseIn);
}
TEST(TimelineEditorTest, PlayPauseStop) {
    TimelineEditor te;
    te.Play();
    EXPECT_TRUE(te.IsPlaying());
    EXPECT_FALSE(te.IsPaused());
    te.Pause();
    EXPECT_TRUE(te.IsPaused());
    te.Stop();
    EXPECT_FALSE(te.IsPlaying());
    EXPECT_FALSE(te.IsPaused());
    EXPECT_FLOAT_EQ(te.GetCurrentTime(), 0.0f);
}
TEST(TimelineEditorTest, SetCurrentTime) {
    TimelineEditor te;
    te.SetCurrentTime(5.0f);
    EXPECT_FLOAT_EQ(te.GetCurrentTime(), 5.0f);
}
TEST(TimelineEditorTest, SetCurrentTimeClamp) {
    TimelineEditor te;
    te.SetDuration(10.0f);
    te.SetCurrentTime(20.0f);
    EXPECT_LE(te.GetCurrentTime(), 10.0f);
    te.SetCurrentTime(-5.0f);
    EXPECT_GE(te.GetCurrentTime(), 0.0f);
}
TEST(TimelineEditorTest, Duration) {
    TimelineEditor te;
    te.SetDuration(30.0f);
    EXPECT_FLOAT_EQ(te.GetDuration(), 30.0f);
}
TEST(TimelineEditorTest, DurationMinClamp) {
    TimelineEditor te;
    te.SetDuration(-5.0f);
    EXPECT_GT(te.GetDuration(), 0.0f);
}
TEST(TimelineEditorTest, PlaybackSpeed) {
    TimelineEditor te;
    te.SetPlaybackSpeed(2.0f);
    EXPECT_FLOAT_EQ(te.GetPlaybackSpeed(), 2.0f);
}
TEST(TimelineEditorTest, PlaybackSpeedClamp) {
    TimelineEditor te;
    te.SetPlaybackSpeed(-1.0f);
    EXPECT_GE(te.GetPlaybackSpeed(), 0.0f);
}
TEST(TimelineEditorTest, Looping) {
    TimelineEditor te;
    te.SetLooping(true);
    EXPECT_TRUE(te.IsLooping());
    te.SetLooping(false);
    EXPECT_FALSE(te.IsLooping());
}
TEST(TimelineEditorTest, MuteTrack) {
    TimelineEditor te;
    te.AddTrack("A", EditorTrackType::Property);
    te.MuteTrack(0, true);
    EXPECT_TRUE(te.IsTrackMuted(0));
    te.MuteTrack(0, false);
    EXPECT_FALSE(te.IsTrackMuted(0));
}
TEST(TimelineEditorTest, LockTrack) {
    TimelineEditor te;
    te.AddTrack("A", EditorTrackType::Property);
    te.LockTrack(0, true);
    EXPECT_TRUE(te.IsTrackLocked(0));
}
TEST(TimelineEditorTest, EvaluateTrackLinear) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 0.0f, 0.0f);
    te.AddKeyframe(0, 1.0f, 10.0f);
    float v[3];
    EXPECT_TRUE(te.EvaluateTrack(0, 0.5f, v));
    EXPECT_NEAR(v[0], 5.0f, 0.01f);
}
TEST(TimelineEditorTest, EvaluateTrackEaseIn) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 0.0f, 0.0f);
    te.AddKeyframe(0, 1.0f, 10.0f);
    te.SetKeyframeInterpolation(0, 0, InterpolationMode::EaseIn);
    float v[3];
    EXPECT_TRUE(te.EvaluateTrack(0, 0.5f, v));
    EXPECT_NEAR(v[0], 2.5f, 0.01f); // EaseIn: 0.5*0.5 = 0.25, 10*0.25 = 2.5
}
TEST(TimelineEditorTest, EvaluateTrackEmpty) {
    TimelineEditor te;
    te.AddTrack("Empty", EditorTrackType::Property);
    float v[3];
    EXPECT_FALSE(te.EvaluateTrack(0, 0.0f, v));
}
TEST(TimelineEditorTest, EvaluateTrackBeforeFirst) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 1.0f, 5.0f);
    float v[3];
    EXPECT_TRUE(te.EvaluateTrack(0, 0.0f, v));
    EXPECT_FLOAT_EQ(v[0], 5.0f);
}
TEST(TimelineEditorTest, EvaluateTrackAfterLast) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 0.0f, 5.0f);
    float v[3];
    EXPECT_TRUE(te.EvaluateTrack(0, 10.0f, v));
    EXPECT_FLOAT_EQ(v[0], 5.0f);
}
TEST(TimelineEditorTest, CopyPasteKeyframe) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    te.AddKeyframe(0, 1.0f, 100.0f, 200.0f, 300.0f);
    EXPECT_FALSE(te.HasClipboard());
    te.CopyKeyframe(0, 0);
    EXPECT_TRUE(te.HasClipboard());
    uint32_t pastedIdx = te.PasteKeyframe(0, 2.0f);
    EXPECT_NE(pastedIdx, UINT32_MAX);
    EXPECT_EQ(te.GetTrack(0)->keyframes.size(), 2u);
    // Find the pasted keyframe (at time 2.0)
    bool found = false;
    for (auto& kf : te.GetTrack(0)->keyframes) {
        if (kf.time == 2.0f) { found = true; EXPECT_FLOAT_EQ(kf.value[0], 100.0f); }
    }
    EXPECT_TRUE(found);
}
TEST(TimelineEditorTest, PasteWithoutCopy) {
    TimelineEditor te;
    te.AddTrack("Pos", EditorTrackType::Transform);
    EXPECT_EQ(te.PasteKeyframe(0, 1.0f), UINT32_MAX);
}
TEST(TimelineEditorTest, UndoRedo) {
    TimelineEditor te;
    te.AddTrack("A", EditorTrackType::Property);
    EXPECT_EQ(te.GetTrackCount(), 1u);
    EXPECT_TRUE(te.CanUndo());

    te.Undo();
    EXPECT_EQ(te.GetTrackCount(), 0u);
    EXPECT_TRUE(te.CanRedo());

    te.Redo();
    EXPECT_EQ(te.GetTrackCount(), 1u);
}
TEST(TimelineEditorTest, UndoEmpty) {
    TimelineEditor te;
    EXPECT_FALSE(te.CanUndo());
    te.Undo(); // should not crash
    EXPECT_EQ(te.GetTrackCount(), 0u);
}
TEST(TimelineEditorTest, GetTrackOutOfRange) {
    TimelineEditor te;
    EXPECT_EQ(te.GetTrack(0), nullptr);
    EXPECT_FALSE(te.IsTrackMuted(99));
    EXPECT_FALSE(te.IsTrackLocked(99));
}
TEST(TimelineEditorTest, EvaluateTrackOutOfRange) {
    TimelineEditor te;
    float v[3];
    EXPECT_FALSE(te.EvaluateTrack(99, 0.0f, v));
}
