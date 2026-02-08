/// @file Fence.cpp
/// @brief GPU同期用フェンスの実装
#include "pch.h"
#include "Graphics/Device/Fence.h"
#include "Core/Logger.h"

namespace GX
{

Fence::~Fence()
{
    if (m_event)
    {
        CloseHandle(m_event);
        m_event = nullptr;
    }
}

bool Fence::Initialize(ID3D12Device* device)
{
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create fence (HRESULT: 0x%08X)", hr);
        return false;
    }

    m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_event)
    {
        GX_LOG_ERROR("Failed to create fence event");
        return false;
    }

    m_fenceValue = 0;
    return true;
}

uint64_t Fence::Signal(ID3D12CommandQueue* queue)
{
    m_fenceValue++;
    HRESULT hr = queue->Signal(m_fence.Get(), m_fenceValue);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to signal fence (HRESULT: 0x%08X)", hr);
    }
    return m_fenceValue;
}

void Fence::WaitForValue(uint64_t value)
{
    if (m_fence->GetCompletedValue() < value)
    {
        m_fence->SetEventOnCompletion(value, m_event);
        WaitForSingleObject(m_event, INFINITE);
    }
}

void Fence::WaitForGPU(ID3D12CommandQueue* queue)
{
    uint64_t value = Signal(queue);
    WaitForValue(value);
}

} // namespace GX
