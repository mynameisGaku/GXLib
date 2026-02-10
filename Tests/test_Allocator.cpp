#include "pch.h"
#include <gtest/gtest.h>
#include "Core/PoolAllocator.h"
#include "Core/FrameAllocator.h"

// ============================================================================
// PoolAllocator のテスト（固定サイズのプール）
// ============================================================================

struct TestObj
{
    int   a;
    float b;
    TestObj() : a(0), b(0.0f) {}
    TestObj(int x, float y) : a(x), b(y) {}
};

TEST(PoolAllocatorTest, AllocateAndFree)
{
    GX::PoolAllocator<TestObj, 4> pool;
    EXPECT_EQ(pool.GetActiveCount(), 0u);

    void* p1 = pool.Allocate();
    void* p2 = pool.Allocate();
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);
    EXPECT_EQ(pool.GetActiveCount(), 2u);

    pool.Free(p1);
    EXPECT_EQ(pool.GetActiveCount(), 1u);

    pool.Free(p2);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(PoolAllocatorTest, NewAndDelete)
{
    GX::PoolAllocator<TestObj, 4> pool;
    TestObj* obj = pool.New(42, 3.14f);
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(obj->a, 42);
    EXPECT_NEAR(obj->b, 3.14f, 0.001f);
    EXPECT_EQ(pool.GetActiveCount(), 1u);

    pool.Delete(obj);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(PoolAllocatorTest, BlockGrowth)
{
    GX::PoolAllocator<TestObj, 4> pool;
    EXPECT_EQ(pool.GetCapacity(), 0u);

    // 4個確保（最初のブロックが埋まる）
    void* ptrs[8];
    for (int i = 0; i < 4; ++i)
        ptrs[i] = pool.Allocate();
    EXPECT_EQ(pool.GetCapacity(), 4u);
    EXPECT_EQ(pool.GetActiveCount(), 4u);

    // 5個目で次のブロックが作られる
    ptrs[4] = pool.Allocate();
    EXPECT_EQ(pool.GetCapacity(), 8u);
    EXPECT_EQ(pool.GetActiveCount(), 5u);

    // 全部解放
    for (int i = 0; i < 5; ++i)
        pool.Free(ptrs[i]);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(PoolAllocatorTest, ReuseFreedSlots)
{
    GX::PoolAllocator<TestObj, 4> pool;
    void* p1 = pool.Allocate();
    pool.Free(p1);

    // 次の確保で解放済みスロットが再利用される
    void* p2 = pool.Allocate();
    EXPECT_EQ(p1, p2);
}

// ============================================================================
// FrameAllocator のテスト（フレーム単位の線形確保）
// ============================================================================

TEST(FrameAllocatorTest, BasicAllocate)
{
    GX::FrameAllocator alloc(1024);
    EXPECT_EQ(alloc.GetUsedBytes(), 0u);
    EXPECT_EQ(alloc.GetCapacity(), 1024u);

    void* p = alloc.Allocate(64);
    EXPECT_NE(p, nullptr);
    EXPECT_GE(alloc.GetUsedBytes(), 64u);
}

TEST(FrameAllocatorTest, TypedAllocate)
{
    GX::FrameAllocator alloc(1024);
    float* f = alloc.Allocate<float>(10);
    EXPECT_NE(f, nullptr);

    // 書き込み→読み戻しを確認
    for (int i = 0; i < 10; ++i)
        f[i] = static_cast<float>(i);
    for (int i = 0; i < 10; ++i)
        EXPECT_NEAR(f[i], static_cast<float>(i), 0.001f);
}

TEST(FrameAllocatorTest, Reset)
{
    GX::FrameAllocator alloc(1024);
    alloc.Allocate(512);
    EXPECT_GE(alloc.GetUsedBytes(), 512u);

    alloc.Reset();
    EXPECT_EQ(alloc.GetUsedBytes(), 0u);
    EXPECT_EQ(alloc.GetRemainingBytes(), 1024u);
}

TEST(FrameAllocatorTest, Alignment)
{
    GX::FrameAllocator alloc(1024);
    // 1バイト確保後に256バイト境界で確保
    alloc.Allocate(1, 1);
    void* p = alloc.Allocate(32, 256);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % 256, 0u);
}

TEST(FrameAllocatorTest, CapacityExhausted)
{
    GX::FrameAllocator alloc(64);
    void* p1 = alloc.Allocate(32);
    EXPECT_NE(p1, nullptr);

    void* p2 = alloc.Allocate(64);  // exceeds remaining
    EXPECT_EQ(p2, nullptr);
}

TEST(FrameAllocatorTest, SequentialAddresses)
{
    GX::FrameAllocator alloc(1024);
    void* p1 = alloc.Allocate(16, 16);
    void* p2 = alloc.Allocate(16, 16);
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);

    // 線形確保なのでp2はp1の後ろに並ぶ
    EXPECT_GT(reinterpret_cast<uintptr_t>(p2), reinterpret_cast<uintptr_t>(p1));
}
