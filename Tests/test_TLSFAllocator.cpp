/// @file test_TLSFAllocator.cpp
#include <gtest/gtest.h>
#include "Core/TLSFAllocator.h"
using namespace gx;

TEST(TLSFAllocatorTest, Initialize) {
    TLSFAllocator alloc;
    EXPECT_TRUE(alloc.Initialize(1024));
    EXPECT_EQ(alloc.GetPoolSize(), 1024u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
}

TEST(TLSFAllocatorTest, Allocate) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    size_t offset = alloc.Allocate(64);
    EXPECT_NE(offset, std::numeric_limits<size_t>::max());
    EXPECT_EQ(alloc.GetAllocationCount(), 1u);
    EXPECT_GE(alloc.GetUsedSize(), 64u);
}

TEST(TLSFAllocatorTest, Free) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    size_t offset = alloc.Allocate(64);
    alloc.Free(offset);
    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
}

TEST(TLSFAllocatorTest, AllocateAfterFreeCoalescing) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    size_t a = alloc.Allocate(256);
    size_t b = alloc.Allocate(256);
    alloc.Free(a);
    alloc.Free(b);
    // After coalescing, should be able to allocate a large block again
    size_t c = alloc.Allocate(512);
    EXPECT_NE(c, std::numeric_limits<size_t>::max());
}

TEST(TLSFAllocatorTest, MultipleAllocations) {
    TLSFAllocator alloc;
    alloc.Initialize(4096);
    gx::Vector<size_t> offsets;
    for (int i = 0; i < 10; i++) {
        size_t off = alloc.Allocate(64);
        EXPECT_NE(off, std::numeric_limits<size_t>::max());
        offsets.push_back(off);
    }
    EXPECT_EQ(alloc.GetAllocationCount(), 10u);

    // All offsets should be unique
    std::sort(offsets.begin(), offsets.end());
    auto last = std::unique(offsets.begin(), offsets.end());
    EXPECT_EQ(last, offsets.end());
}

TEST(TLSFAllocatorTest, Fragmentation) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    // Allocate then free alternating blocks to create fragmentation
    size_t a = alloc.Allocate(128);
    size_t b = alloc.Allocate(128);
    size_t c = alloc.Allocate(128);
    alloc.Free(b); // Free middle block
    // Largest free block should be at least 128
    EXPECT_GE(alloc.GetLargestFreeBlock(), 128u);
    (void)a; (void)c;
}

TEST(TLSFAllocatorTest, DefragPlan) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    size_t a = alloc.Allocate(128);
    size_t b = alloc.Allocate(128);
    size_t c = alloc.Allocate(128);
    alloc.Free(b); // Create a gap
    auto moves = alloc.ComputeDefragPlan();
    // Block c should need to move to fill gap left by b
    EXPECT_GE(moves.size(), 1u);
    (void)a; (void)c;
}

TEST(TLSFAllocatorTest, Reset) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    alloc.Allocate(128);
    alloc.Allocate(256);
    alloc.Reset();
    EXPECT_EQ(alloc.GetAllocationCount(), 0u);
    EXPECT_EQ(alloc.GetUsedSize(), 0u);
    EXPECT_EQ(alloc.GetLargestFreeBlock(), 1024u);
}

TEST(TLSFAllocatorTest, LargestFreeBlock) {
    TLSFAllocator alloc;
    alloc.Initialize(1024);
    EXPECT_EQ(alloc.GetLargestFreeBlock(), 1024u);
    alloc.Allocate(512);
    EXPECT_LE(alloc.GetLargestFreeBlock(), 512u);
}

TEST(TLSFAllocatorTest, PoolMetrics) {
    TLSFAllocator alloc;
    alloc.Initialize(2048);
    EXPECT_EQ(alloc.GetPoolSize(), 2048u);
    EXPECT_EQ(alloc.GetFreeSize(), 2048u);
    alloc.Allocate(100);
    EXPECT_GT(alloc.GetUsedSize(), 0u);
    EXPECT_EQ(alloc.GetFreeSize(), alloc.GetPoolSize() - alloc.GetUsedSize());
}

TEST(TLSFAllocatorTest, StressTest) {
    TLSFAllocator alloc;
    alloc.Initialize(65536, 32);
    gx::Vector<size_t> offsets;

    // Allocate many small blocks
    for (int i = 0; i < 100; i++) {
        size_t off = alloc.Allocate(32 + (i % 5) * 32);
        if (off != std::numeric_limits<size_t>::max())
            offsets.push_back(off);
    }

    // Free half
    for (size_t i = 0; i < offsets.size(); i += 2) {
        alloc.Free(offsets[i]);
    }

    // Allocate some more
    for (int i = 0; i < 20; i++) {
        alloc.Allocate(64);
    }

    EXPECT_GT(alloc.GetAllocationCount(), 0u);
}
