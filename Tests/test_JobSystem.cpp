/// @file test_JobSystem.cpp
/// @brief Phase 5: JobSystemシングルトン、ワーカースレッド、Submit、Wait、ParallelFor、依存関係

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/JobSystem.h"
#include <atomic>

using namespace gx;

// ============================================================================
// JobHandle
// ============================================================================

TEST(JobHandleTest, DefaultInvalid)
{
    JobHandle handle;
    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.id, 0u);
}

// ============================================================================
// JobPriority列挙型
// ============================================================================

TEST(JobPriorityTest, EnumValues)
{
    EXPECT_NE(static_cast<int>(JobPriority::Low), static_cast<int>(JobPriority::Normal));
    EXPECT_NE(static_cast<int>(JobPriority::Normal), static_cast<int>(JobPriority::High));
    EXPECT_LT(static_cast<int>(JobPriority::Low), static_cast<int>(JobPriority::High));
}

// ============================================================================
// JobSystemフィクスチャ（テストごとに初期化/シャットダウン）
// ============================================================================

class JobSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 前のテストから残った状態をシャットダウン
        if (JobSystem::Instance().IsInitialized())
        {
            JobSystem::Instance().WaitAll();
            JobSystem::Instance().Shutdown();
        }
    }

    void TearDown() override
    {
        if (JobSystem::Instance().IsInitialized())
        {
            JobSystem::Instance().WaitAll();
            JobSystem::Instance().Shutdown();
        }
    }
};

// ============================================================================
// シングルトンと初期化
// ============================================================================

TEST_F(JobSystemTest, Singleton)
{
    auto& instance1 = JobSystem::Instance();
    auto& instance2 = JobSystem::Instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(JobSystemTest, InitializeDefault)
{
    EXPECT_TRUE(JobSystem::Instance().Initialize());
    EXPECT_TRUE(JobSystem::Instance().IsInitialized());
    EXPECT_GT(JobSystem::Instance().GetWorkerCount(), 0u);

    JobSystem::Instance().Shutdown();
    EXPECT_FALSE(JobSystem::Instance().IsInitialized());
}

TEST_F(JobSystemTest, InitializeCustomWorkerCount)
{
    EXPECT_TRUE(JobSystem::Instance().Initialize(2));
    EXPECT_TRUE(JobSystem::Instance().IsInitialized());
    EXPECT_EQ(JobSystem::Instance().GetWorkerCount(), 2u);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// SubmitとWait
// ============================================================================

TEST_F(JobSystemTest, SubmitAndWait)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    std::atomic<int> counter{ 0 };
    auto handle = JobSystem::Instance().Submit([&]() {
        counter.fetch_add(1);
    });

    EXPECT_TRUE(handle.IsValid());
    JobSystem::Instance().Wait(handle);
    EXPECT_EQ(counter.load(), 1);

    JobSystem::Instance().Shutdown();
}

TEST_F(JobSystemTest, SubmitMultipleJobs)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    std::atomic<int> counter{ 0 };
    gx::Vector<JobHandle> handles;

    for (int i = 0; i < 10; ++i)
    {
        handles.push_back(JobSystem::Instance().Submit([&]() {
            counter.fetch_add(1);
        }));
    }

    for (auto& h : handles)
    {
        JobSystem::Instance().Wait(h);
    }

    EXPECT_EQ(counter.load(), 10);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// IsComplete
// ============================================================================

TEST_F(JobSystemTest, IsComplete)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    std::atomic<bool> started{ false };
    std::atomic<bool> canFinish{ false };

    auto handle = JobSystem::Instance().Submit([&]() {
        started.store(true);
        while (!canFinish.load()) { std::this_thread::yield(); }
    });

    // ジョブの開始を待つ
    while (!started.load()) { std::this_thread::yield(); }

    // ジョブは実行中で未完了
    EXPECT_FALSE(JobSystem::Instance().IsComplete(handle));

    // 完了させる
    canFinish.store(true);
    JobSystem::Instance().Wait(handle);

    EXPECT_TRUE(JobSystem::Instance().IsComplete(handle));

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// WaitAll
// ============================================================================

TEST_F(JobSystemTest, WaitAll)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    std::atomic<int> counter{ 0 };

    for (int i = 0; i < 20; ++i)
    {
        JobSystem::Instance().Submit([&]() {
            counter.fetch_add(1);
        });
    }

    JobSystem::Instance().WaitAll();
    EXPECT_EQ(counter.load(), 20);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// ParallelFor
// ============================================================================

TEST_F(JobSystemTest, ParallelFor)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    const uint32_t count = 1000;
    gx::Vector<int> data(count, 0);

    JobSystem::Instance().ParallelFor(count, [&](uint32_t index) {
        data[index] = static_cast<int>(index * 2);
    });

    for (uint32_t i = 0; i < count; ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(i * 2));
    }

    JobSystem::Instance().Shutdown();
}

