/// @file test_VideoRecording.cpp
/// @brief Phase 5: VideoRecorderの設定、状態、列挙型テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/VideoRecorder.h"

using namespace gx;

// ============================================================================
// VideoCodec列挙型
// ============================================================================

TEST(VideoRecordingTest, CodecEnumValues)
{
    EXPECT_NE(static_cast<uint32_t>(VideoCodec::H264), static_cast<uint32_t>(VideoCodec::HEVC));
}

// ============================================================================
// RecordingState列挙型
// ============================================================================

TEST(VideoRecordingTest, RecordingStateEnumValues)
{
    EXPECT_NE(static_cast<uint32_t>(RecordingState::Idle), static_cast<uint32_t>(RecordingState::Recording));
    EXPECT_NE(static_cast<uint32_t>(RecordingState::Recording), static_cast<uint32_t>(RecordingState::Paused));
    EXPECT_NE(static_cast<uint32_t>(RecordingState::Idle), static_cast<uint32_t>(RecordingState::Paused));
}

// ============================================================================
// VideoRecorderConfigデフォルト値
// ============================================================================

TEST(VideoRecordingTest, ConfigDefaults)
{
    VideoRecorderConfig config;
    EXPECT_EQ(config.outputPath, L"recording.mp4");
    EXPECT_EQ(config.width, 1920u);
    EXPECT_EQ(config.height, 1080u);
    EXPECT_EQ(config.fps, 30u);
    EXPECT_EQ(config.bitrateMbps, 20u);
    EXPECT_EQ(config.codec, VideoCodec::H264);
}

TEST(VideoRecordingTest, ConfigCustom)
{
    VideoRecorderConfig config;
    config.outputPath = L"custom_output.mp4";
    config.width = 3840;
    config.height = 2160;
    config.fps = 60;
    config.bitrateMbps = 50;
    config.codec = VideoCodec::HEVC;

    EXPECT_EQ(config.outputPath, L"custom_output.mp4");
    EXPECT_EQ(config.width, 3840u);
    EXPECT_EQ(config.height, 2160u);
    EXPECT_EQ(config.fps, 60u);
    EXPECT_EQ(config.bitrateMbps, 50u);
    EXPECT_EQ(config.codec, VideoCodec::HEVC);
}

// ============================================================================
// VideoRecorder初期状態
// ============================================================================

TEST(VideoRecordingTest, InitialState)
{
    VideoRecorder recorder;
    EXPECT_EQ(recorder.GetState(), RecordingState::Idle);
    EXPECT_FALSE(recorder.IsRecording());
    EXPECT_EQ(recorder.GetFramesCaptured(), 0u);
}

TEST(VideoRecordingTest, InitializeWithNullDevice)
{
    VideoRecorder recorder;
    // nullptrデバイスでの初期化は安全に失敗するはず
    bool result = recorder.Initialize(nullptr);
    // 実装により成功/失敗するが、クラッシュしないこと
    EXPECT_EQ(recorder.GetState(), RecordingState::Idle);
}

TEST(VideoRecordingTest, StartRecordingWithoutInit)
{
    VideoRecorder recorder;
    VideoRecorderConfig config;
    // 初期化なしの開始は失敗するはず
    bool result = recorder.StartRecording(config);
    EXPECT_FALSE(result);
}

TEST(VideoRecordingTest, StopRecordingWhenIdle)
{
    VideoRecorder recorder;
    // 録画していない時の停止はクラッシュしないこと
    bool result = recorder.StopRecording();
    // 停止するものがないためfalseを返す可能性がある
    EXPECT_EQ(recorder.GetState(), RecordingState::Idle);
}

TEST(VideoRecordingTest, SetOnErrorCallback)
{
    VideoRecorder recorder;
    bool errorCallbackCalled = false;
    recorder.SetOnError([&](const gx::String& msg) {
        errorCallbackCalled = true;
    });
    // コールバックがクラッシュなしで設定できることを検証
    EXPECT_FALSE(errorCallbackCalled);
}

TEST(VideoRecordingTest, GetOutputPathDefault)
{
    VideoRecorder recorder;
    // 録画前は出力パスがconfigのデフォルト値であるはず
    EXPECT_EQ(recorder.GetOutputPath(), L"recording.mp4");
}

