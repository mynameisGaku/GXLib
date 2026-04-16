/// @file VideoRecorder.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Graphics/VideoRecorder.h"
#include "Core/Logger.h"

// Media Foundationヘッダー
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace gx
{

struct VideoRecorder::Impl
{
    ComPtr<IMFSinkWriter> sinkWriter;
    DWORD videoStreamIndex = 0;
    uint64_t frameDuration = 0;
    uint64_t timestamp = 0;
};

VideoRecorder::VideoRecorder()
    : m_impl(std::make_unique<Impl>())
{
}

VideoRecorder::~VideoRecorder()
{
    if (m_state != RecordingState::Idle)
    {
        StopRecording();
    }
    ShutdownMediaFoundation();
}

bool VideoRecorder::Initialize(ID3D12Device* device)
{
    m_device = device;

    if (!InitMediaFoundation())
    {
        GX_LOG_WARN("VideoRecorder: Media Foundation initialization failed — recording disabled");
        return false;
    }

    GX_LOG_INFO("VideoRecorder: initialized successfully");
    return true;
}

bool VideoRecorder::InitMediaFoundation()
{
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("VideoRecorder: MFStartup failed (HRESULT: 0x%08X)", hr);
        return false;
    }
    m_mfInitialized = true;
    return true;
}

void VideoRecorder::ShutdownMediaFoundation()
{
    if (m_mfInitialized)
    {
        MFShutdown();
        m_mfInitialized = false;
    }
}

