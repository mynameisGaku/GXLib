/// @file RenderTarget.cpp
/// @brief オフスクリーンレンダーターゲットの実装
#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Core/Logger.h"

namespace GX
{

bool RenderTarget::Create(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    m_width  = width;
    m_height = height;
    m_format = format;
    m_currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // レンダーターゲット用リソースを作成
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width            = width;
    texDesc.Height           = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels        = 1;
    texDesc.Format           = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = format;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 0.0f;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create render target resource (HRESULT: 0x%08X)", hr);
        return false;
    }

    // RTV用ディスクリプタヒープ
    if (!m_rtvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false))
        return false;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format        = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    device->CreateRenderTargetView(m_resource.Get(), &rtvDesc, m_rtvHeap.GetCPUHandle(0));

    // SRV用ディスクリプタヒープ（shader-visible）
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true))
        return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = format;
    srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels      = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvHeap.GetCPUHandle(0));

    GX_LOG_INFO("RenderTarget created (%dx%d, format=%d)", width, height, format);
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRTVHandle() const
{
    return m_rtvHeap.GetCPUHandle(0);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTarget::GetSRVGPUHandle() const
{
    return m_srvHeap.GetGPUHandle(0);
}

void RenderTarget::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
{
    if (m_currentState == newState)
        return;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_resource.Get();
    barrier.Transition.StateBefore = m_currentState;
    barrier.Transition.StateAfter  = newState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    m_currentState = newState;
}

} // namespace GX
