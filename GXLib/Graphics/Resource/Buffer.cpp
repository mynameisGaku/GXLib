/// @file Buffer.cpp
/// @brief GPUバッファの作成・管理の実装
#include "pch.h"
#include "Graphics/Resource/Buffer.h"
#include "Core/Logger.h"

namespace GX
{

bool Buffer::CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride)
{
    if (!CreateUploadBuffer(device, data, size))
        return false;

    // GPU側が参照するビュー情報を設定
    m_vbv.BufferLocation = m_resource->GetGPUVirtualAddress();
    m_vbv.SizeInBytes    = size;
    m_vbv.StrideInBytes  = stride;

    return true;
}

bool Buffer::CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size, DXGI_FORMAT format)
{
    if (!CreateUploadBuffer(device, data, size))
        return false;

    m_ibv.BufferLocation = m_resource->GetGPUVirtualAddress();
    m_ibv.SizeInBytes    = size;
    m_ibv.Format         = format;

    return true;
}

bool Buffer::CreateUploadBuffer(ID3D12Device* device, const void* data, uint32_t size)
{
    // UPLOADヒープはCPU/GPU両方からアクセス可能なメモリ領域
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width            = size;
    resourceDesc.Height           = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels        = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create upload buffer (HRESULT: 0x%08X)", hr);
        return false;
    }

    // Map→memcpy→Unmapの流れでCPUからGPUメモリへデータを転送
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // CPU側からの読み戻しは不要
    hr = m_resource->Map(0, &readRange, &mapped);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to map buffer (HRESULT: 0x%08X)", hr);
        return false;
    }

    memcpy(mapped, data, size);
    m_resource->Unmap(0, nullptr);

    return true;
}

bool Buffer::CreateDefaultBuffer(ID3D12Device* device, uint64_t size,
                                  D3D12_RESOURCE_FLAGS flags,
                                  D3D12_RESOURCE_STATES initialState)
{
    // DEFAULTヒープはGPU専用。CPUから直接書き込めないが高速
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width            = size;
    resourceDesc.Height           = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels        = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags            = flags;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create default buffer (HRESULT: 0x%08X)", hr);
        return false;
    }
    return true;
}

bool Buffer::CreateUploadBufferEmpty(ID3D12Device* device, uint64_t size)
{
    // データなしのUPLOADバッファ。Map/Unmapで後からデータを書き込む用途
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width            = size;
    resourceDesc.Height           = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels        = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create upload buffer (HRESULT: 0x%08X)", hr);
        return false;
    }
    return true;
}

void* Buffer::Map()
{
    if (!m_resource) return nullptr;
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    HRESULT hr = m_resource->Map(0, &readRange, &mapped);
    if (FAILED(hr)) return nullptr;
    return mapped;
}

void Buffer::Unmap()
{
    if (m_resource)
        m_resource->Unmap(0, nullptr);
}

} // namespace GX
