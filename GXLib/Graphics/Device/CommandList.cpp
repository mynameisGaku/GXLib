/// @file CommandList.cpp
/// @brief コマンドリストの実装
#include "pch.h"
#include "Graphics/Device/CommandList.h"
#include "Core/Logger.h"

namespace GX
{

bool CommandList::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    // ダブルバッファリング分のアロケータを作成
    // GPUが前フレームのアロケータを使用中でも、別のアロケータで次フレームの記録を始められる
    for (uint32_t i = 0; i < k_AllocatorCount; ++i)
    {
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocators[i]));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("Failed to create command allocator %d (HRESULT: 0x%08X)", i, hr);
            return false;
        }
    }

    HRESULT hr = device->CreateCommandList(
        0, type, m_allocators[0].Get(), nullptr,
        IID_PPV_ARGS(&m_commandList)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create command list (HRESULT: 0x%08X)", hr);
        return false;
    }

    // DXR用: CommandList4を取得（非対応GPUではnullのまま）
    m_commandList.As(&m_commandList4);

    // D3D12はCreateCommandList直後がOpen状態なので、初回Resetに備えてCloseしておく
    m_commandList->Close();

    GX_LOG_INFO("Command List created");
    return true;
}

bool CommandList::Reset(uint32_t frameIndex, ID3D12PipelineState* initialPSO)
{
    // frameIndexからアロケータを選択（0→allocator[0], 1→allocator[1]）
    uint32_t allocatorIndex = frameIndex % k_AllocatorCount;

    // アロケータのリセット（内部のコマンドメモリを解放する）
    // GPU側でこのアロケータの命令を実行し終えている必要がある
    HRESULT hr = m_allocators[allocatorIndex]->Reset();
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to reset command allocator (HRESULT: 0x%08X)", hr);
        return false;
    }

    hr = m_commandList->Reset(m_allocators[allocatorIndex].Get(), initialPSO);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to reset command list (HRESULT: 0x%08X)", hr);
        return false;
    }

    return true;
}

bool CommandList::Close()
{
    HRESULT hr = m_commandList->Close();
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to close command list (HRESULT: 0x%08X)", hr);
        return false;
    }
    return true;
}

} // namespace GX
