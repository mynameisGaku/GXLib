/// @file DynamicBuffer.cpp
/// @brief ダイナミックバッファの実装
#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Core/Logger.h"

namespace GX
{

bool DynamicBuffer::Initialize(ID3D12Device* device, uint32_t maxSize, uint32_t stride)
{
    m_maxSize = maxSize;
    m_stride  = stride;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width            = maxSize;
    resourceDesc.Height           = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels        = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    for (uint32_t i = 0; i < k_BufferCount; ++i)
    {
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_buffers[i])
        );

        if (FAILED(hr))
        {
            GX_LOG_ERROR("Failed to create dynamic buffer %d (HRESULT: 0x%08X)", i, hr);
            return false;
        }
    }

    GX_LOG_INFO("DynamicBuffer created (size: %u, stride: %u)", maxSize, stride);
    return true;
}

void* DynamicBuffer::Map(uint32_t frameIndex)
{
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // CPU側からの読み込みは不要
    HRESULT hr = m_buffers[frameIndex]->Map(0, &readRange, &mapped);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to map dynamic buffer (HRESULT: 0x%08X)", hr);
        return nullptr;
    }
    return mapped;
}

void DynamicBuffer::Unmap(uint32_t frameIndex)
{
    m_buffers[frameIndex]->Unmap(0, nullptr);
}

D3D12_VERTEX_BUFFER_VIEW DynamicBuffer::GetVertexBufferView(uint32_t frameIndex, uint32_t usedSize) const
{
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = m_buffers[frameIndex]->GetGPUVirtualAddress();
    vbv.SizeInBytes    = usedSize;
    vbv.StrideInBytes  = m_stride;
    return vbv;
}

D3D12_INDEX_BUFFER_VIEW DynamicBuffer::GetIndexBufferView(uint32_t frameIndex, uint32_t usedSize,
                                                            DXGI_FORMAT format) const
{
    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = m_buffers[frameIndex]->GetGPUVirtualAddress();
    ibv.SizeInBytes    = usedSize;
    ibv.Format         = format;
    return ibv;
}

D3D12_GPU_VIRTUAL_ADDRESS DynamicBuffer::GetGPUVirtualAddress(uint32_t frameIndex) const
{
    return m_buffers[frameIndex]->GetGPUVirtualAddress();
}

} // namespace GX
