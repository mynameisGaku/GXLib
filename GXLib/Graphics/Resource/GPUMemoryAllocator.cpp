#include "pch_graphics.h"
/// @file GPUMemoryAllocator.cpp
/// @brief GPUメモリアロケータ実装 -- ヒープ管理と一時リングバッファ

#include "Graphics/Resource/GPUMemoryAllocator.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// 初期化
// ============================================================================

bool GPUMemoryAllocator::Initialize(ID3D12Device* device, uint64_t defaultHeapSize,
                                     uint64_t ringBufferSize)
{
    if (!device)
    {
        GX_LOG_ERROR("GPUMemoryAllocator::Initialize: device is null");
        return false;
    }

    m_device = device;
    m_defaultHeapSize = defaultHeapSize;
    m_stats = {};

    // 最初のデフォルトヒープを作成する
    if (!CreateHeap(defaultHeapSize))
    {
        GX_LOG_ERROR("GPUMemoryAllocator::Initialize: failed to create initial heap");
        return false;
    }

    // 一時リングバッファを作成する（アップロードヒープ、永続マップ）
    m_ringBufferSize = ringBufferSize;
    m_ringBufferHead = 0;
    for (auto& fence : m_ringBufferFrameFences)
        fence = 0;

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC ringDesc = {};
    ringDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ringDesc.Width = ringBufferSize;
    ringDesc.Height = 1;
    ringDesc.DepthOrArraySize = 1;
    ringDesc.MipLevels = 1;
    ringDesc.SampleDesc.Count = 1;
    ringDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = m_device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &ringDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_ringBuffer));

    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUMemoryAllocator::Initialize: failed to create ring buffer (HRESULT: 0x%08X)", hr);
        return false;
    }

    m_ringBuffer->SetName(L"GPUMemoryAllocator_RingBuffer");

    // リングバッファを永続的にマップする
    D3D12_RANGE readRange = {}; // CPU側からこのリソースは読み取らない
    hr = m_ringBuffer->Map(0, &readRange, &m_ringBufferCPU);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUMemoryAllocator::Initialize: failed to map ring buffer");
        m_ringBuffer.Reset();
        return false;
    }

    m_ringBufferGPU = m_ringBuffer->GetGPUVirtualAddress();

    GX_LOG_INFO("GPUMemoryAllocator initialized (heapSize=%llu MB, ringBuffer=%llu MB)",
                defaultHeapSize / (1024 * 1024), ringBufferSize / (1024 * 1024));
    return true;
}

// ============================================================================
// ヒープ作成
// ============================================================================

bool GPUMemoryAllocator::CreateHeap(uint64_t size)
{
    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.SizeInBytes = size;
    heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

    HeapBlock block;
    HRESULT hr = m_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&block.heap));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUMemoryAllocator::CreateHeap: D3D12 CreateHeap failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    block.totalSize = size;

    // ヒープ全体を1つの空き領域として開始する
    FreeRange freeRange;
    freeRange.offset = 0;
    freeRange.size = size;
    block.freeRanges.push_back(freeRange);

    m_heaps.push_back(std::move(block));
    m_stats.totalHeapSize += size;
    m_stats.heapCount = static_cast<uint32_t>(m_heaps.size());

    return true;
}

// ============================================================================
// 確保（ファーストフィット）
// ============================================================================

