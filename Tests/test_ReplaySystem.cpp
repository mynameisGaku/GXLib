/// @file test_ReplaySystem.cpp
/// @brief ReplaySystem 単体テスト — リプレイ録画・再生・シーク・ループ

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/ReplaySystem.h"

using namespace gx;

// ヘルパー: ダミーエンティティ状態を生成
static ReplayEntityState MakeEntity(uint32_t id, float x, float y, float z)
{
    ReplayEntityState state{};
    state.entityId    = id;
    state.position[0] = x;
    state.position[1] = y;
    state.position[2] = z;
    state.rotation[0] = 0.0f;
    state.rotation[1] = 0.0f;
    state.rotation[2] = 0.0f;
    state.rotation[3] = 1.0f;
    state.scale[0]    = 1.0f;
    state.scale[1]    = 1.0f;
    state.scale[2]    = 1.0f;
    return state;
}

// =============================================================================
// ReplayRecorder テスト
// =============================================================================

TEST(ReplayRecorderTest, RecorderConstruct)
{
    ReplayRecorder recorder;
    EXPECT_FALSE(recorder.IsRecording());
    EXPECT_EQ(recorder.GetFrameCount(), 0u);
    EXPECT_FLOAT_EQ(recorder.GetDuration(), 0.0f);
}

TEST(ReplayRecorderTest, StartStopRecording)
{
    ReplayRecorder recorder;

    recorder.StartRecording("TestReplay");
    EXPECT_TRUE(recorder.IsRecording());
    EXPECT_EQ(recorder.GetHeader().name, "TestReplay");

    recorder.StopRecording();
    EXPECT_FALSE(recorder.IsRecording());
}

TEST(ReplayRecorderTest, RecordFrame)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f); // No interval restriction
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(0.1f, entities);
    recorder.RecordFrame(0.2f, entities);

    EXPECT_EQ(recorder.GetFrameCount(), 3u);
    const auto& frames = recorder.GetFrames();
    ASSERT_EQ(frames.size(), 3u);
    EXPECT_FLOAT_EQ(frames[0].timestamp, 0.0f);
    EXPECT_FLOAT_EQ(frames[2].timestamp, 0.2f);

    recorder.StopRecording();
}

TEST(ReplayRecorderTest, RecorderFrameCount)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    for (int i = 0; i < 10; ++i)
    {
        recorder.RecordFrame(static_cast<float>(i) * 0.1f, entities);
    }

    EXPECT_EQ(recorder.GetFrameCount(), 10u);
    recorder.StopRecording();
    EXPECT_EQ(recorder.GetHeader().frameCount, 10u);
}

TEST(ReplayRecorderTest, RecorderDuration)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(1.0f, entities);
    recorder.RecordFrame(3.5f, entities);

    recorder.StopRecording();
    EXPECT_FLOAT_EQ(recorder.GetDuration(), 2.5f);
}

TEST(ReplayRecorderTest, RecorderMaxFrames)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.SetMaxFrames(5);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    for (int i = 0; i < 20; ++i)
    {
        recorder.RecordFrame(static_cast<float>(i), entities);
    }

    EXPECT_EQ(recorder.GetFrameCount(), 5u);
    recorder.StopRecording();
}

TEST(ReplayRecorderTest, RecorderInterval)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.5f); // 0.5 second interval
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    // Record every 0.1s — only some should be accepted
    for (int i = 0; i < 20; ++i)
    {
        recorder.RecordFrame(static_cast<float>(i) * 0.1f, entities);
    }

    // First frame always recorded, then at 0.5, 1.0, 1.5 = 4 frames
    EXPECT_LE(recorder.GetFrameCount(), 5u);
    EXPECT_GE(recorder.GetFrameCount(), 3u);

    recorder.StopRecording();
}

TEST(ReplayRecorderTest, RecorderClear)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);

    recorder.Clear();
    EXPECT_FALSE(recorder.IsRecording());
    EXPECT_EQ(recorder.GetFrameCount(), 0u);
    EXPECT_FLOAT_EQ(recorder.GetDuration(), 0.0f);
}

TEST(ReplayRecorderTest, RecorderVariables)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    recorder.SetVariable("health", 100.0f);
    recorder.SetVariable("score", 42.0f);

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);

    const auto& frames = recorder.GetFrames();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_FLOAT_EQ(frames[0].variables.at("health"), 100.0f);
    EXPECT_FLOAT_EQ(frames[0].variables.at("score"), 42.0f);

    recorder.StopRecording();
}

