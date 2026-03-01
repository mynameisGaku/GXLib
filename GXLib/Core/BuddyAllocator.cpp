/// @file BuddyAllocator.cpp
/// @brief 対応する.hの実装
#include "pch_common.h"
#include "Core/BuddyAllocator.h"
#include <limits>

namespace gx
{

bool BuddyAllocator::Initialize(size_t totalSize, size_t minBlockSize)
{
    if (totalSize == 0 || minBlockSize == 0) return false;

    // Round totalSize up to power of 2
    m_totalSize = RoundUpToPow2(totalSize);
    m_minBlockSize = RoundUpToPow2(minBlockSize);
    m_usedSize = 0;
    m_allocationCount = 0;

    // Calculate max order: totalSize = minBlockSize * 2^maxOrder
    m_maxOrder = 0;
    size_t tmp = m_totalSize / m_minBlockSize;
    while (tmp > 1) { tmp >>= 1; m_maxOrder++; }

    // Initialize free lists (order 0 = minBlockSize, order maxOrder = totalSize)
    m_freeLists.clear();
    m_freeLists.resize(m_maxOrder + 1);
    m_allocated.clear();

    // The entire pool starts as one free block at max order
    m_freeLists[m_maxOrder].insert(0);

    return true;
}

size_t BuddyAllocator::Allocate(size_t size)
{
    if (size == 0) return std::numeric_limits<size_t>::max();

    // Round up to at least minBlockSize and power of 2
    size_t blockSize = RoundUpToPow2((std::max)(size, m_minBlockSize));
    uint32_t order = SizeToOrder(blockSize);

    if (order > m_maxOrder)
        return std::numeric_limits<size_t>::max();

    // Find smallest order with a free block
    uint32_t foundOrder = order;
    while (foundOrder <= m_maxOrder && m_freeLists[foundOrder].empty())
        foundOrder++;

    if (foundOrder > m_maxOrder)
        return std::numeric_limits<size_t>::max();

    // Take a block from the found order
    size_t offset = *m_freeLists[foundOrder].begin();
    m_freeLists[foundOrder].erase(m_freeLists[foundOrder].begin());

    // Split down to the requested order
    while (foundOrder > order)
    {
        foundOrder--;
        // Create the buddy (second half) and put it in the free list
        size_t buddyOffset = offset + OrderToSize(foundOrder);
        m_freeLists[foundOrder].insert(buddyOffset);
    }

    // Mark as allocated
    m_allocated[offset] = order;
    m_usedSize += OrderToSize(order);
    m_allocationCount++;

    return offset;
}

void BuddyAllocator::Free(size_t offset)
{
    auto it = m_allocated.find(offset);
    if (it == m_allocated.end()) return;

    uint32_t order = it->second;
    m_usedSize -= OrderToSize(order);
    m_allocationCount--;
    m_allocated.erase(it);

    // Try to merge with buddy recursively
    while (order < m_maxOrder)
    {
        size_t buddyOffset = GetBuddyOffset(offset, order);

        // Check if buddy is free at the same order
        auto buddyIt = m_freeLists[order].find(buddyOffset);
        if (buddyIt == m_freeLists[order].end())
            break; // Buddy is not free, cannot merge

        // Remove buddy from free list
        m_freeLists[order].erase(buddyIt);

        // Merge: take the smaller offset as the merged block
        offset = (std::min)(offset, buddyOffset);
        order++;
    }

    // Insert the (possibly merged) block into free list
    m_freeLists[order].insert(offset);
}

void BuddyAllocator::Reset()
{
    m_usedSize = 0;
    m_allocationCount = 0;
    m_allocated.clear();

    for (auto& list : m_freeLists)
        list.clear();

    // Re-create the initial free block
    m_freeLists[m_maxOrder].insert(0);
}

size_t BuddyAllocator::RoundUpToPow2(size_t size) const
{
    if (size == 0) return 1;
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size |= size >> 32;
    size++;
    return size;
}

uint32_t BuddyAllocator::SizeToOrder(size_t size) const
{
    // order 0 = minBlockSize, order 1 = 2*minBlockSize, etc.
    uint32_t order = 0;
    size_t s = m_minBlockSize;
    while (s < size) { s <<= 1; order++; }
    return order;
}

size_t BuddyAllocator::OrderToSize(uint32_t order) const
{
    return m_minBlockSize << order;
}

size_t BuddyAllocator::GetBuddyOffset(size_t offset, uint32_t order) const
{
    size_t blockSize = OrderToSize(order);
    return offset ^ blockSize;
}

} // namespace gx
