/// @file test_BuddyAllocator.cpp
#include <gtest/gtest.h>
#include "Core/BuddyAllocator.h"
using namespace gx;

TEST(BuddyAllocatorTest, Initialize) {
    BuddyAllocator alloc;
    EXPECT_TRUE(alloc.Initialize(1024, 16));
    EXPECT_EQ(alloc.GetTotalSize(), 1024u);
    EXPECT_EQ(alloc.GetMinBlockSize(), 16u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
}

TEST(BuddyAllocatorTest, Allocate) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    size_t offset = alloc.Allocate(64);
    EXPECT_NE(offset, std::numeric_limits<size_t>::max());
    EXPECT_EQ(alloc.GetAllocationCount(), 1u);
    EXPECT_EQ(alloc.GetUsedSize(), 64u);
}

TEST(BuddyAllocatorTest, Free) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    size_t offset = alloc.Allocate(64);
    alloc.Free(offset);
    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
}

TEST(BuddyAllocatorTest, BuddyMerge) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    // Allocate two 512-byte blocks (splitting the 1024 pool)
    size_t a = alloc.Allocate(512);
    size_t b = alloc.Allocate(512);
    EXPECT_EQ(alloc.GetFreeSize(), 0u);
    // Free both - they should merge back into a single 1024 block
    alloc.Free(a);
    alloc.Free(b);
    EXPECT_EQ(alloc.GetFreeSize(), 1024u);
    // Should be able to allocate the full 1024 again
    size_t c = alloc.Allocate(1024);
    EXPECT_NE(c, std::numeric_limits<size_t>::max());
}

TEST(BuddyAllocatorTest, MultipleAllocations) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    std::vector<size_t> offsets;
    // Allocate several blocks
    for (int i = 0; i < 8; i++) {
        size_t off = alloc.Allocate(16);
        EXPECT_NE(off, std::numeric_limits<size_t>::max());
        offsets.push_back(off);
    }
    EXPECT_EQ(alloc.GetAllocationCount(), 8u);

    // All offsets should be unique
    std::sort(offsets.begin(), offsets.end());
    auto last = std::unique(offsets.begin(), offsets.end());
    EXPECT_EQ(last, offsets.end());
}

TEST(BuddyAllocatorTest, PowerOf2Rounding) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    // Allocating 17 bytes should round up to 32
    size_t offset = alloc.Allocate(17);
    EXPECT_NE(offset, std::numeric_limits<size_t>::max());
    EXPECT_EQ(alloc.GetUsedSize(), 32u);
}

TEST(BuddyAllocatorTest, MaxOrder) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    // maxOrder = log2(1024/16) = log2(64) = 6
    EXPECT_EQ(alloc.GetMaxOrder(), 6u);
}

TEST(BuddyAllocatorTest, MinBlockSize) {
    BuddyAllocator alloc;
    alloc.Initialize(256, 32);
    EXPECT_EQ(alloc.GetMinBlockSize(), 32u);
    // Allocating less than min should still use min
    size_t offset = alloc.Allocate(1);
    EXPECT_NE(offset, std::numeric_limits<size_t>::max());
    EXPECT_EQ(alloc.GetUsedSize(), 32u);
}

TEST(BuddyAllocatorTest, Reset) {
    BuddyAllocator alloc;
    alloc.Initialize(1024, 16);
    alloc.Allocate(64);
    alloc.Allocate(128);
    alloc.Reset();
    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
    EXPECT_EQ(alloc.GetFreeSize(), 1024u);
    // Should be able to allocate the full pool
    size_t off = alloc.Allocate(1024);
    EXPECT_NE(off, std::numeric_limits<size_t>::max());
}

TEST(BuddyAllocatorTest, StressTest) {
    BuddyAllocator alloc;
    alloc.Initialize(65536, 16);
    std::vector<size_t> offsets;

    // Allocate many blocks
    for (int i = 0; i < 100; i++) {
        size_t off = alloc.Allocate(16 + (i % 8) * 16);
        if (off != std::numeric_limits<size_t>::max())
            offsets.push_back(off);
    }

    // Free all in reverse order
    for (auto rit = offsets.rbegin(); rit != offsets.rend(); ++rit) {
        alloc.Free(*rit);
    }

    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
    // After freeing all, pool should fully merge back
    EXPECT_EQ(alloc.GetFreeSize(), 65536u);
}