// =============================================================================
// ReplayPlayer テスト
// =============================================================================

TEST(ReplayPlayerTest, PlayerConstruct)
{
    ReplayPlayer player;
    EXPECT_FALSE(player.HasReplay());
    EXPECT_FALSE(player.IsPlaying());
    EXPECT_FALSE(player.IsPaused());
}

TEST(ReplayPlayerTest, PlayerSetReplay)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording("Test");

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 1, 2, 3) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(1.0f, entities);
    recorder.StopRecording();

    ReplayPlayer player;
    player.SetReplay(recorder.GetFrames(), recorder.GetHeader());
    EXPECT_TRUE(player.HasReplay());
    EXPECT_FLOAT_EQ(player.GetDuration(), 1.0f);
}

TEST(ReplayPlayerTest, PlayerPlayPauseStop)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(1.0f, entities);
    recorder.StopRecording();

    ReplayPlayer player;
    player.SetReplay(recorder.GetFrames(), recorder.GetHeader());

    // Play
    player.Play();
    EXPECT_TRUE(player.IsPlaying());
    EXPECT_FALSE(player.IsPaused());

    // Pause
    player.Pause();
    EXPECT_FALSE(player.IsPlaying());
    EXPECT_TRUE(player.IsPaused());

    // Resume
    player.Play();
    EXPECT_TRUE(player.IsPlaying());
    EXPECT_FALSE(player.IsPaused());

    // Stop
    player.Stop();
    EXPECT_FALSE(player.IsPlaying());
    EXPECT_FALSE(player.IsPaused());
}

TEST(ReplayPlayerTest, PlayerSeek)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(0.5f, entities);
    recorder.RecordFrame(1.0f, entities);
    recorder.StopRecording();

    ReplayPlayer player;
    player.SetReplay(recorder.GetFrames(), recorder.GetHeader());

    player.Seek(0.5f);
    EXPECT_FLOAT_EQ(player.GetCurrentTime(), 0.5f);
    EXPECT_EQ(player.GetCurrentFrameIndex(), 1u);

    // Seek past end — clamped
    player.Seek(999.0f);
    EXPECT_FLOAT_EQ(player.GetCurrentTime(), 1.0f);
}

TEST(ReplayPlayerTest, PlayerUpdate)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(1.0f, entities);
    recorder.RecordFrame(2.0f, entities);
    recorder.StopRecording();

    ReplayPlayer player;
    player.SetReplay(recorder.GetFrames(), recorder.GetHeader());
    player.Play();

    // Update 0.5 seconds
    player.Update(0.5f);
    EXPECT_TRUE(player.IsPlaying());
    EXPECT_FLOAT_EQ(player.GetCurrentTime(), 0.5f);

    // Update past end
    player.Update(5.0f);
    EXPECT_FALSE(player.IsPlaying());
    EXPECT_TRUE(player.IsFinished());
}

TEST(ReplayPlayerTest, PlayerLooping)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(1.0f, entities);
    recorder.StopRecording();

    ReplayPlayer player;
    player.SetReplay(recorder.GetFrames(), recorder.GetHeader());
    player.SetLooping(true);
    EXPECT_TRUE(player.IsLooping());

    player.Play();

    // Update past end — should loop
    player.Update(1.5f);
    EXPECT_TRUE(player.IsPlaying());
    EXPECT_LT(player.GetCurrentTime(), 1.0f); // Should have wrapped
}

TEST(ReplayPlayerTest, PlayerPlaybackSpeed)
{
    ReplayRecorder recorder;
    recorder.SetRecordInterval(0.0f);
    recorder.StartRecording();

    std::vector<ReplayEntityState> entities = { MakeEntity(1, 0, 0, 0) };
    recorder.RecordFrame(0.0f, entities);
    recorder.RecordFrame(2.0f, entities);
    recorder.StopRecording();

    ReplayPlayer player;
    player.SetReplay(recorder.GetFrames(), recorder.GetHeader());
    player.SetPlaybackSpeed(2.0f);
    EXPECT_FLOAT_EQ(player.GetPlaybackSpeed(), 2.0f);

    player.Play();
    player.Update(0.5f); // 0.5 * 2.0 = 1.0 second of playback

    EXPECT_FLOAT_EQ(player.GetCurrentTime(), 1.0f);
}
