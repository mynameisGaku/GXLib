/// @file GraphicsDevice.cpp
/// @brief D3D12デバイス初期化の実装
#include "pch.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Core/Logger.h"

namespace GX
{

bool GraphicsDevice::Initialize(bool enableDebugLayer)
{
    GX_LOG_INFO("Initializing Graphics Device...");

    if (enableDebugLayer)
    {
        EnableDebugLayer();
    }

    UINT factoryFlags = enableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0;
    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create DXGI Factory (HRESULT: 0x%08X)", hr);
        return false;
    }

    if (!SelectAdapter())
    {
        GX_LOG_ERROR("Failed to select GPU adapter");
        return false;
    }

    hr = D3D12CreateDevice(
        m_adapter.Get(),
        D3D_FEATURE_LEVEL_12_0,
        IID_PPV_ARGS(&m_device)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create D3D12 Device (HRESULT: 0x%08X)", hr);
        return false;
    }

    if (enableDebugLayer)
    {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_device.As(&infoQueue)))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            GX_LOG_INFO("D3D12 Debug info queue configured");
        }
    }

    GX_LOG_INFO("Graphics Device initialized successfully");
    return true;
}

void GraphicsDevice::EnableDebugLayer()
{
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        GX_LOG_INFO("D3D12 Debug Layer enabled");
    }
    else
    {
        GX_LOG_WARN("Failed to enable D3D12 Debug Layer");
    }
}

bool GraphicsDevice::SelectAdapter()
{
    ComPtr<IDXGIAdapter1> adapter;

    for (UINT i = 0;
        m_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
        ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
            __uuidof(ID3D12Device), nullptr)))
        {
            m_adapter = adapter;
            GX_LOG_INFO("Selected GPU: %ls", desc.Description);
            GX_LOG_INFO("  Video Memory: %llu MB", desc.DedicatedVideoMemory / (1024 * 1024));
            return true;
        }
    }

    GX_LOG_ERROR("No compatible GPU found");
    return false;
}

} // namespace GX
