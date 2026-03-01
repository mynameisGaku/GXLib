/// @file MemoryProfiler.cpp
/// @brief メモリプロファイラの実装

#include "pch_common.h"
#include "Core/MemoryProfiler.h"

#include <algorithm>

namespace gx
{

// =============================================================================
// 初期化 / シャットダウン
// =============================================================================

void MemoryProfiler::Initialize()
{
    m_enabled = true;
    Reset();
}

void MemoryProfiler::Shutdown()
{
    m_enabled = false;
    m_activeAllocations.clear();
    m_current = {};
    m_peak    = {};
    m_frameNumber = 0;
}

// =============================================================================
// 記録
// =============================================================================

void MemoryProfiler::RecordAllocation(void* ptr, size_t size, const std::string& category)
{
    if (!m_enabled || ptr == nullptr)
    {
        return;
    }

    AllocationRecord record;
    record.address     = ptr;
    record.size        = size;
    record.category    = category;
    record.frameNumber = m_frameNumber;

    m_activeAllocations[ptr] = record;

    // 統計更新
    m_current.totalAllocated += size;
    m_current.currentUsage   += size;
    m_current.allocationCount++;

    // カテゴリ別使用量
    m_current.categoryUsage[category] += size;

    // ピーク更新
    if (m_current.currentUsage > m_current.peakUsage)
    {
        m_current.peakUsage = m_current.currentUsage;
    }

    // ピークスナップショット更新
    if (m_current.currentUsage > m_peak.peakUsage)
    {
        m_peak = m_current;
        m_peak.frameNumber = m_frameNumber;
    }
}

void MemoryProfiler::RecordFree(void* ptr)
{
    if (!m_enabled || ptr == nullptr)
    {
        return;
    }

    auto it = m_activeAllocations.find(ptr);
    if (it == m_activeAllocations.end())
    {
        return; // 未追跡のアドレスは無視
    }

    const AllocationRecord& record = it->second;

    // 統計更新
    m_current.totalFreed   += record.size;
    m_current.currentUsage -= record.size;
    m_current.freeCount++;

    // カテゴリ別使用量を減算
    auto catIt = m_current.categoryUsage.find(record.category);
    if (catIt != m_current.categoryUsage.end())
    {
        if (catIt->second >= record.size)
        {
            catIt->second -= record.size;
        }
        else
        {
            catIt->second = 0;
        }
        // 0 になったカテゴリは残しておく（履歴として）
    }

    m_activeAllocations.erase(it);
}

// =============================================================================
// フレーム管理
// =============================================================================

void MemoryProfiler::BeginFrame(uint32_t frameNumber)
{
    m_frameNumber = frameNumber;
    m_current.frameNumber = frameNumber;
}

void MemoryProfiler::EndFrame()
{
    // ピークスナップショットの更新確認
    if (m_current.currentUsage > m_peak.peakUsage)
    {
        m_peak = m_current;
    }
}

// =============================================================================
// スナップショット取得
// =============================================================================

MemorySnapshot MemoryProfiler::GetCurrentSnapshot() const
{
    return m_current;
}

MemorySnapshot MemoryProfiler::GetPeakSnapshot() const
{
    return m_peak;
}

// =============================================================================
// カテゴリ別情報
// =============================================================================

uint64_t MemoryProfiler::GetCategoryUsage(const std::string& category) const
{
    auto it = m_current.categoryUsage.find(category);
    if (it != m_current.categoryUsage.end())
    {
        return it->second;
    }
    return 0;
}

std::vector<std::string> MemoryProfiler::GetCategories() const
{
    std::vector<std::string> categories;
    categories.reserve(m_current.categoryUsage.size());
    for (const auto& [cat, usage] : m_current.categoryUsage)
    {
        categories.push_back(cat);
    }
    std::sort(categories.begin(), categories.end());
    return categories;
}

// =============================================================================
// リーク検出
// =============================================================================

std::vector<AllocationRecord> MemoryProfiler::GetLeakedAllocations() const
{
    std::vector<AllocationRecord> leaks;
    leaks.reserve(m_activeAllocations.size());
    for (const auto& [ptr, record] : m_activeAllocations)
    {
        leaks.push_back(record);
    }
    return leaks;
}

bool MemoryProfiler::HasLeaks() const
{
    return !m_activeAllocations.empty();
}

// =============================================================================
// 統計
// =============================================================================

uint64_t MemoryProfiler::GetTotalAllocated() const
{
    return m_current.totalAllocated;
}

uint64_t MemoryProfiler::GetTotalFreed() const
{
    return m_current.totalFreed;
}

uint64_t MemoryProfiler::GetCurrentUsage() const
{
    return m_current.currentUsage;
}

uint64_t MemoryProfiler::GetPeakUsage() const
{
    return m_current.peakUsage;
}

uint32_t MemoryProfiler::GetAllocationCount() const
{
    return m_current.allocationCount;
}

// =============================================================================
// リセット
// =============================================================================

void MemoryProfiler::Reset()
{
    m_activeAllocations.clear();
    m_current     = {};
    m_peak        = {};
    m_frameNumber = 0;
}

// =============================================================================
// 有効/無効
// =============================================================================

void MemoryProfiler::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool MemoryProfiler::IsEnabled() const
{
    return m_enabled;
}

} // namespace gx