GPUAllocation GPUMemoryAllocator::Allocate(uint64_t size, uint64_t alignment)
{
    GPUAllocation result;
    if (size == 0 || !m_device)
        return result;

    // 既存の各ヒープを試す
    for (uint32_t hi = 0; hi < static_cast<uint32_t>(m_heaps.size()); ++hi)
    {
        auto& heap = m_heaps[hi];

        for (size_t fi = 0; fi < heap.freeRanges.size(); ++fi)
        {
            auto& freeRange = heap.freeRanges[fi];

            // オフセットをアラインメントする
            uint64_t alignedOffset = (freeRange.offset + alignment - 1) & ~(alignment - 1);
            uint64_t alignmentPadding = alignedOffset - freeRange.offset;
            uint64_t totalRequired = alignmentPadding + size;

            if (totalRequired <= freeRange.size)
            {
                result.offset = alignedOffset;
                result.size = size;
                result.heapIndex = hi;
                result.resource = nullptr; // 呼び出し元が配置リソースを作成する
                result.valid = true;

                // 空き領域を分割する
                if (alignmentPadding > 0)
                {
                    // アラインメントギャップ用に小さな空き領域を保持する
                    FreeRange gapRange;
                    gapRange.offset = freeRange.offset;
                    gapRange.size = alignmentPadding;

                    uint64_t remainingOffset = alignedOffset + size;
                    uint64_t remainingSize = freeRange.size - totalRequired;

                    // 現在の空き領域を残り（アロケーション後）で置き換える
                    if (remainingSize > 0)
                    {
                        freeRange.offset = remainingOffset;
                        freeRange.size = remainingSize;
                    }
                    else
                    {
                        heap.freeRanges.erase(heap.freeRanges.begin() + static_cast<ptrdiff_t>(fi));
                    }

                    // アラインメントギャップを空き領域として挿入する
                    heap.freeRanges.push_back(gapRange);
                }
                else
                {
                    uint64_t remainingOffset = alignedOffset + size;
                    uint64_t remainingSize = freeRange.size - size;

                    if (remainingSize > 0)
                    {
                        freeRange.offset = remainingOffset;
                        freeRange.size = remainingSize;
                    }
                    else
                    {
                        heap.freeRanges.erase(heap.freeRanges.begin() + static_cast<ptrdiff_t>(fi));
                    }
                }

                m_stats.totalAllocated += size;
                m_stats.allocationCount++;
                if (m_stats.totalAllocated > m_stats.peakAllocated)
                    m_stats.peakAllocated = m_stats.totalAllocated;

                return result;
            }
        }
    }

    // 既存ヒープでは要求を満たせない -- 新しいヒープを作成する
    uint64_t newHeapSize = (std::max)(m_defaultHeapSize, size + alignment);
    if (!CreateHeap(newHeapSize))
    {
        GX_LOG_ERROR("GPUMemoryAllocator::Allocate: failed to create new heap for %llu bytes", size);
        return result;
    }

    // 新しいヒープからアロケーションを再試行する（空なので成功する）
    uint32_t newHeapIndex = static_cast<uint32_t>(m_heaps.size()) - 1;
    auto& newHeap = m_heaps[newHeapIndex];
    auto& freeRange = newHeap.freeRanges[0];

    uint64_t alignedOffset = (freeRange.offset + alignment - 1) & ~(alignment - 1);
    result.offset = alignedOffset;
    result.size = size;
    result.heapIndex = newHeapIndex;
    result.resource = nullptr;
    result.valid = true;

    uint64_t remainingOffset = alignedOffset + size;
    uint64_t remainingSize = freeRange.size - (alignedOffset - freeRange.offset) - size;

    if (remainingSize > 0)
    {
        freeRange.offset = remainingOffset;
        freeRange.size = remainingSize;
    }
    else
    {
        newHeap.freeRanges.clear();
    }

    m_stats.totalAllocated += size;
    m_stats.allocationCount++;
    if (m_stats.totalAllocated > m_stats.peakAllocated)
        m_stats.peakAllocated = m_stats.totalAllocated;

    return result;
}

// ============================================================================
// 解放
// ============================================================================

void GPUMemoryAllocator::Free(const GPUAllocation& alloc)
{
    if (!alloc.valid)
        return;

    if (alloc.heapIndex >= static_cast<uint32_t>(m_heaps.size()))
        return;

    auto& heap = m_heaps[alloc.heapIndex];

    // 解放された領域を戻す
    FreeRange newFree;
    newFree.offset = alloc.offset;
    newFree.size = alloc.size;

    // マージのためにオフセット順にソートして挿入する
    auto it = heap.freeRanges.begin();
    while (it != heap.freeRanges.end() && it->offset < newFree.offset)
        ++it;
    it = heap.freeRanges.insert(it, newFree);

    // 次の空き領域とのマージを試みる
    auto next = it;
    ++next;
    if (next != heap.freeRanges.end() && it->offset + it->size == next->offset)
    {
        it->size += next->size;
        heap.freeRanges.erase(next);
    }

    // 前の空き領域とのマージを試みる
    if (it != heap.freeRanges.begin())
    {
        auto prev = it;
        --prev;
        if (prev->offset + prev->size == it->offset)
        {
            prev->size += it->size;
            heap.freeRanges.erase(it);
        }
    }

    // 統計を更新する
    if (m_stats.totalAllocated >= alloc.size)
        m_stats.totalAllocated -= alloc.size;
    else
        m_stats.totalAllocated = 0;

    if (m_stats.allocationCount > 0)
        m_stats.allocationCount--;
}