bool VideoRecorder::StartRecording(const VideoRecorderConfig& config)
{
    if (m_state != RecordingState::Idle)
    {
        GX_LOG_WARN("VideoRecorder: already recording");
        return false;
    }

    if (!m_mfInitialized)
    {
        GX_LOG_ERROR("VideoRecorder: Media Foundation not initialized");
        return false;
    }

    m_config = config;
    m_framesCaptured = 0;
    m_impl->timestamp = 0;

    // フレーム間隔を100ナノ秒単位で計算する
    m_impl->frameDuration = 10'000'000ULL / config.fps;

    // シンクライターを作成する
    ComPtr<IMFAttributes> attrs;
    HRESULT hr = MFCreateAttributes(&attrs, 1);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("VideoRecorder: MFCreateAttributes failed");
        return false;
    }

    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    hr = MFCreateSinkWriterFromURL(config.outputPath.c_str(), nullptr, attrs.Get(),
                                     &m_impl->sinkWriter);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("VideoRecorder: MFCreateSinkWriterFromURL failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    // 出力メディアタイプを設定する（H.264）
    ComPtr<IMFMediaType> outputType;
    MFCreateMediaType(&outputType);
    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

    if (config.codec == VideoCodec::HEVC)
        outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_HEVC);
    else
        outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);

    outputType->SetUINT32(MF_MT_AVG_BITRATE, config.bitrateMbps * 1'000'000);
    outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, config.width, config.height);
    MFSetAttributeRatio(outputType.Get(), MF_MT_FRAME_RATE, config.fps, 1);
    MFSetAttributeRatio(outputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = m_impl->sinkWriter->AddStream(outputType.Get(), &m_impl->videoStreamIndex);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("VideoRecorder: AddStream failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    // 入力メディアタイプを設定する（RGB32）
    ComPtr<IMFMediaType> inputType;
    MFCreateMediaType(&inputType);
    inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, config.width, config.height);
    MFSetAttributeRatio(inputType.Get(), MF_MT_FRAME_RATE, config.fps, 1);
    MFSetAttributeRatio(inputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = m_impl->sinkWriter->SetInputMediaType(m_impl->videoStreamIndex, inputType.Get(), nullptr);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("VideoRecorder: SetInputMediaType failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    hr = m_impl->sinkWriter->BeginWriting();
    if (FAILED(hr))
    {
        GX_LOG_ERROR("VideoRecorder: BeginWriting failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    // リードバックバッファを作成する
    if (m_device)
    {
        CreateReadbackBuffers(config.width, config.height);
    }

    m_state = RecordingState::Recording;
    GX_LOG_INFO("VideoRecorder: recording started (%ux%u @ %u fps, %u Mbps)",
                config.width, config.height, config.fps, config.bitrateMbps);
    return true;
}

bool VideoRecorder::CaptureFrame(ID3D12GraphicsCommandList* cmdList,
                                   ID3D12Resource* backBuffer,
                                   uint32_t width, uint32_t height)
{
    if (m_state != RecordingState::Recording)
        return false;

    if (!cmdList || !backBuffer)
        return false;

    if (!m_readbackBuffer[m_readbackIndex])
        return false;

    // バックバッファをコピーソースに遷移させる
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // リードバックバッファにコピーする
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = backBuffer;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = m_readbackBuffer[m_readbackIndex].Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Offset = 0;
    dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dst.PlacedFootprint.Footprint.Width = width;
    dst.PlacedFootprint.Footprint.Height = height;
    dst.PlacedFootprint.Footprint.Depth = 1;
    dst.PlacedFootprint.Footprint.RowPitch = m_readbackRowPitch;

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // 元のステートに戻す
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cmdList->ResourceBarrier(1, &barrier);

    // 前フレームのデータをリードバックする（1フレーム遅れ）
    uint32_t prevIndex = (m_readbackIndex + 1) % 2;
    if (m_framesCaptured > 0 && m_readbackBuffer[prevIndex])
    {
        void* mappedData = nullptr;
        D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(m_readbackRowPitch * height) };
        HRESULT hr = m_readbackBuffer[prevIndex]->Map(0, &readRange, &mappedData);
        if (SUCCEEDED(hr))
        {
            WriteFrame(static_cast<const uint8_t*>(mappedData), width, height);
            D3D12_RANGE writeRange = { 0, 0 };
            m_readbackBuffer[prevIndex]->Unmap(0, &writeRange);
        }
    }

    m_readbackIndex = (m_readbackIndex + 1) % 2;
    m_framesCaptured++;
    return true;
}

bool VideoRecorder::WriteFrame(const uint8_t* rgbaData, uint32_t width, uint32_t height)
{
    if (!m_impl->sinkWriter || !rgbaData)
        return false;

    uint32_t stride = width * 4;
    uint32_t bufferSize = stride * height;

    ComPtr<IMFMediaBuffer> buffer;
    HRESULT hr = MFCreateMemoryBuffer(bufferSize, &buffer);
    if (FAILED(hr)) return false;

    BYTE* bufferData = nullptr;
    hr = buffer->Lock(&bufferData, nullptr, nullptr);
    if (FAILED(hr)) return false;

    // 行をコピーする（MF向けに垂直反転が必要な場合がある）
    for (uint32_t y = 0; y < height; ++y)
    {
        const uint8_t* srcRow = rgbaData + y * m_readbackRowPitch;
        BYTE* dstRow = bufferData + y * stride;
        std::memcpy(dstRow, srcRow, stride);
    }

    buffer->Unlock();
    buffer->SetCurrentLength(bufferSize);

    ComPtr<IMFSample> sample;
    MFCreateSample(&sample);
    sample->AddBuffer(buffer.Get());
    sample->SetSampleTime(static_cast<LONGLONG>(m_impl->timestamp));
    sample->SetSampleDuration(static_cast<LONGLONG>(m_impl->frameDuration));

    hr = m_impl->sinkWriter->WriteSample(m_impl->videoStreamIndex, sample.Get());
    m_impl->timestamp += m_impl->frameDuration;

    return SUCCEEDED(hr);
}

void VideoRecorder::PauseRecording()
{
    if (m_state == RecordingState::Recording)
    {
        m_state = RecordingState::Paused;
        GX_LOG_INFO("VideoRecorder: paused");
    }
}

void VideoRecorder::ResumeRecording()
{
    if (m_state == RecordingState::Paused)
    {
        m_state = RecordingState::Recording;
        GX_LOG_INFO("VideoRecorder: resumed");
    }
}

bool VideoRecorder::StopRecording()
{
    if (m_state == RecordingState::Idle)
        return false;

    if (m_impl->sinkWriter)
    {
        m_impl->sinkWriter->Finalize();
        m_impl->sinkWriter.Reset();
    }

    m_readbackBuffer[0].Reset();
    m_readbackBuffer[1].Reset();

    m_state = RecordingState::Idle;
    GX_LOG_INFO("VideoRecorder: stopped (%u frames captured)", m_framesCaptured);
    return true;
}

float VideoRecorder::GetRecordingDuration() const
{
    if (m_config.fps == 0) return 0.0f;
    return static_cast<float>(m_framesCaptured) / static_cast<float>(m_config.fps);
}

bool VideoRecorder::CreateReadbackBuffers(uint32_t width, uint32_t height)
{
    if (!m_device) return false;

    // 行ピッチを計算する（D3D12_TEXTURE_DATA_PITCH_ALIGNMENT = 256にアラインメント）
    m_readbackRowPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
                          & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);

    uint64_t bufferSize = static_cast<uint64_t>(m_readbackRowPitch) * height;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = bufferSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    for (int i = 0; i < 2; ++i)
    {
        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_readbackBuffer[i]));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("VideoRecorder: Failed to create readback buffer %d", i);
            return false;
        }
    }

    GX_LOG_INFO("VideoRecorder: readback buffers created (%ux%u, pitch=%u)", width, height, m_readbackRowPitch);
    return true;
}

} // namespace gx
