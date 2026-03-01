/// @file BenchmarkRunner.cpp
/// @brief マイクロベンチマークフレームワーク実装
#include "pch_common.h"
#include "Core/BenchmarkRunner.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <numeric>

namespace gx
{

void BenchmarkRunner::Register(const std::string& name, std::function<void()> fn,
                                const BenchmarkConfig& config)
{
    BenchmarkEntry entry;
    entry.name = name;
    entry.fn = std::move(fn);
    entry.config = config;
    m_benchmarks.push_back(std::move(entry));
}

void BenchmarkRunner::RunAll()
{
    m_results.clear();
    for (auto& entry : m_benchmarks)
    {
        m_results.push_back(RunSingle(entry.name, entry.fn, entry.config));
    }
}

bool BenchmarkRunner::Run(const std::string& name)
{
    for (auto& entry : m_benchmarks)
    {
        if (entry.name == name)
        {
            m_results.push_back(RunSingle(entry.name, entry.fn, entry.config));
            return true;
        }
    }
    return false;
}

const BenchmarkResult* BenchmarkRunner::FindResult(const std::string& name) const
{
    for (const auto& result : m_results)
    {
        if (result.name == name)
            return &result;
    }
    return nullptr;
}

std::string BenchmarkRunner::GenerateReport() const
{
    std::ostringstream oss;

    // ヘッダー
    char header[256];
    snprintf(header, sizeof(header), "%-30s %12s %12s %12s %12s %12s",
             "Benchmark", "Avg(us)", "Min(us)", "Max(us)", "Median(us)", "StdDev(us)");
    oss << header << "\n";

    // セパレータ
    for (int i = 0; i < 102; ++i)
        oss << "-";
    oss << "\n";

    // データ行
    for (const auto& result : m_results)
    {
        char line[256];
        snprintf(line, sizeof(line), "%-30s %12.2f %12.2f %12.2f %12.2f %12.2f",
                 result.name.c_str(),
                 result.avgMicroseconds,
                 result.minMicroseconds,
                 result.maxMicroseconds,
                 result.medianMicroseconds,
                 result.stdDevMicroseconds);
        oss << line;
        if (!result.passed)
            oss << " [REGRESSION]";
        oss << "\n";
    }

    return oss.str();
}

bool BenchmarkRunner::LoadBaseline(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    m_baselines.clear();

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // "name avg_us" をパース
        std::istringstream iss(line);
        BenchmarkBaseline baseline;
        if (iss >> baseline.name >> baseline.avgMicroseconds)
        {
            m_baselines.push_back(baseline);
        }
    }

    return true;
}

bool BenchmarkRunner::SaveBaseline(const std::string& filePath) const
{
    std::ofstream file(filePath);
    if (!file.is_open())
        return false;

    file << "# Benchmark baseline (name avg_microseconds)\n";
    for (const auto& result : m_results)
    {
        file << result.name << " " << result.avgMicroseconds << "\n";
    }

    return true;
}

int BenchmarkRunner::CompareWithBaseline(float regressionThreshold)
{
    int regressionCount = 0;

    for (auto& result : m_results)
    {
        result.passed = true;

        // 対応するベースラインを検索
        for (const auto& baseline : m_baselines)
        {
            if (baseline.name == result.name)
            {
                if (baseline.avgMicroseconds > 0.0)
                {
                    double ratio = (result.avgMicroseconds - baseline.avgMicroseconds)
                                   / baseline.avgMicroseconds;
                    if (ratio > static_cast<double>(regressionThreshold))
                    {
                        result.passed = false;
                        regressionCount++;
                    }
                }
                break;
            }
        }
    }

    return regressionCount;
}

void BenchmarkRunner::Clear()
{
    m_benchmarks.clear();
    m_results.clear();
    m_baselines.clear();
}

BenchmarkResult BenchmarkRunner::RunSingle(const std::string& name,
                                            std::function<void()>& fn,
                                            const BenchmarkConfig& config)
{
    BenchmarkResult result;
    result.name = name;

    // ウォームアップ
    for (int i = 0; i < config.warmupIterations; ++i)
    {
        fn();
    }

    // 全繰り返しの計測時間を収集
    std::vector<double> durations;
    int totalIterations = config.measureIterations * config.repeatCount;
    durations.reserve(totalIterations);

    for (int r = 0; r < config.repeatCount; ++r)
    {
        for (int i = 0; i < config.measureIterations; ++i)
        {
            auto start = std::chrono::high_resolution_clock::now();
            fn();
            auto end = std::chrono::high_resolution_clock::now();

            double us = std::chrono::duration<double, std::micro>(end - start).count();
            durations.push_back(us);
        }
    }

    result.iterations = totalIterations;

    if (durations.empty())
        return result;

    // 統計量を計算
    std::sort(durations.begin(), durations.end());

    // 最小値 / 最大値
    result.minMicroseconds = durations.front();
    result.maxMicroseconds = durations.back();

    // 平均値
    double sum = std::accumulate(durations.begin(), durations.end(), 0.0);
    result.avgMicroseconds = sum / static_cast<double>(durations.size());

    // 中央値
    size_t n = durations.size();
    if (n % 2 == 0)
    {
        result.medianMicroseconds = (durations[n / 2 - 1] + durations[n / 2]) / 2.0;
    }
    else
    {
        result.medianMicroseconds = durations[n / 2];
    }

    // 標準偏差
    double variance = 0.0;
    for (double d : durations)
    {
        double diff = d - result.avgMicroseconds;
        variance += diff * diff;
    }
    variance /= static_cast<double>(durations.size());
    result.stdDevMicroseconds = std::sqrt(variance);

    result.passed = true;
    return result;
}

} // namespace gx
