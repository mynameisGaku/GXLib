/// @file DepthBuffer.cpp
/// @brief 深度バッファの実装
#include "pch.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Core/Logger.h"

namespace GX
{

bool DepthBuffer::Create(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    m_width  = width;
    m_height = height;
    m_format = format;

    // ALLOW_DEPTH_STENCILフラグで深度書き込み可能なリソースを作成
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width            = width;
    depthDesc.Height           = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels        = 1;
    depthDesc.Format           = format;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // clearValueに1.0fを設定（遠方が1.0、手前が0.0）
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format               = format;
    clearValue.DepthStencil.Depth   = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create depth buffer (HRESULT: 0x%08X)", hr);
        return false;
    }

    // DSV用ヒープを作成（non-shader-visible）
    if (!m_dsvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false))
        return false;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format        = format;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsvHeap.GetCPUHandle(0));

    GX_LOG_INFO("DepthBuffer created (%dx%d)", width, height);
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSVHandle() const
{
    return m_dsvHeap.GetCPUHandle(0);
}

bool DepthBuffer::CreateWithSRV(ID3D12Device* device, uint32_t width, uint32_t height,
                                  DescriptorHeap* srvHeap, uint32_t srvIndex)
{
    m_width  = width;
    m_height = height;
    m_format = DXGI_FORMAT_D32_FLOAT;

    // R32_TYPELESSで作成することで、DSVにはD32_FLOAT、SRVにはR32_FLOATとして
    // 同一リソースを異なるフォーマットで参照できる
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width            = width;
    depthDesc.Height           = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels        = 1;
    depthDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format               = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth   = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&m_resource));

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create depth buffer with SRV (HRESULT: 0x%08X)", hr);
        return false;
    }

    // DSV
    if (!m_dsvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false))
        return false;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsvHeap.GetCPUHandle(0));

    // SRV — シェーダーから深度値をR32_FLOATとして読み取る
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                  = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels    = 1;
    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, srvHeap->GetCPUHandle(srvIndex));
    m_srvGPUHandle = srvHeap->GetGPUHandle(srvIndex);

    GX_LOG_INFO("DepthBuffer with SRV created (%dx%d)", width, height);
    return true;
}

bool DepthBuffer::CreateWithOwnSRV(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_format = DXGI_FORMAT_D32_FLOAT;
    m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    // CreateWithSRVと同じくR32_TYPELESSで作成
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width            = width;
    depthDesc.Height           = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels        = 1;
    depthDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format               = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth   = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&m_resource));

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create depth buffer with own SRV (HRESULT: 0x%08X)", hr);
        return false;
    }

    // DSV
    if (!m_dsvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false))
        return false;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsvHeap.GetCPUHandle(0));

    // 自前のshader-visibleヒープにSRVを作成
    // ポストエフェクト(SSAO等)で他のヒープと独立してバインドする必要があるため
    if (!m_ownSrvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true))
        return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                  = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels    = 1;
    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_ownSrvHeap.GetCPUHandle(0));

    m_hasOwnSRV = true;

    GX_LOG_INFO("DepthBuffer with own SRV heap created (%dx%d)", width, height);
    return true;
}

void DepthBuffer::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
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
