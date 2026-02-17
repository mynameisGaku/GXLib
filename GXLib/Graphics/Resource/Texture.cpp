/// @file Texture.cpp
/// @brief テクスチャリソース管理の実装

// stb_image の実装をここで定義
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

#include "pch.h"
#include "Graphics/Resource/Texture.h"
#include "Core/Logger.h"

namespace GX
{

bool Texture::LoadFromFile(ID3D12Device* device,
                           ID3D12CommandQueue* cmdQueue,
                           const std::wstring& filePath,
                           DescriptorHeap* srvHeap,
                           uint32_t srvIndex)
{
    // wstring → string 変換（stb_imageはchar*パスのみ対応）
    int pathLen = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string pathUtf8(pathLen - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, pathUtf8.data(), pathLen, nullptr, nullptr);

    // stb_imageで画像を読み込み（4チャンネルRGBA強制）
    int width, height, channels;
    unsigned char* pixels = stbi_load(pathUtf8.c_str(), &width, &height, &channels, 4);
    if (!pixels)
    {
        GX_LOG_ERROR("Failed to load image: %s (stb_image: %s)", pathUtf8.c_str(), stbi_failure_reason());
        return false;
    }

    bool result = UploadToGPU(device, cmdQueue, pixels,
                               static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                               srvHeap, srvIndex);

    stbi_image_free(pixels);

    if (result)
    {
        GX_LOG_INFO("Texture loaded: %s (%dx%d)", pathUtf8.c_str(), m_width, m_height);
    }

    return result;
}

bool Texture::CreateFromMemory(ID3D12Device* device,
                                ID3D12CommandQueue* cmdQueue,
                                const void* pixels,
                                uint32_t width, uint32_t height,
                                DescriptorHeap* srvHeap,
                                uint32_t srvIndex)
{
    return UploadToGPU(device, cmdQueue, pixels, width, height, srvHeap, srvIndex);
}

bool Texture::UploadToGPU(ID3D12Device* device,
                           ID3D12CommandQueue* cmdQueue,
                           const void* pixels,
                           uint32_t width, uint32_t height,
                           DescriptorHeap* srvHeap,
                           uint32_t srvIndex)
{
    m_width  = width;
    m_height = height;
    m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_srvIndex = srvIndex;

    // DEFAULTヒープにテクスチャリソースを作成
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width            = width;
    texDesc.Height           = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels        = 1;
    texDesc.Format           = m_format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create texture resource (HRESULT: 0x%08X)", hr);
        return false;
    }

    // UPLOADヒープにステージングバッファを作成
    // テクスチャのアップロード行ピッチは256バイトアライメントが必要
    const uint32_t bytesPerPixel = 4;
    const uint32_t rowPitch = (width * bytesPerPixel + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
                              & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    const uint64_t uploadSize = static_cast<uint64_t>(rowPitch) * height;

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width            = uploadSize;
    uploadDesc.Height           = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels        = 1;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create upload buffer for texture (HRESULT: 0x%08X)", hr);
        return false;
    }