TEST_F(JobSystemTest, ParallelForWithCounter)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    std::atomic<int> sum{ 0 };
    const uint32_t count = 100;

    JobSystem::Instance().ParallelFor(count, [&](uint32_t index) {
        sum.fetch_add(1);
    });

    EXPECT_EQ(sum.load(), static_cast<int>(count));

    JobSystem::Instance().Shutdown();
}

TEST_F(JobSystemTest, ParallelForZeroCount)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    std::atomic<int> counter{ 0 };
    JobSystem::Instance().ParallelFor(0, [&](uint32_t) {
        counter.fetch_add(1);
    });

    EXPECT_EQ(counter.load(), 0);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// SubmitAfter（依存関係）
// ============================================================================

TEST_F(JobSystemTest, SubmitAfterDependency)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    std::atomic<int> sequence{ 0 };
    int firstValue = 0;
    int secondValue = 0;

    auto first = JobSystem::Instance().Submit([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        sequence.fetch_add(1);
        firstValue = sequence.load();
    });

    auto second = JobSystem::Instance().SubmitAfter(first, [&]() {
        sequence.fetch_add(1);
        secondValue = sequence.load();
    });

    JobSystem::Instance().Wait(second);

    // 2番目のジョブは1番目の後に実行されるはず
    EXPECT_EQ(firstValue, 1);
    EXPECT_EQ(secondValue, 2);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 優先度
// ============================================================================

TEST_F(JobSystemTest, SubmitWithPriority)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(1)); // 順序テスト用にワーカー1つ

    std::atomic<int> counter{ 0 };

    auto lowHandle = JobSystem::Instance().Submit([&]() {
        counter.fetch_add(1);
    }, JobPriority::Low);

    auto highHandle = JobSystem::Instance().Submit([&]() {
        counter.fetch_add(1);
    }, JobPriority::High);

    JobSystem::Instance().WaitAll();
    EXPECT_EQ(counter.load(), 2);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: 再初期化
// ============================================================================

TEST_F(JobSystemTest, ReInitialize)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));
    EXPECT_TRUE(JobSystem::Instance().IsInitialized());

    JobSystem::Instance().Shutdown();
    EXPECT_FALSE(JobSystem::Instance().IsInitialized());

    // 再初期化が正常に動作するか
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));
    EXPECT_TRUE(JobSystem::Instance().IsInitialized());
    EXPECT_EQ(JobSystem::Instance().GetWorkerCount(), 4u);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: ワーカー数が0のときデフォルト値を使用
// ============================================================================

TEST_F(JobSystemTest, DefaultWorkerCountGreaterThanZero)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(0));
    // ワーカー数0は「自動検出」を意味し、少なくとも1以上のはず
    EXPECT_GT(JobSystem::Instance().GetWorkerCount(), 0u);
    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: 空のジョブ
// ============================================================================

TEST_F(JobSystemTest, EmptyJob)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    // 何もしないジョブでもクラッシュしないこと
    auto handle = JobSystem::Instance().Submit([]() {});
    EXPECT_TRUE(handle.IsValid());
    JobSystem::Instance().Wait(handle);
    EXPECT_TRUE(JobSystem::Instance().IsComplete(handle));

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: 大量ジョブ
// ============================================================================

TEST_F(JobSystemTest, LargeJobCount)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    std::atomic<int> counter{ 0 };
    const int jobCount = 500;

    for (int i = 0; i < jobCount; ++i)
    {
        JobSystem::Instance().Submit([&]() {
            counter.fetch_add(1);
        });
    }

    JobSystem::Instance().WaitAll();
    EXPECT_EQ(counter.load(), jobCount);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: ShutdownSafety — 初期化前のShutdown