// ============================================================================
// 追加テスト: VideoRecorderConfig
// ============================================================================

TEST(VideoRecordingTest, ConfigWidthHeight720p)
{
    VideoRecorderConfig config;
    config.width = 1280;
    config.height = 720;
    EXPECT_EQ(config.width, 1280u);
    EXPECT_EQ(config.height, 720u);
}

TEST(VideoRecordingTest, ConfigFps60)
{
    VideoRecorderConfig config;
    config.fps = 60;
    EXPECT_EQ(config.fps, 60u);
}

TEST(VideoRecordingTest, ConfigHighBitrate)
{
    VideoRecorderConfig config;
    config.bitrateMbps = 100;
    EXPECT_EQ(config.bitrateMbps, 100u);
}

TEST(VideoRecordingTest, ConfigHEVCCodec)
{
    VideoRecorderConfig config;
    config.codec = VideoCodec::HEVC;
    EXPECT_EQ(config.codec, VideoCodec::HEVC);
}

TEST(VideoRecordingTest, ConfigOutputPathCustom)
{
    VideoRecorderConfig config;
    config.outputPath = L"C:/Videos/my_recording.mp4";
    EXPECT_EQ(config.outputPath, L"C:/Videos/my_recording.mp4");
}

// ============================================================================
// 追加テスト: VideoRecorder状態遷移
// ============================================================================

TEST(VideoRecordingTest, IsRecordingFalseByDefault)
{
    VideoRecorder recorder;
    EXPECT_FALSE(recorder.IsRecording());
}

TEST(VideoRecordingTest, GetFramesCapturedDefault)
{
    VideoRecorder recorder;
    EXPECT_EQ(recorder.GetFramesCaptured(), 0u);
}

TEST(VideoRecordingTest, GetRecordingDurationDefault)
{
    VideoRecorder recorder;
    float duration = recorder.GetRecordingDuration();
    EXPECT_FLOAT_EQ(duration, 0.0f);
}

TEST(VideoRecordingTest, PauseRecordingWhenIdle)
{
    VideoRecorder recorder;
    // 録画していない時のPauseはクラッシュしないこと
    recorder.PauseRecording();
    EXPECT_EQ(recorder.GetState(), RecordingState::Idle);
}

TEST(VideoRecordingTest, ResumeRecordingWhenIdle)
{
    VideoRecorder recorder;
    // 録画していない時のResumeはクラッシュしないこと
    recorder.ResumeRecording();
    EXPECT_EQ(recorder.GetState(), RecordingState::Idle);
}

TEST(VideoRecordingTest, CaptureFrameWithoutInit)
{
    VideoRecorder recorder;
    // 初期化前のCaptureFrameはクラッシュしないこと
    bool result = recorder.CaptureFrame(nullptr, nullptr, 1920, 1080);
    EXPECT_FALSE(result);
}

TEST(VideoRecordingTest, WriteFrameWithoutInit)
{
    VideoRecorder recorder;
    // 初期化前のWriteFrameはクラッシュしないこと
    gx::Vector<uint8_t> dummyData(1920 * 1080 * 4, 0);
    bool result = recorder.WriteFrame(dummyData.data(), 1920, 1080);
    EXPECT_FALSE(result);
}

TEST(VideoRecordingTest, MultipleSetOnError)
{
    VideoRecorder recorder;
    int callCount = 0;

    recorder.SetOnError([&](const gx::String& msg) { callCount++; });
    // 再度設定しても問題ないこと
    recorder.SetOnError([&](const gx::String& msg) { callCount += 10; });

    EXPECT_EQ(callCount, 0);
}

TEST(VideoRecordingTest, StopRecordingReturnValue)
{
    VideoRecorder recorder;
    // 録画していない時の停止
    bool result = recorder.StopRecording();
    // 停止するものがないためfalseを返す可能性がある
    (void)result;
    EXPECT_EQ(recorder.GetState(), RecordingState::Idle);
}

TEST(VideoRecordingTest, ConfigDefaultCodecIsH264)
{
    VideoRecorderConfig config;
    EXPECT_EQ(config.codec, VideoCodec::H264);
}
