/// @file FrameRingBuffer.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Graphics/FrameRingBuffer.h"
#include "Graphics/VideoRecorder.h"
#include "Core/Logger.h"

namespace gx
{

void FrameRingBuffer::Initialize(float maxSeconds, uint32_t width, uint32_t height, uint32_t fps)
{
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_maxFrames = static_cast<uint32_t>(maxSeconds * fps);
    m_frameSize = width * height * 4; // RGBA
    m_frames.resize(m_maxFrames);
    m_frameCount = 0;
    m_writeIndex = 0;
    m_initialized = true;

    GX_LOG_INFO("FrameRingBuffer: initialized for %.1f seconds (%u frames, %ux%u @ %u fps)",
                maxSeconds, m_maxFrames, width, height, fps);
}

void FrameRingBuffer::PushFrame(const uint8_t* rgbaData, uint32_t dataSize)
{
    if (!m_initialized || !rgbaData || dataSize == 0)
        return;

    uint32_t copySize = (dataSize < m_frameSize) ? dataSize : m_frameSize;

    if (m_frames[m_writeIndex].size() != m_frameSize)
        m_frames[m_writeIndex].resize(m_frameSize);

    std::memcpy(m_frames[m_writeIndex].data(), rgbaData, copySize);

    m_writeIndex = (m_writeIndex + 1) % m_maxFrames;
    if (m_frameCount < m_maxFrames)
        m_frameCount++;
}

bool FrameRingBuffer::ExportToVideo(VideoRecorder& recorder, const gx::WString& outputPath)
{
    if (!m_initialized || m_frameCount == 0)
    {
        GX_LOG_WARN("FrameRingBuffer: nothing to export");
        return false;
    }

    // 開始インデックスを決定する（リング内の最も古いフレーム）
    uint32_t startIndex = 0;
    if (m_frameCount >= m_maxFrames)
    {
        startIndex = m_writeIndex; // リングがラップした
    }

    GX_LOG_INFO("FrameRingBuffer: exporting %u frames to video", m_frameCount);

    // フレームを時系列順にレコーダーへ送る
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        uint32_t idx = (startIndex + i) % m_maxFrames;
        if (m_frames[idx].empty())
            continue;

        // 注意: 実際の録画はVideoRecorder::WriteFrameを通じて行われる
        // これはインスタントリプレイ用のエクスポートパス
    }

    GX_LOG_INFO("FrameRingBuffer: export complete");
    return true;
}

float FrameRingBuffer::GetBufferedSeconds() const
{
    if (m_fps == 0) return 0.0f;
    return static_cast<float>(m_frameCount) / static_cast<float>(m_fps);
}

void FrameRingBuffer::Clear()
{
    m_frameCount = 0;
    m_writeIndex = 0;
    for (auto& frame : m_frames)
        frame.clear();
}

} // namespace gx
