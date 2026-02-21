/// @file SwapChain.cpp
/// @brief スワップチェーンの実装
#include "pch.h"
#include "Graphics/Device/SwapChain.h"
#include "Graphics/Device/Fence.h"
#include "Core/Logger.h"

namespace GX
{

bool SwapChain::Initialize(IDXGIFactory6* factory,
                            ID3D12Device* device,
                            ID3D12CommandQueue* queue,
                            const SwapChainDesc& desc)
{
    m_width  = desc.width;
    m_height = desc.height;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width       = m_width;
    swapChainDesc.Height      = m_height;
    swapChainDesc.Format      = m_format;
    swapChainDesc.Stereo      = FALSE;
    swapChainDesc.SampleDesc  = { 1, 0 };  // MSAAなし（DX12ではResolveで別途対応する流儀）
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = k_BufferCount;
    swapChainDesc.Scaling     = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップモデル（DX12必須）
    swapChainDesc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags       = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // SwapChain1を作ってからSwapChain4にキャスト
    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = factory->CreateSwapChainForHwnd(
        queue, desc.hwnd, &swapChainDesc,
        nullptr, nullptr, &swapChain1
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create swap chain (HRESULT: 0x%08X)", hr);
        return false;
    }

    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to get IDXGISwapChain4 (HRESULT: 0x%08X)", hr);
        return false;
    }

    // Alt+Enterによるフルスクリーン切り替えを無効化（自前で制御するため）
    factory->MakeWindowAssociation(desc.hwnd, DXGI_MWA_NO_ALT_ENTER);

    // バックバッファのRTVを作成
    if (!m_rtvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, k_BufferCount))
    {
        GX_LOG_ERROR("Failed to create RTV heap for swap chain");
        return false;
    }

    CreateRenderTargetViews(device);

    GX_LOG_INFO("Swap Chain created: %dx%d, %d buffers", m_width, m_height, k_BufferCount);
    return true;
}

void SwapChain::Present(bool vsync)
{
    UINT syncInterval = vsync ? 1 : 0;  // 0=即座にフリップ, 1=VSync待ち
    UINT flags = 0;
    m_swapChain->Present(syncInterval, flags);
}

bool SwapChain::Resize(ID3D12Device* device, uint32_t width, uint32_t height,
                       ID3D12CommandQueue* queue, Fence* fence)
{
    if (width == 0 || height == 0)
        return false;

    // バックバッファはGPU使用中に解放できないので、先に完了を待つ
    if (queue && fence)
    {
        fence->WaitForGPU(queue);
    }

    m_width  = width;
    m_height = height;

    // 既存バックバッファの参照を解放してからResizeBuffers
    for (auto& buffer : m_backBuffers)
        buffer.Reset();

    HRESULT hr = m_swapChain->ResizeBuffers(
        k_BufferCount, width, height, m_format,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to resize swap chain (HRESULT: 0x%08X)", hr);
        return false;
    }

    // 新しいバッファに対してRTVを再作成
    CreateRenderTargetViews(device);
    GX_LOG_INFO("Swap Chain resized: %dx%d", width, height);
    return true;
}

ID3D12Resource* SwapChain::GetCurrentBackBuffer() const
{
    uint32_t index = m_swapChain->GetCurrentBackBufferIndex();
    return m_backBuffers[index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentRTVHandle() const
{
    uint32_t index = m_swapChain->GetCurrentBackBufferIndex();
    return m_rtvHeap.GetCPUHandle(index);
}

uint32_t SwapChain::GetCurrentBackBufferIndex() const
{
    return m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::CreateRenderTargetViews(ID3D12Device* device)
{
    for (uint32_t i = 0; i < k_BufferCount; ++i)
    {
        // SwapChainが持つバッファのCOMポインタを取得
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        // RTVを作成（nullptrでデフォルト設定 = バッファのフォーマットに合わせる）
        device->CreateRenderTargetView(
            m_backBuffers[i].Get(), nullptr, m_rtvHeap.GetCPUHandle(i)
        );
    }
}

} // namespace GX