    // ステージングバッファにピクセルデータを書き込み
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    hr = uploadBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to map upload buffer (HRESULT: 0x%08X)", hr);
        return false;
    }

    const uint8_t* srcData = static_cast<const uint8_t*>(pixels);
    uint8_t* dstData = static_cast<uint8_t*>(mapped);
    for (uint32_t y = 0; y < height; ++y)
    {
        memcpy(dstData + static_cast<uint64_t>(y) * rowPitch, srcData + static_cast<uint64_t>(y) * width * bytesPerPixel, width * bytesPerPixel);
    }
    uploadBuffer->Unmap(0, nullptr);

    // コマンドアロケータとコマンドリストを作成してコピーコマンドを記録
    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create command allocator for texture upload (HRESULT: 0x%08X)", hr);
        return false;
    }

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create command list for texture upload (HRESULT: 0x%08X)", hr);
        return false;
    }

    // CopyTextureRegionでUPLOAD→DEFAULTにコピー
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource        = m_resource.Get();
    dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource                          = uploadBuffer.Get();
    src.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset             = 0;
    src.PlacedFootprint.Footprint.Format   = m_format;
    src.PlacedFootprint.Footprint.Width    = width;
    src.PlacedFootprint.Footprint.Height   = height;
    src.PlacedFootprint.Footprint.Depth    = 1;
    src.PlacedFootprint.Footprint.RowPitch = rowPitch;

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // リソースバリア: COPY_DEST → PIXEL_SHADER_RESOURCE
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->Close();

    // コマンド実行
    ID3D12CommandList* lists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);

    // フェンスで完了待ち
    ComPtr<ID3D12Fence> fence;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create fence for texture upload (HRESULT: 0x%08X)", hr);
        return false;
    }

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event)
    {
        GX_LOG_ERROR("Texture: Failed to create event");
        return false;
    }
    cmdQueue->Signal(fence.Get(), 1);
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    // SRV（Shader Resource View）を作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                        = m_format;
    srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels          = 1;
    srvDesc.Texture2D.MostDetailedMip    = 0;

    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, srvHeap->GetCPUHandle(srvIndex));
    m_srvGPUHandle = srvHeap->GetGPUHandle(srvIndex);

    return true;
}

bool Texture::UpdatePixels(ID3D12Device* device,
                            ID3D12CommandQueue* cmdQueue,
                            const void* pixels,
                            uint32_t width, uint32_t height)
{
    if (!m_resource || width != m_width || height != m_height)
    {
        GX_LOG_ERROR("UpdatePixels: resource null or size mismatch (%ux%u vs %ux%u)",
                     width, height, m_width, m_height);
        return false;
    }

    const uint32_t bytesPerPixel = 4;
    const uint32_t rowPitch = (width * bytesPerPixel + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
                              & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    const uint64_t uploadSize = static_cast<uint64_t>(rowPitch) * height;

    // UPLOADヒープにステージングバッファを作成
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width            = uploadSize;
    uploadDesc.Height           = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels        = 1;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("UpdatePixels: Failed to create upload buffer (0x%08X)", hr);
        return false;
    }

    // ステージングバッファにピクセルデータを書き込み
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    hr = uploadBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("UpdatePixels: Failed to map upload buffer (0x%08X)", hr);
        return false;
    }

    const uint8_t* srcData = static_cast<const uint8_t*>(pixels);
    uint8_t* dstData = static_cast<uint8_t*>(mapped);
    for (uint32_t y = 0; y < height; ++y)
    {
        memcpy(dstData + static_cast<uint64_t>(y) * rowPitch, srcData + static_cast<uint64_t>(y) * width * bytesPerPixel, width * bytesPerPixel);
    }
    uploadBuffer->Unmap(0, nullptr);

    // コマンドアロケータとコマンドリストを作成
    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("UpdatePixels: Failed to create command allocator (0x%08X)", hr);
        return false;
    }

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("UpdatePixels: Failed to create command list (0x%08X)", hr);
        return false;
    }

    // PIXEL_SHADER_RESOURCE → COPY_DEST
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // CopyTextureRegionでUPLOAD→DEFAULTにコピー
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource        = m_resource.Get();
    dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource                          = uploadBuffer.Get();
    src.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset             = 0;
    src.PlacedFootprint.Footprint.Format   = m_format;
    src.PlacedFootprint.Footprint.Width    = width;
    src.PlacedFootprint.Footprint.Height   = height;
    src.PlacedFootprint.Footprint.Depth    = 1;
    src.PlacedFootprint.Footprint.RowPitch = rowPitch;

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // COPY_DEST → PIXEL_SHADER_RESOURCE
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->Close();

    // コマンド実行
    ID3D12CommandList* lists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);

    // フェンスで完了待ち
    ComPtr<ID3D12Fence> fence;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("UpdatePixels: Failed to create fence (0x%08X)", hr);
        return false;
    }

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event)
    {
        GX_LOG_ERROR("Texture: Failed to create event");
        return false;
    }
    cmdQueue->Signal(fence.Get(), 1);
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    return true;
}

} // namespace GX
