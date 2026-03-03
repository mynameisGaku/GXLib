/// @file test_ConcurrentObjectPool.cpp
/// @brief ConcurrentObjectPool スレッドセーフテスト

#include <gtest/gtest.h>
#include "Core/ObjectPool.h"
#include <thread>
#include <atomic>
#include <vector>
using namespace gx;

// ============================================================================
// Basic single-threaded tests (same as ObjectPool)
// ============================================================================

TEST(ConcurrentObjectPoolTest, DefaultConstruction)
{
    ConcurrentObjectPool<int> pool;
    EXPECT_EQ(pool.GetActiveCount(), 0u);
    EXPECT_EQ(pool.GetCapacity(), 64u);
    EXPECT_EQ(pool.GetFreeCount(), 64u);
}

TEST(ConcurrentObjectPoolTest, CustomCapacity)
{
    ConcurrentObjectPool<int> pool(128);
    EXPECT_EQ(pool.GetCapacity(), 128u);
    EXPECT_EQ(pool.GetFreeCount(), 128u);
}

TEST(ConcurrentObjectPoolTest, GetReturnsNonNull)
{
    ConcurrentObjectPool<int> pool(4);
    int* obj = pool.Get();
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 1u);
    EXPECT_EQ(pool.GetFreeCount(), 3u);
}

TEST(ConcurrentObjectPoolTest, GetInitializesValue)
{
    ConcurrentObjectPool<int> pool(4);
    int* obj = pool.Get();
    EXPECT_EQ(*obj, 0);
}

TEST(ConcurrentObjectPoolTest, Release)
{
    ConcurrentObjectPool<int> pool(4);
    int* obj = pool.Get();
    pool.Release(obj);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
    EXPECT_EQ(pool.GetFreeCount(), 4u);
}

