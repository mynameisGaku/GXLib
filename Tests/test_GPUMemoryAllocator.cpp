/// @file test_GPUMemoryAllocator.cpp
/// @brief GPUMemoryAllocatorのテスト — 統計情報、一時アロケーション、デフォルトステート（GPUデバイスなし）

#include <gtest/gtest.h>
#include "Graphics/Resource/GPUMemoryAllocator.h"

using namespace gx;

// ============================================================================
// デフォルトステート
// ============================================================================

TEST(GPUMemoryAllocatorTest, DefaultStats)
{
    GPUMemoryAllocator alloc;
    GPUMemoryStats stats = alloc.GetStats();
    EXPECT_EQ(stats.totalAllocated, 0u);
    EXPECT_EQ(stats.peakAllocated, 0u);
    EXPECT_EQ(stats.totalHeapSize, 0u);
    EXPECT_EQ(stats.allocationCount, 0u);
    EXPECT_EQ(stats.heapCount, 0u);
}

TEST(GPUMemoryAllocatorTest, InitializeWithoutDevice)
{
    GPUMemoryAllocator alloc;
    // nullptrデバイスでの初期化は正常に失敗するはず
    bool result = alloc.Initialize(nullptr);
    EXPECT_FALSE(result);
}

TEST(GPUMemoryAllocatorTest, AllocateWithoutInit)
{
    GPUMemoryAllocator alloc;
    GPUAllocation result = alloc.Allocate(1024);
    EXPECT_FALSE(result.valid);
}

TEST(GPUMemoryAllocatorTest, TransientWithoutInit)
{
    GPUMemoryAllocator alloc;
    TransientAllocation result = alloc.AllocateTransient(256);
    EXPECT_FALSE(result.valid);
}

TEST(GPUMemoryAllocatorTest, ShutdownSafe)
{
    GPUMemoryAllocator alloc;
    // 初期化なしのShutdownでクラッシュしないこと
    alloc.Shutdown();
    GPUMemoryStats stats = alloc.GetStats();
    EXPECT_EQ(stats.totalAllocated, 0u);
}

TEST(GPUMemoryAllocatorTest, FreeInvalidAllocation)
{
    GPUMemoryAllocator alloc;
    GPUAllocation invalid;
    invalid.valid = false;
    // 無効なアロケーションのFreeでクラッシュしないこと
    alloc.Free(invalid);
}
