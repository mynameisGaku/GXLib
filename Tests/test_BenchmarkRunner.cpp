/// @file test_BenchmarkRunner.cpp
/// @brief BenchmarkRunnerマイクロベンチマークフレームワークのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/BenchmarkRunner.h"
#include <cstdio>
#include <fstream>
#include <thread>
#include <sstream>

using namespace gx;

// ===========================================================================
// BenchmarkRunnerテスト
// ===========================================================================

TEST(BenchmarkRunnerTest, InitialState)
{
    BenchmarkRunner runner;
    EXPECT_EQ(runner.GetBenchmarkCount(), 0);
    EXPECT_TRUE(runner.GetResults().empty());
}

TEST(BenchmarkRunnerTest, Register_IncrementsCount)
{
    BenchmarkRunner runner;
    runner.Register("noop", []() {});
    EXPECT_EQ(runner.GetBenchmarkCount(), 1);
}

TEST(BenchmarkRunnerTest, Register_Multiple)
{
    BenchmarkRunner runner;
    runner.Register("a", []() {});
    runner.Register("b", []() {});
    runner.Register("c", []() {});
    EXPECT_EQ(runner.GetBenchmarkCount(), 3);
}

TEST(BenchmarkRunnerTest, RunAll_ProducesResults)
{
    BenchmarkRunner runner;
    runner.Register("test1", []() { volatile int x = 0; (void)x; });
    runner.Register("test2", []() { volatile int x = 1; (void)x; });
    runner.RunAll();

    EXPECT_EQ(runner.GetResults().size(), 2u);
}

TEST(BenchmarkRunnerTest, Run_SingleByName)
{
    BenchmarkRunner runner;
    runner.Register("target", []() { volatile int x = 42; (void)x; });
    runner.Register("other", []() { volatile int x = 0; (void)x; });

    EXPECT_TRUE(runner.Run("target"));
    EXPECT_EQ(runner.GetResults().size(), 1u);
    EXPECT_EQ(runner.GetResults()[0].name, "target");
}

TEST(BenchmarkRunnerTest, Run_ReturnsFlaseForUnknown)
{
    BenchmarkRunner runner;
    EXPECT_FALSE(runner.Run("nonexistent"));
}

TEST(BenchmarkRunnerTest, ResultHasCorrectName)
{
    BenchmarkRunner runner;
    runner.Register("my_bench", []() {});
    runner.RunAll();

    ASSERT_EQ(runner.GetResults().size(), 1u);
    EXPECT_EQ(runner.GetResults()[0].name, "my_bench");
}

TEST(BenchmarkRunnerTest, ResultHasPositiveAvg)
{
    BenchmarkRunner runner;
    runner.Register("work", []() {
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) sum += i;
        (void)sum;
    });
    runner.RunAll();

    ASSERT_EQ(runner.GetResults().size(), 1u);
    EXPECT_GT(runner.GetResults()[0].avgMicroseconds, 0.0);
}

TEST(BenchmarkRunnerTest, ResultHasValidStatistics)
{
    BenchmarkRunner runner;
    runner.Register("stats_test", []() {
        volatile int x = 0;
        for (int i = 0; i < 50; ++i) x += i;
        (void)x;
    }, { 2, 50, 1 });
    runner.RunAll();

    ASSERT_EQ(runner.GetResults().size(), 1u);
    const auto& r = runner.GetResults()[0];
    EXPECT_GE(r.minMicroseconds, 0.0);
    EXPECT_GE(r.maxMicroseconds, r.minMicroseconds);
    EXPECT_GE(r.avgMicroseconds, r.minMicroseconds);
    EXPECT_LE(r.avgMicroseconds, r.maxMicroseconds);
    EXPECT_GE(r.medianMicroseconds, r.minMicroseconds);
    EXPECT_GE(r.stdDevMicroseconds, 0.0);
    EXPECT_EQ(r.iterations, 50);
    EXPECT_TRUE(r.passed);
}

TEST(BenchmarkRunnerTest, FindResult_Found)
{
    BenchmarkRunner runner;
    runner.Register("alpha", []() {});
    runner.Register("beta", []() {});
    runner.RunAll();

    const BenchmarkResult* r = runner.FindResult("beta");
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->name, "beta");
}

TEST(BenchmarkRunnerTest, FindResult_NotFound)
{
    BenchmarkRunner runner;
    runner.Register("alpha", []() {});
    runner.RunAll();

    const BenchmarkResult* r = runner.FindResult("gamma");
    EXPECT_EQ(r, nullptr);
}

TEST(BenchmarkRunnerTest, GenerateReport_ContainsNames)
{
    BenchmarkRunner runner;
    runner.Register("bench_a", []() {});
    runner.Register("bench_b", []() {});
    runner.RunAll();

    gx::String report = runner.GenerateReport();
    EXPECT_NE(report.find("bench_a"), gx::String::npos);
    EXPECT_NE(report.find("bench_b"), gx::String::npos);
    EXPECT_NE(report.find("Benchmark"), gx::String::npos);
}

TEST(BenchmarkRunnerTest, Clear_RemovesAll)
{
    BenchmarkRunner runner;
    runner.Register("a", []() {});
    runner.RunAll();
    runner.Clear();

    EXPECT_EQ(runner.GetBenchmarkCount(), 0);
    EXPECT_TRUE(runner.GetResults().empty());
}

