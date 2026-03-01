#pragma once
/// @file TestConfig.h
/// @brief テスト設定・ユーティリティ
///
/// テストカテゴリラベル、タイムアウト設定、CI環境検出等のテストインフラ。

#include <gtest/gtest.h>

namespace gx { namespace Testing {

/// @brief CI環境で実行中か判定
inline bool IsCI()
{
    const char* ci = std::getenv("CI");
    return ci != nullptr && std::string(ci) == "true";
}

/// @brief GPU利用可能か判定（CIではfalseの可能性）
inline bool IsGPUAvailable()
{
    // CI環境ではGPUが利用できない可能性がある
    if (IsCI()) return false;
    // TODO: 実際のGPU検出はD3D12CreateDevice試行で判定
    return true;
}

/// @brief テストタイムアウト(ms)のデフォルト値
constexpr int k_DefaultTimeoutMs = 30000;

/// @brief ベンチマークテストの最小反復回数
constexpr int k_MinBenchmarkIterations = 100;

}} // namespace gx::Testing
