/// @file test_Coroutine.cpp
/// @brief Phase 3: Coroutine, WaitForSeconds, CoroutineManager テスト

#include <gtest/gtest.h>
#include "Core/Coroutine.h"

using namespace gx;

// ============================================================================
// Coroutine 基本
// ============================================================================

static int g_coroutineStep = 0;

Coroutine SimpleCoroutine()
{
    g_coroutineStep = 1;
    co_await std::suspend_always{};
    g_coroutineStep = 2;
    co_await std::suspend_always{};
    g_coroutineStep = 3;
}

TEST(CoroutineTest, ResumeAndDone)
{
    g_coroutineStep = 0;
    Coroutine co = SimpleCoroutine();

    EXPECT_FALSE(co.IsDone());
    EXPECT_EQ(g_coroutineStep, 0);

    co.Resume(); // step 1
    EXPECT_EQ(g_coroutineStep, 1);
    EXPECT_FALSE(co.IsDone());

    co.Resume(); // step 2
    EXPECT_EQ(g_coroutineStep, 2);
    EXPECT_FALSE(co.IsDone());

    co.Resume(); // step 3 → done
    EXPECT_EQ(g_coroutineStep, 3);
    EXPECT_TRUE(co.IsDone());
}

TEST(CoroutineTest, ResumeAfterDoneReturnsTrue)
{
    Coroutine co = SimpleCoroutine();
    co.Resume();
    co.Resume();
    co.Resume();
    EXPECT_TRUE(co.IsDone());
    EXPECT_TRUE(co.Resume()); // 既に完了済み
}

// ============================================================================
// WaitForSeconds
// ============================================================================

static float g_waitResult = 0.0f;

Coroutine WaitCoroutine()
{
    g_waitResult = 1.0f;
    co_await WaitForSeconds{ 0.5f };
    g_waitResult = 2.0f;
    co_await WaitForSeconds{ 1.0f };
    g_waitResult = 3.0f;
}

TEST(CoroutineTest, WaitForSecondsBasic)
{
    g_waitResult = 0.0f;
    CoroutineManager mgr;

    Coroutine co = WaitCoroutine();
    mgr.Start(std::move(co));

    EXPECT_EQ(mgr.GetActiveCount(), 1u);

    // 初回Resume → ステップ1、0.5秒待機
    mgr.Update(0.0f);
    EXPECT_EQ(g_waitResult, 1.0f);

    // 時間不足
    mgr.Update(0.3f);
    EXPECT_EQ(g_waitResult, 1.0f);

    // 十分な時間経過 → ステップ2、1.0秒待機
    mgr.Update(0.3f);
    EXPECT_EQ(g_waitResult, 2.0f);

    // 時間不足
    mgr.Update(0.5f);
    EXPECT_EQ(g_waitResult, 2.0f);

    // 十分な時間経過 → ステップ3、完了
    mgr.Update(0.6f);
    EXPECT_EQ(g_waitResult, 3.0f);

    EXPECT_EQ(mgr.GetActiveCount(), 0u);
}

// ============================================================================
// CoroutineManager
// ============================================================================

static int g_coA = 0, g_coB = 0;

Coroutine CoroutineA()
{
    g_coA = 1;
    co_await std::suspend_always{};
    g_coA = 2;
}

Coroutine CoroutineB()
{
    g_coB = 1;
    co_await std::suspend_always{};
    g_coB = 2;
    co_await std::suspend_always{};
    g_coB = 3;
}

TEST(CoroutineManagerTest, MultipleCoroutines)
{
    g_coA = g_coB = 0;
    CoroutineManager mgr;
    mgr.Start(CoroutineA());
    mgr.Start(CoroutineB());

    EXPECT_EQ(mgr.GetActiveCount(), 2u);

    mgr.Update(0.016f); // 両方とも1回再開
    EXPECT_EQ(g_coA, 1);
    EXPECT_EQ(g_coB, 1);

    mgr.Update(0.016f); // Aが終了、Bが再開
    EXPECT_EQ(g_coA, 2);
    EXPECT_EQ(g_coB, 2);
    EXPECT_EQ(mgr.GetActiveCount(), 1u); // Bのみ残存

    mgr.Update(0.016f); // Bが終了
    EXPECT_EQ(g_coB, 3);
    EXPECT_EQ(mgr.GetActiveCount(), 0u);
}

TEST(CoroutineManagerTest, StopAll)
{
    CoroutineManager mgr;
    mgr.Start(CoroutineA());
    mgr.Start(CoroutineB());
    EXPECT_EQ(mgr.GetActiveCount(), 2u);

    mgr.StopAll();
    EXPECT_EQ(mgr.GetActiveCount(), 0u);
}

Coroutine ZeroWaitCoroutine()
{
    co_await WaitForSeconds{ 0.0f };
    g_waitResult = 99.0f;
}

TEST(CoroutineTest, ZeroSecondWait)
{
    g_waitResult = 0.0f;
    CoroutineManager mgr;
    mgr.Start(ZeroWaitCoroutine());
    mgr.Update(0.0f);
    // 0秒待機は即座に完了するはず
    EXPECT_EQ(g_waitResult, 99.0f);
}

TEST(CoroutineTest, MoveSemantics)
{
    Coroutine co = SimpleCoroutine();
    Coroutine co2 = std::move(co);
    EXPECT_TRUE(co.IsDone()); // moved-from
    EXPECT_FALSE(co2.IsDone());
    co2.Resume();
}