TEST(BenchmarkRunnerTest, SaveAndLoadBaseline)
{
    BenchmarkRunner runner;
    runner.Register("baseline_test", []() {
        volatile int x = 0;
        for (int i = 0; i < 10; ++i) x += i;
        (void)x;
    });
    runner.RunAll();

    gx::String path = "test_baseline_temp.txt";
    ASSERT_TRUE(runner.SaveBaseline(path));

    BenchmarkRunner runner2;
    ASSERT_TRUE(runner2.LoadBaseline(path));

    // クリーンアップ
    std::remove(path.c_str());
}

TEST(BenchmarkRunnerTest, CompareWithBaseline_NoRegression)
{
    BenchmarkRunner runner;
    runner.Register("fast_op", []() { volatile int x = 1; (void)x; });
    runner.RunAll();

    // ベースラインを保存
    gx::String path = "test_baseline_noreg.txt";
    ASSERT_TRUE(runner.SaveBaseline(path));

    // ロードして比較（同じ結果なのでリグレッションしないはず）
    ASSERT_TRUE(runner.LoadBaseline(path));
    int regressions = runner.CompareWithBaseline(10.0f); // 非常に高いスレッショルド
    EXPECT_EQ(regressions, 0);

    std::remove(path.c_str());
}

TEST(BenchmarkRunnerTest, CompareWithBaseline_DetectsRegression)
{
    // 非常に高速な時間を持つ偽のベースラインを作成
    gx::String path = "test_baseline_reg.txt";
    {
        std::ofstream f(path);
        f << "slow_op 0.001\n"; // ベースラインは0.001usで不可能に速い
    }

    BenchmarkRunner runner;
    runner.Register("slow_op", []() {
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) sum += i;
        (void)sum;
    });
    runner.RunAll();

    ASSERT_TRUE(runner.LoadBaseline(path));
    int regressions = runner.CompareWithBaseline(0.05f); // 5%スレッショルド
    EXPECT_GE(regressions, 1);

    // 結果が失敗としてマークされていることを確認
    const auto* r = runner.FindResult("slow_op");
    ASSERT_NE(r, nullptr);
    EXPECT_FALSE(r->passed);

    std::remove(path.c_str());
}

TEST(BenchmarkRunnerTest, LoadBaseline_FailsOnMissingFile)
{
    BenchmarkRunner runner;
    EXPECT_FALSE(runner.LoadBaseline("nonexistent_baseline.txt"));
}

TEST(BenchmarkRunnerTest, SaveBaseline_FailsOnInvalidPath)
{
    BenchmarkRunner runner;
    runner.Register("x", []() {});
    runner.RunAll();
    // 存在しないディレクトリのような無効なパス
    // ほとんどのシステムでは失敗する
    // 注: 一部のシステムでは成功する可能性があるため、クラッシュしないことのみ確認
    runner.SaveBaseline("nonexistent_dir/baseline.txt");
}

TEST(BenchmarkRunnerTest, Config_WarmupAndMeasure)
{
    BenchmarkRunner runner;
    int callCount = 0;
    runner.Register("counted", [&]() { callCount++; }, { 5, 10, 1 });
    runner.RunAll();

    // ウォームアップ(5) + 計測(10) = 15回呼ばれるはず
    EXPECT_EQ(callCount, 15);
}

TEST(BenchmarkRunnerTest, Config_RepeatCount)
{
    BenchmarkRunner runner;
    int callCount = 0;
    runner.Register("repeated", [&]() { callCount++; }, { 0, 10, 3 });
    runner.RunAll();

    // ウォームアップ(0) + 計測(10) * 繰り返し(3) = 30
    EXPECT_EQ(callCount, 30);

    const auto& r = runner.GetResults()[0];
    EXPECT_EQ(r.iterations, 30);
}

TEST(BenchmarkRunnerTest, GenerateReport_HasHeader)
{
    BenchmarkRunner runner;
    runner.Register("x", []() {});
    runner.RunAll();

    gx::String report = runner.GenerateReport();
    EXPECT_NE(report.find("Avg"), gx::String::npos);
    EXPECT_NE(report.find("Min"), gx::String::npos);
    EXPECT_NE(report.find("Max"), gx::String::npos);
    EXPECT_NE(report.find("Median"), gx::String::npos);
}

TEST(BenchmarkRunnerTest, RunAll_ClearsPreviousResults)
{
    BenchmarkRunner runner;
    runner.Register("a", []() {});
    runner.RunAll();
    EXPECT_EQ(runner.GetResults().size(), 1u);

    runner.RunAll(); // 前の結果をクリアして新しい結果を生成するはず
    EXPECT_EQ(runner.GetResults().size(), 1u);
}

TEST(BenchmarkRunnerTest, Run_AppendsToResults)
{
    BenchmarkRunner runner;
    runner.Register("a", []() {});
    runner.Register("b", []() {});

    runner.Run("a");
    EXPECT_EQ(runner.GetResults().size(), 1u);
    runner.Run("b");
    EXPECT_EQ(runner.GetResults().size(), 2u);
}
