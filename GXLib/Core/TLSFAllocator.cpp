/// @file TLSFAllocator.cpp
/// @brief 対応する.hの実装
#include "pch_common.h"
#include "Core/TLSFAllocator.h"
#include <bit>
#include <limits>

namespace gx
{

TLSFAllocator::~TLSFAllocator()
{
    // unique_ptr handles cleanup
}

bool TLSFAllocator::Initialize(size_t poolSize, size_t minBlockSize)
{
    if (poolSize == 0 || minBlockSize == 0) return false;

    m_poolSize = poolSize;
    m_minBlockSize = minBlockSize;
    m_usedSize = 0;
    m_allocationCount = 0;
    m_flBitmap = 0;

    std::memset(m_slBitmap, 0, sizeof(m_slBitmap));
    std::memset(m_freeList, 0, sizeof(m_freeList));

    m_blockStorage.clear();
    m_offsetToBlock.clear();

    // Create the initial free block spanning the entire pool
    auto block = std::make_unique<TLSFBlock>();
    block->size = poolSize;
    block->free = true;
    block->offset = 0;
    block->prev = nullptr;
    block->next = nullptr;
    block->prevFree = nullptr;
    block->nextFree = nullptr;

    TLSFBlock* rawPtr = block.get();
    m_blockStorage.push_back(std::move(block));
    m_offsetToBlock[0] = rawPtr;

    InsertFreeBlock(rawPtr);

    return true;
}

size_t TLSFAllocator::Allocate(size_t size)
{
    if (size == 0) return std::numeric_limits<size_t>::max();

    // Round up to minimum block size
    size_t adjustedSize = (std::max)(size, m_minBlockSize);

    uint32_t fl = 0, sl = 0;
    MappingSearch(adjustedSize, fl, sl);

    TLSFBlock* block = FindFreeBlock(fl, sl);
    if (!block)
        return std::numeric_limits<size_t>::max();

    RemoveFreeBlock(block);

    // Split if remaining is large enough
    if (block->size >= adjustedSize + m_minBlockSize)
    {
        SplitBlock(block, adjustedSize);
    }

    block->free = false;
    m_usedSize += block->size;
    m_allocationCount++;

    return block->offset;
}

void TLSFAllocator::Free(size_t offset)
{
    auto it = m_offsetToBlock.find(offset);
    if (it == m_offsetToBlock.end()) return;

    TLSFBlock* block = it->second;
    if (block->free) return;

    block->free = true;
    m_usedSize -= block->size;
    m_allocationCount--;

    // Merge with next block if free
    if (block->next && block->next->free)
    {
        TLSFBlock* nextBlock = block->next;
        RemoveFreeBlock(nextBlock);

        block->size += nextBlock->size;
        block->next = nextBlock->next;
        if (nextBlock->next)
            nextBlock->next->prev = block;

        m_offsetToBlock.erase(nextBlock->offset);
        // Remove from storage
        for (auto sit = m_blockStorage.begin(); sit != m_blockStorage.end(); ++sit)
        {
            if (sit->get() == nextBlock)
            {
                m_blockStorage.erase(sit);
                break;
            }
        }
    }

    // Merge with previous block if free
    if (block->prev && block->prev->free)
    {
        TLSFBlock* prevBlock = block->prev;
        RemoveFreeBlock(prevBlock);

        prevBlock->size += block->size;
        prevBlock->next = block->next;
        if (block->next)
            block->next->prev = prevBlock;

        m_offsetToBlock.erase(block->offset);
        // Remove from storage
        for (auto sit = m_blockStorage.begin(); sit != m_blockStorage.end(); ++sit)
        {
            if (sit->get() == block)
            {
                m_blockStorage.erase(sit);
                break;
            }
        }

        block = prevBlock;
    }

    InsertFreeBlock(block);
}

size_t TLSFAllocator::GetLargestFreeBlock() const
{
    size_t largest = 0;
    for (const auto& b : m_blockStorage)
    {
        if (b->free && b->size > largest)
            largest = b->size;
    }
    return largest;
}

std::vector<DefragMove> TLSFAllocator::ComputeDefragPlan() const
{
    std::vector<DefragMove> moves;

    // Collect all allocated blocks sorted by offset
    std::vector<TLSFBlock*> allocated;
    for (const auto& b : m_blockStorage)
    {
        if (!b->free)
            allocated.push_back(b.get());
    }

    std::sort(allocated.begin(), allocated.end(),
        [](const TLSFBlock* a, const TLSFBlock* b) { return a->offset < b->offset; });

    size_t dstOffset = 0;
    for (const auto* blk : allocated)
    {
        if (blk->offset != dstOffset)
        {
            DefragMove move;
            move.srcOffset = blk->offset;
            move.dstOffset = dstOffset;
            move.size = blk->size;
            moves.push_back(move);
        }
        dstOffset += blk->size;
    }

    return moves;
}

void TLSFAllocator::Reset()
{
    m_usedSize = 0;
    m_allocationCount = 0;
    m_flBitmap = 0;
    std::memset(m_slBitmap, 0, sizeof(m_slBitmap));
    std::memset(m_freeList, 0, sizeof(m_freeList));

    m_blockStorage.clear();
    m_offsetToBlock.clear();

    // Recreate the initial free block
    auto block = std::make_unique<TLSFBlock>();
    block->size = m_poolSize;
    block->free = true;
    block->offset = 0;
    block->prev = nullptr;
    block->next = nullptr;
    block->prevFree = nullptr;
    block->nextFree = nullptr;

    TLSFBlock* rawPtr = block.get();
    m_blockStorage.push_back(std::move(block));
    m_offsetToBlock[0] = rawPtr;

    InsertFreeBlock(rawPtr);
}

void TLSFAllocator::MappingInsert(size_t size, uint32_t& fl, uint32_t& sl) const
{
    if (size < m_minBlockSize)
    {
        fl = 0;
        sl = 0;
        return;
    }

    // First-level index: floor(log2(size))
    fl = 0;
    size_t tmp = size;
    while (tmp > 1) { tmp >>= 1; fl++; }

    // Second-level index: extract SL_INDEX_LOG2 bits below the MSB
    if (fl >= SL_INDEX_LOG2)
        sl = static_cast<uint32_t>((size >> (fl - SL_INDEX_LOG2)) & (SL_INDEX_COUNT - 1));
    else
        sl = 0;

    // Clamp
    if (fl >= FL_INDEX_COUNT) fl = FL_INDEX_COUNT - 1;
}

void TLSFAllocator::MappingSearch(size_t size, uint32_t& fl, uint32_t& sl) const
{
    // Round up to ensure we find a block >= size
    if (size >= m_minBlockSize)
    {
        size_t roundUp = size;
        // Round up to the next SL boundary for search
        uint32_t tempFl = 0;
        size_t tmp = size;
        while (tmp > 1) { tmp >>= 1; tempFl++; }

        if (tempFl >= SL_INDEX_LOG2)
        {
            size_t slSize = static_cast<size_t>(1) << (tempFl - SL_INDEX_LOG2);
            roundUp = (size + slSize - 1) & ~(slSize - 1);
        }

        MappingInsert(roundUp, fl, sl);
    }
    else
    {
        MappingInsert(m_minBlockSize, fl, sl);
    }
}

TLSFBlock* TLSFAllocator::FindFreeBlock(uint32_t fl, uint32_t sl)
{
    // Search in the current SL list first
    if (fl < FL_INDEX_COUNT && sl < SL_INDEX_COUNT)
    {
        // Check from sl upward in the current fl
        uint32_t slMap = m_slBitmap[fl] & (~0u << sl);
        if (slMap != 0)
        {
            // Find first set bit
            uint32_t foundSl = 0;
            uint32_t tmpMap = slMap;
            while ((tmpMap & 1) == 0) { tmpMap >>= 1; foundSl++; }

            if (m_freeList[fl][foundSl])
                return m_freeList[fl][foundSl];
        }

        // Search in higher FL levels
        uint32_t flMap = m_flBitmap & (~0u << (fl + 1));
        if (flMap != 0)
        {
            uint32_t foundFl = 0;
            uint32_t tmpMap = flMap;
            while ((tmpMap & 1) == 0) { tmpMap >>= 1; foundFl++; }

            // Get first available SL in this FL
            uint32_t slMap2 = m_slBitmap[foundFl];
            if (slMap2 != 0)
            {
                uint32_t foundSl = 0;
                while ((slMap2 & 1) == 0) { slMap2 >>= 1; foundSl++; }

                if (m_freeList[foundFl][foundSl])
                    return m_freeList[foundFl][foundSl];
            }
        }
    }

    return nullptr;
}

void TLSFAllocator::InsertFreeBlock(TLSFBlock* block)
{
    uint32_t fl = 0, sl = 0;
    MappingInsert(block->size, fl, sl);

    if (fl >= FL_INDEX_COUNT) fl = FL_INDEX_COUNT - 1;
    if (sl >= SL_INDEX_COUNT) sl = SL_INDEX_COUNT - 1;

    block->prevFree = nullptr;
    block->nextFree = m_freeList[fl][sl];
    if (m_freeList[fl][sl])
        m_freeList[fl][sl]->prevFree = block;
    m_freeList[fl][sl] = block;

    m_flBitmap |= (1u << fl);
    m_slBitmap[fl] |= (1u << sl);
}

void TLSFAllocator::RemoveFreeBlock(TLSFBlock* block)
{
    uint32_t fl = 0, sl = 0;
    MappingInsert(block->size, fl, sl);

    if (fl >= FL_INDEX_COUNT) fl = FL_INDEX_COUNT - 1;
    if (sl >= SL_INDEX_COUNT) sl = SL_INDEX_COUNT - 1;

    if (block->prevFree)
        block->prevFree->nextFree = block->nextFree;
    else
        m_freeList[fl][sl] = block->nextFree;

    if (block->nextFree)
        block->nextFree->prevFree = block->prevFree;

    block->prevFree = nullptr;
    block->nextFree = nullptr;

    // Update bitmaps if the list is now empty
    if (!m_freeList[fl][sl])
    {
        m_slBitmap[fl] &= ~(1u << sl);
        if (m_slBitmap[fl] == 0)
            m_flBitmap &= ~(1u << fl);
    }
}

TLSFBlock* TLSFAllocator::SplitBlock(TLSFBlock* block, size_t size)
{
    size_t remaining = block->size - size;

    auto newBlock = std::make_unique<TLSFBlock>();
    newBlock->size = remaining;
    newBlock->free = true;
    newBlock->offset = block->offset + size;
    newBlock->prev = block;
    newBlock->next = block->next;

    if (block->next)
        block->next->prev = newBlock.get();

    block->size = size;
    block->next = newBlock.get();

    TLSFBlock* rawPtr = newBlock.get();
    m_offsetToBlock[rawPtr->offset] = rawPtr;
    m_blockStorage.push_back(std::move(newBlock));

    InsertFreeBlock(rawPtr);

    return rawPtr;
}

} // namespace gx
