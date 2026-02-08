/// @file CommandQueue.cpp
/// @brief コマンドキュー管理の実装
#include "pch.h"
#include "Graphics/Device/CommandQueue.h"
#include "Core/Logger.h"

namespace GX
{

bool CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type     = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create command queue (HRESULT: 0x%08X)", hr);
        return false;
    }

    if (!m_fence.Initialize(device))
    {
        GX_LOG_ERROR("Failed to initialize fence for command queue");
        return false;
    }

    GX_LOG_INFO("Command Queue created (type: %d)", static_cast<int>(type));
    return true;
}

void CommandQueue::ExecuteCommandLists(ID3D12CommandList* const* lists, uint32_t count)
{
    m_queue->ExecuteCommandLists(count, lists);
}

void CommandQueue::ExecuteCommandList(ID3D12CommandList* list)
{
    ExecuteCommandLists(&list, 1);
}

void CommandQueue::Flush()
{
    m_fence.WaitForGPU(m_queue.Get());
}

} // namespace GX
