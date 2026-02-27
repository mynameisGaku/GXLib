/// @file AsyncComputeQueue.cpp
/// @brief 非同期コンピュートキューの実装
#include "pch_graphics.h"
#include "Graphics/Device/AsyncComputeQueue.h"
#include "Core/Logger.h"

namespace gx
{

bool AsyncComputeQueue::Initialize(ID3D12Device* device)
{
    // Compute コマンドキューを作成
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("AsyncComputeQueue: CreateCommandQueue failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }

    // コマンドアロケータを作成（ダブルバッファ）
    for (uint32_t i = 0; i < k_AllocatorCount; ++i)
    {
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                             IID_PPV_ARGS(&m_allocators[i]));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("AsyncComputeQueue: CreateCommandAllocator failed for frame {}", i);
            return false;
        }
    }

    // コマンドリストを作成
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                    m_allocators[0].Get(), nullptr,
                                    IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("AsyncComputeQueue: CreateCommandList failed");
        return false;
    }
    m_commandList->Close(); // 初回は閉じた状態にしておく

    // フェンスを初期化
    if (!m_fence.Initialize(device))
        return false;
    if (!m_syncFence.Initialize(device))
        return false;

    return true;
}

bool AsyncComputeQueue::BeginRecord(uint32_t frameIndex, ID3D12PipelineState* initialPSO)
{
    auto* allocator = m_allocators[frameIndex % k_AllocatorCount].Get();
    HRESULT hr = allocator->Reset();
    if (FAILED(hr)) return false;

    hr = m_commandList->Reset(allocator, initialPSO);
    return SUCCEEDED(hr);
}

void AsyncComputeQueue::EndRecordAndExecute()
{
    m_commandList->Close();

    ID3D12CommandList* lists[] = { m_commandList.Get() };
    m_queue->ExecuteCommandLists(1, lists);
}

void AsyncComputeQueue::WaitForGraphics(ID3D12CommandQueue* graphicsQueue)
{
    // Graphics キュー側で同期フェンスに Signal を発行
    uint64_t val = m_syncFence.Signal(graphicsQueue);
    // Compute キュー側でその値に到達するまで GPU Wait（CPU は止めない）
    m_queue->Wait(m_syncFence.GetD3D12Fence(), val);
}

void AsyncComputeQueue::SignalToGraphics(ID3D12CommandQueue* graphicsQueue)
{
    // Compute キュー側でフェンスに Signal を発行
    uint64_t val = m_fence.Signal(m_queue.Get());
    // Graphics キュー側で Compute の完了を GPU Wait（CPU は止めない）
    graphicsQueue->Wait(m_fence.GetD3D12Fence(), val);
}

void AsyncComputeQueue::Flush()
{
    m_fence.WaitForGPU(m_queue.Get());
}

} // namespace gx