TEST(ConcurrentObjectPoolTest, ReleaseNull)
{
    ConcurrentObjectPool<int> pool(4);
    pool.Release(nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, DoubleRelease)
{
    ConcurrentObjectPool<int> pool(4);
    int* obj = pool.Get();
    pool.Release(obj);
    pool.Release(obj); // second release should be a no-op
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, MultipleGetRelease)
{
    ConcurrentObjectPool<int> pool(4);
    int* a = pool.Get();
    int* b = pool.Get();
    int* c = pool.Get();
    EXPECT_EQ(pool.GetActiveCount(), 3u);

    pool.Release(b);
    EXPECT_EQ(pool.GetActiveCount(), 2u);

    int* d = pool.Get();
    EXPECT_NE(d, nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 3u);

    pool.Release(a);
    pool.Release(c);
    pool.Release(d);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, AutoExpand)
{
    ConcurrentObjectPool<int> pool(2);
    int* a = pool.Get();
    int* b = pool.Get();
    EXPECT_EQ(pool.GetCapacity(), 2u);

    // Auto-expand triggers
    int* c = pool.Get();
    EXPECT_NE(c, nullptr);
    EXPECT_GE(pool.GetCapacity(), 3u);
    EXPECT_EQ(pool.GetActiveCount(), 3u);

    pool.Release(a);
    pool.Release(b);
    pool.Release(c);
}

TEST(ConcurrentObjectPoolTest, NoAutoExpand)
{
    ConcurrentObjectPool<int> pool(2, false);
    int* a = pool.Get();
    int* b = pool.Get();
    int* c = pool.Get();
    EXPECT_EQ(a != nullptr, true);
    EXPECT_EQ(b != nullptr, true);
    EXPECT_EQ(c, nullptr);
    EXPECT_EQ(pool.GetActiveCount(), 2u);

    pool.Release(a);
    pool.Release(b);
}

TEST(ConcurrentObjectPoolTest, ReleaseAll)
{
    ConcurrentObjectPool<int> pool(8);
    pool.Get();
    pool.Get();
    pool.Get();
    EXPECT_EQ(pool.GetActiveCount(), 3u);

    pool.ReleaseAll();
    EXPECT_EQ(pool.GetActiveCount(), 0u);
    EXPECT_EQ(pool.GetFreeCount(), 8u);
}

TEST(ConcurrentObjectPoolTest, ForEachActive)
{
    ConcurrentObjectPool<int> pool(8);
    int* a = pool.Get(); *a = 10;
    int* b = pool.Get(); *b = 20;
    int* c = pool.Get(); *c = 30;
    pool.Release(b);

    int sum = 0;
    pool.ForEachActive([&sum](int& val) { sum += val; });
    EXPECT_EQ(sum, 40); // 10 + 30
}

TEST(ConcurrentObjectPoolTest, ReleaseIf)
{
    ConcurrentObjectPool<int> pool(8);
    int* a = pool.Get(); *a = 1;
    int* b = pool.Get(); *b = 2;
    int* c = pool.Get(); *c = 3;
    int* d = pool.Get(); *d = 4;

    pool.ReleaseIf([](const int& val) { return val % 2 == 0; });
    EXPECT_EQ(pool.GetActiveCount(), 2u);

    int sum = 0;
    pool.ForEachActive([&sum](int& val) { sum += val; });
    EXPECT_EQ(sum, 4); // 1 + 3
}

// ============================================================================
// Struct type test
// ============================================================================

namespace {
struct Particle
{
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    bool alive = false;
};
} // anonymous namespace

TEST(ConcurrentObjectPoolTest, StructType)
{
    ConcurrentObjectPool<Particle> pool(16);
    Particle* p = pool.Get();
    EXPECT_NE(p, nullptr);
    p->x = 1.0f;
    p->y = 2.0f;
    p->alive = true;
    EXPECT_FLOAT_EQ(p->x, 1.0f);

    pool.Release(p);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

// ============================================================================
// Multi-threaded tests
// ============================================================================

TEST(ConcurrentObjectPoolTest, ConcurrentGetRelease)
{
    // 4 threads, each doing 1000 Get/Release cycles
    constexpr int kThreadCount = 4;
    constexpr int kCyclesPerThread = 1000;

    ConcurrentObjectPool<int> pool(kThreadCount * 16);
    std::atomic<int> successCount{0};

    auto worker = [&pool, &successCount]() {
        for (int i = 0; i < kCyclesPerThread; ++i)
        {
            int* obj = pool.Get();
            if (obj)
            {
                *obj = i;
                successCount.fetch_add(1, std::memory_order_relaxed);
                // Small amount of work to increase contention
                volatile int dummy = *obj + 1;
                (void)dummy;
                pool.Release(obj);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int i = 0; i < kThreadCount; ++i)
        threads.emplace_back(worker);

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(successCount.load(), kThreadCount * kCyclesPerThread);
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, ConcurrentMixedOperations)
{
    // Multiple threads doing Get, some hold objects, then release
    constexpr int kThreadCount = 4;
    constexpr int kOpsPerThread = 500;

    ConcurrentObjectPool<int> pool(kThreadCount * kOpsPerThread);

    auto worker = [&pool](int threadId) {
        std::vector<int*> held;
        held.reserve(kOpsPerThread);

        for (int i = 0; i < kOpsPerThread; ++i)
        {
            int* obj = pool.Get();
            if (obj)
            {
                *obj = threadId * 10000 + i;
                held.push_back(obj);
            }
        }

        // Release all held objects
        for (int* obj : held)
            pool.Release(obj);
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int i = 0; i < kThreadCount; ++i)
        threads.emplace_back(worker, i);

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, ConcurrentGetReleaseWithActiveCount)
{
    // Verify GetActiveCount is consistent under concurrent access
    constexpr int kThreadCount = 4;
    constexpr int kOpsPerThread = 200;

    ConcurrentObjectPool<int> pool(kThreadCount * kOpsPerThread);
    std::atomic<bool> start{false};

    auto worker = [&pool, &start]() {
        while (!start.load(std::memory_order_acquire)) {} // spin until start

        std::vector<int*> held;
        for (int i = 0; i < kOpsPerThread; ++i)
        {
            int* obj = pool.Get();
            if (obj) held.push_back(obj);
        }

        // Active count should never be negative
        EXPECT_GE(pool.GetActiveCount(), held.size());

        for (int* obj : held)
            pool.Release(obj);
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int i = 0; i < kThreadCount; ++i)
        threads.emplace_back(worker);

    start.store(true, std::memory_order_release);

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, ConcurrentForEachActive)
{
    // One thread iterates, others Get/Release
    constexpr int kInitial = 256;
    ConcurrentObjectPool<int> pool(kInitial);

    // Pre-populate some active objects
    std::vector<int*> initial;
    for (int i = 0; i < 32; ++i)
    {
        int* p = pool.Get();
        *p = i;
        initial.push_back(p);
    }

    std::atomic<bool> done{false};

    // Writer thread: continuously Get/Release
    auto writer = [&pool, &done]() {
        for (int i = 0; i < 500; ++i)
        {
            int* obj = pool.Get();
            if (obj)
            {
                *obj = i + 100;
                pool.Release(obj);
            }
        }
        done.store(true, std::memory_order_release);
    };

    // Reader thread: iterate active objects
    auto reader = [&pool, &done]() {
        while (!done.load(std::memory_order_acquire))
        {
            int count = 0;
            pool.ForEachActive([&count](int&) { count++; });
            // count should be >= 0 and reasonable
            EXPECT_GE(count, 0);
        }
    };

    std::thread t1(writer);
    std::thread t2(reader);

    t1.join();
    t2.join();

    // Clean up initial objects
    for (int* p : initial)
        pool.Release(p);

    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, ConcurrentReleaseIf)
{
    ConcurrentObjectPool<int> pool(256);

    // Pre-populate
    for (int i = 0; i < 100; ++i)
    {
        int* p = pool.Get();
        *p = i;
    }
    EXPECT_EQ(pool.GetActiveCount(), 100u);

    std::atomic<bool> start{false};

    // Thread 1: ReleaseIf even numbers
    auto t1Func = [&pool, &start]() {
        while (!start.load(std::memory_order_acquire)) {}
        pool.ReleaseIf([](const int& val) { return val % 2 == 0; });
    };

    // Thread 2: Get new objects while ReleaseIf is running
    auto t2Func = [&pool, &start]() {
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < 20; ++i)
        {
            int* p = pool.Get();
            if (p) *p = 1000 + i; // odd numbers, won't be released
        }
    };

    std::thread t1(t1Func);
    std::thread t2(t2Func);

    start.store(true, std::memory_order_release);

    t1.join();
    t2.join();

    // All remaining objects should be odd (or >= 1000)
    pool.ForEachActive([](int& val) {
        EXPECT_TRUE(val % 2 != 0 || val >= 1000);
    });

    pool.ReleaseAll();
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}

TEST(ConcurrentObjectPoolTest, StressTest)
{
    // High contention stress test.
    // Pool must be large enough to avoid reallocation, which would invalidate
    // pointers held by other threads (gx::Vector::resize moves storage).
    constexpr int kThreadCount = 8;
    constexpr int kOpsPerThread = 2000;
    // Worst case: each thread holds ~2/3 of its ops simultaneously
    constexpr int kPoolSize = kThreadCount * kOpsPerThread;

    ConcurrentObjectPool<int> pool(kPoolSize, false); // no auto-expand
    std::atomic<int> totalGets{0};
    std::atomic<int> totalReleases{0};

    auto worker = [&pool, &totalGets, &totalReleases]() {
        std::vector<int*> held;
        for (int i = 0; i < kOpsPerThread; ++i)
        {
            // Alternate between get and release
            if (held.empty() || (i % 3 != 0))
            {
                int* obj = pool.Get();
                if (obj)
                {
                    *obj = i;
                    held.push_back(obj);
                    totalGets.fetch_add(1, std::memory_order_relaxed);
                }
            }
            else
            {
                pool.Release(held.back());
                held.pop_back();
                totalReleases.fetch_add(1, std::memory_order_relaxed);
            }
        }
        // Release remaining
        for (int* obj : held)
        {
            pool.Release(obj);
            totalReleases.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int i = 0; i < kThreadCount; ++i)
        threads.emplace_back(worker);

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(totalGets.load(), totalReleases.load());
    EXPECT_EQ(pool.GetActiveCount(), 0u);
}
