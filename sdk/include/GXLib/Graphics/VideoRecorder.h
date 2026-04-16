#pragma once
/// @file VideoRecorder.h
/// @brief ゲーム画面の動画録画（Media Foundation + H.264/HEVC）
///
/// バックバッファのフレームデータを Media Foundation の SinkWriter に渡し、
/// H.264 または HEVC でエンコードして MP4 ファイルに書き出す。
/// StartRecording() → CaptureFrame()（毎フレーム）→ StopRecording() の手順で使う。
/// @addtogroup grp_graphics/// @{
#include "pch_graphics.h"
#include <functional>

namespace gx
{

enum class VideoCodec : uint32_t
{
    H264,
    HEVC,
};

enum class RecordingState : uint32_t
{
    Idle,
    Recording,
    Paused,
};

struct VideoRecorderConfig
{
    gx::WString outputPath = L"recording.mp4";
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 30;
    uint32_t bitrateMbps = 20;
    VideoCodec codec = VideoCodec::H264;
};

class VideoRecorder
{
public:
    VideoRecorder();
    ~VideoRecorder();

    bool Initialize(ID3D12Device* device);
    bool StartRecording(const VideoRecorderConfig& config);
    bool CaptureFrame(ID3D12GraphicsCommandList* cmdList,
                       ID3D12Resource* backBuffer,
                       uint32_t width, uint32_t height);
    void PauseRecording();
    void ResumeRecording();
    bool StopRecording();

    RecordingState GetState() const { return m_state; }
    bool IsRecording() const { return m_state == RecordingState::Recording; }
    uint32_t GetFramesCaptured() const { return m_framesCaptured; }
    float GetRecordingDuration() const;
    const gx::WString& GetOutputPath() const { return m_config.outputPath; }

    bool WriteFrame(const uint8_t* rgbaData, uint32_t width, uint32_t height);

    void SetOnError(std::function<void(const gx::String&)> callback)
    {
        m_onError = std::move(callback);
    }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    ID3D12Device* m_device = nullptr;
    RecordingState m_state = RecordingState::Idle;
    VideoRecorderConfig m_config;
    uint32_t m_framesCaptured = 0;
    bool m_mfInitialized = false;

    // リードバックリソース（ダブルバッファ）
    ComPtr<ID3D12Resource> m_readbackBuffer[2];
    uint32_t m_readbackIndex = 0;
    uint32_t m_readbackRowPitch = 0;

    std::function<void(const gx::String&)> m_onError;

    bool InitMediaFoundation();
    void ShutdownMediaFoundation();
    bool CreateReadbackBuffers(uint32_t width, uint32_t height);
};

} // namespace gx
/// @}