// ============================================================================
// 一時リングバッファ
// ============================================================================

TransientAllocation GPUMemoryAllocator::AllocateTransient(uint64_t size, uint64_t alignment)
{
    TransientAllocation result;
    if (size == 0 || !m_ringBuffer)
        return result;

    // ヘッドをアラインメントする
    uint64_t alignedHead = (m_ringBufferHead + alignment - 1) & ~(alignment - 1);

    // ラップなしでリングバッファ内に収まるかチェックする
    if (alignedHead + size <= m_ringBufferSize)
    {
        result.cpuAddress = static_cast<uint8_t*>(m_ringBufferCPU) + alignedHead;
        result.gpuAddress = m_ringBufferGPU + alignedHead;
        result.size = size;
        result.valid = true;
        m_ringBufferHead = alignedHead + size;
        return result;
    }

    // 先頭へのラップを試みる
    alignedHead = 0;
    if (size <= m_ringBufferSize)
    {
        result.cpuAddress = static_cast<uint8_t*>(m_ringBufferCPU);
        result.gpuAddress = m_ringBufferGPU;
        result.size = size;
        result.valid = true;
        m_ringBufferHead = size;
        return result;
    }

    // アロケーションがリングバッファに対して大きすぎる
    GX_LOG_WARN("GPUMemoryAllocator::AllocateTransient: allocation of %llu bytes exceeds ring buffer size", size);
    return result;
}

void GPUMemoryAllocator::ResetTransientForFrame(uint64_t frameIndex)
{
    // 現在のヘッド位置をこのフレームのウォーターマークとして保存する
    uint32_t slot = static_cast<uint32_t>(frameIndex % 3);
    m_ringBufferFrameFences[slot] = m_ringBufferHead;

    // 最も古いフレームの領域は安全に再利用できる。
    // リングバッファのヘッドは現在の位置から継続する。
    // 実際にはリングバッファがラップした場合、古いフレームデータは
    // 十分なフレームが経過した後にのみ上書きされる（安全のため3フレームのレイテンシ）。

    // ラップして古いフレームの領域と衝突しそうな場合、
    // ゼロにリセットする（3フレームのレイテンシがあるので最も古いフレームは完了しているはず）。
    uint32_t oldestSlot = static_cast<uint32_t>((frameIndex + 1) % 3);
    (void)oldestSlot; // ウォーターマークは暗黙的 -- リングバッファは単に進行する

    // 簡易戦略: 3フレームごとに、ヘッドが容量の半分を超えていたら0にラップする
    if (frameIndex % 3 == 0 && m_ringBufferHead > m_ringBufferSize / 2)
    {
        m_ringBufferHead = 0;
    }
}

// ============================================================================
// 統計取得
// ============================================================================

GPUMemoryStats GPUMemoryAllocator::GetStats() const
{
    return m_stats;
}

// ============================================================================
// シャットダウン
// ============================================================================

void GPUMemoryAllocator::Shutdown()
{
    // リングバッファのアンマップと解放
    if (m_ringBuffer && m_ringBufferCPU)
    {
        m_ringBuffer->Unmap(0, nullptr);
        m_ringBufferCPU = nullptr;
    }
    m_ringBuffer.Reset();
    m_ringBufferGPU = 0;
    m_ringBufferSize = 0;
    m_ringBufferHead = 0;
    for (auto& fence : m_ringBufferFrameFences)
        fence = 0;

    // 全ヒープを解放する
    m_heaps.clear();

    m_device = nullptr;
    m_stats = {};
}

} // namespace gx
