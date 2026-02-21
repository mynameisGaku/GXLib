/// @file DescriptorHeap.cpp
/// @brief ディスクリプタヒープの実装
#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Core/Logger.h"

namespace GX
{

bool DescriptorHeap::Initialize(ID3D12Device* device,
                                 D3D12_DESCRIPTOR_HEAP_TYPE type,
                                 uint32_t numDescriptors,
                                 bool shaderVisible)
{
    m_type           = type;
    m_numDescriptors = numDescriptors;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type           = type;
    desc.NumDescriptors = numDescriptors;
    // SHADER_VISIBLEはCBV_SRV_UAVとSAMPLERのみ有効。RTV/DSVには指定しない
    desc.Flags          = shaderVisible
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create descriptor heap (HRESULT: 0x%08X)", hr);
        return false;
    }

    // ディスクリプタ1つ分のサイズはGPU依存なのでAPI経由で取得する
    m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
    m_currentIndex = 0;

    return true;
}

uint32_t DescriptorHeap::AllocateIndex()
{
    // フリーリストに空きがあればそこから再利用（テクスチャ解放→再ロード等のケース）
    if (!m_freeList.empty())
    {
        uint32_t index = m_freeList.back();
        m_freeList.pop_back();
        return index;
    }

    if (m_currentIndex >= m_numDescriptors)
    {
        GX_LOG_ERROR("Descriptor heap is full! (capacity: %u)", m_numDescriptors);
        return k_InvalidIndex;
    }
    return m_currentIndex++;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::Allocate()
{
    uint32_t index = AllocateIndex();
    if (index == k_InvalidIndex)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
        handle.ptr = 0;
        return handle;
    }
    return GetCPUHandle(index);
}

void DescriptorHeap::Free(uint32_t index)
{
    if (index >= m_numDescriptors)
    {
        GX_LOG_ERROR("Invalid descriptor index %u (capacity: %u)", index, m_numDescriptors);
        return;
    }

    // 同じインデックスの二重解放を検出（デバッグ用）
    for (uint32_t freeIdx : m_freeList)
    {
        if (freeIdx == index)
        {
            GX_LOG_ERROR("Double-free of descriptor index %u detected!", index);
            return;
        }
    }

    m_freeList.push_back(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(uint32_t index) const
{
    // ヒープ先頭 + (インデックス * ディスクリプタサイズ) でハンドルを計算
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * m_descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(uint32_t index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * m_descriptorSize;
    return handle;
}

} // namespace GX
