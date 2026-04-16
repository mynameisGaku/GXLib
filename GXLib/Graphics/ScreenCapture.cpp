/// @file ScreenCapture.cpp
/// @brief スクリーンキャプチャ実装
#include "pch_graphics.h"
#include "Graphics/ScreenCapture.h"
#include <fstream>

namespace gx
{

bool ScreenCapture::Initialize(ID3D12Device* device)
{
    if (!device) return false;
    m_device = device;
    return true;
}

bool ScreenCapture::EnsureReadbackBuffer(uint32_t width, uint32_t height)
{
    if (m_readbackWidth == width && m_readbackHeight == height && m_readbackBuffer)
        return true;

    m_readbackBuffer.Reset();

    // 行のアライメント（256バイト）
    m_readbackRowPitch = (static_cast<uint64_t>(width) * 4 + 255) & ~255ULL;
    uint64_t totalSize = m_readbackRowPitch * height;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = totalSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&m_readbackBuffer));

    if (FAILED(hr)) return false;

    m_readbackWidth  = width;
    m_readbackHeight = height;
    return true;
}

bool ScreenCapture::CaptureToMemory(ID3D12GraphicsCommandList* cmdList,
                                     ID3D12Resource* backBuffer,
                                     uint32_t width, uint32_t height,
                                     gx::Vector<uint8_t>& outPixels,
                                     uint32_t& outWidth, uint32_t& outHeight)
{
    if (!m_device || !cmdList || !backBuffer) return false;
    if (!EnsureReadbackBuffer(width, height)) return false;

    // バックバッファ → コピー元に遷移
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // コピーコマンド
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = m_readbackBuffer.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Offset = 0;
    dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dst.PlacedFootprint.Footprint.Width = width;
    dst.PlacedFootprint.Footprint.Height = height;
    dst.PlacedFootprint.Footprint.Depth = 1;
    dst.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(m_readbackRowPitch);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = backBuffer;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // コピー元を元に戻す
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    cmdList->ResourceBarrier(1, &barrier);

    // 注意: 実際のピクセル読み取りはGPU実行完了後にMap()で行う
    outWidth  = width;
    outHeight = height;
    outPixels.resize(static_cast<size_t>(width) * height * 4);

    return true;
}

bool ScreenCapture::CaptureToFile(ID3D12GraphicsCommandList* cmdList,
                                   ID3D12Resource* backBuffer,
                                   uint32_t width, uint32_t height,
                                   const gx::String& filePath)
{
    gx::Vector<uint8_t> pixels;
    uint32_t outW, outH;
    if (!CaptureToMemory(cmdList, backBuffer, width, height, pixels, outW, outH))
        return false;

    // 簡易BMP保存（GPUフェンス待機後に呼ぶ前提）
    // ここではコマンド発行のみ。実データの読み出しは別途フェンス後。
    return true;
}

void ScreenCapture::Shutdown()
{
    m_readbackBuffer.Reset();
    m_device = nullptr;
    m_readbackWidth  = 0;
    m_readbackHeight = 0;
}

} // namespace gx