// ============================================================================

TEST_F(JobSystemTest, ShutdownWithoutInitialize)
{
    EXPECT_FALSE(JobSystem::Instance().IsInitialized());
    // 初期化前のShutdownがクラッシュしないこと
    JobSystem::Instance().Shutdown();
    EXPECT_FALSE(JobSystem::Instance().IsInitialized());
}

// ============================================================================
// 追加テスト: WaitAll — ジョブなしで呼び出し
// ============================================================================

TEST_F(JobSystemTest, WaitAllWithNoJobs)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    // ジョブを投入せずにWaitAllを呼んでもクラッシュしないこと
    JobSystem::Instance().WaitAll();

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: IsComplete on invalid handle
// ============================================================================

TEST_F(JobSystemTest, IsCompleteInvalidHandle)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    JobHandle invalid;
    // 無効なハンドルに対してIsCompleteがクラッシュしないこと
    bool result = JobSystem::Instance().IsComplete(invalid);
    // 無効なハンドルは完了扱いか未完了扱いかは実装依存だが、クラッシュしなければOK
    (void)result;

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: ParallelFor with single element
// ============================================================================

TEST_F(JobSystemTest, ParallelForSingleElement)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    std::atomic<int> counter{ 0 };
    JobSystem::Instance().ParallelFor(1, [&](uint32_t index) {
        EXPECT_EQ(index, 0u);
        counter.fetch_add(1);
    });

    EXPECT_EQ(counter.load(), 1);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: 依存チェーン（3段階）
// ============================================================================

TEST_F(JobSystemTest, ThreeStageDependencyChain)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    std::atomic<int> sequence{ 0 };
    int v1 = 0, v2 = 0, v3 = 0;

    auto first = JobSystem::Instance().Submit([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        sequence.fetch_add(1);
        v1 = sequence.load();
    });

    auto second = JobSystem::Instance().SubmitAfter(first, [&]() {
        sequence.fetch_add(1);
        v2 = sequence.load();
    });

    auto third = JobSystem::Instance().SubmitAfter(second, [&]() {
        sequence.fetch_add(1);
        v3 = sequence.load();
    });

    JobSystem::Instance().Wait(third);

    EXPECT_EQ(v1, 1);
    EXPECT_EQ(v2, 2);
    EXPECT_EQ(v3, 3);

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: ParallelFor large range
// ============================================================================

TEST_F(JobSystemTest, ParallelForLargeRange)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(4));

    const uint32_t count = 10000;
    gx::Vector<int> data(count, 0);

    JobSystem::Instance().ParallelFor(count, [&](uint32_t index) {
        data[index] = 1;
    });

    int sum = 0;
    for (auto v : data) sum += v;
    EXPECT_EQ(sum, static_cast<int>(count));

    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: Submit returns unique handles
// ============================================================================

TEST_F(JobSystemTest, SubmitReturnsUniqueHandles)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    auto h1 = JobSystem::Instance().Submit([]() {});
    auto h2 = JobSystem::Instance().Submit([]() {});
    auto h3 = JobSystem::Instance().Submit([]() {});

    EXPECT_NE(h1.id, h2.id);
    EXPECT_NE(h2.id, h3.id);
    EXPECT_NE(h1.id, h3.id);

    JobSystem::Instance().WaitAll();
    JobSystem::Instance().Shutdown();
}

// ============================================================================
// 追加テスト: SubmitAfter with Normal priority
// ============================================================================

TEST_F(JobSystemTest, SubmitAfterWithPriority)
{
    ASSERT_TRUE(JobSystem::Instance().Initialize(2));

    std::atomic<int> result{ 0 };

    auto first = JobSystem::Instance().Submit([&]() {
        result.store(10);
    }, JobPriority::Normal);

    auto second = JobSystem::Instance().SubmitAfter(first, [&]() {
        result.fetch_add(5);
    }, JobPriority::High);

    JobSystem::Instance().Wait(second);
    EXPECT_EQ(result.load(), 15);

    JobSystem::Instance().Shutdown();
}
