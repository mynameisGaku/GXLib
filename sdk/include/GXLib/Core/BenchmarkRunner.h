#pragma once
/// @file BenchmarkRunner.h
/// @brief マイクロベンチマークフレームワーク — 関数の実行時間をマイクロ秒単位で計測する
///
/// 登録した関数を繰り返し実行し、平均・最小・最大・中央値を算出する。
/// ベースライン登録によるリグレッション検出や、
/// 結果のコンソール出力・CSV保存にも対応する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

namespace gx
{

/// @brief ベンチマークの設定
struct BenchmarkConfig
{
    int warmupIterations = 3;     ///< ウォームアップ反復回数（計測対象外）
    int measureIterations = 100;  ///< 計測反復回数
    int repeatCount = 1;          ///< 全計測の繰り返し回数
};

/// @brief ベンチマーク1回分の結果
struct BenchmarkResult
{
    gx::String name;                    ///< ベンチマーク名
    double avgMicroseconds = 0.0;        ///< 平均実行時間（マイクロ秒）
    double minMicroseconds = 0.0;        ///< 最小実行時間（マイクロ秒）
    double maxMicroseconds = 0.0;        ///< 最大実行時間（マイクロ秒）
    double medianMicroseconds = 0.0;     ///< 中央値実行時間（マイクロ秒）
    double stdDevMicroseconds = 0.0;     ///< 標準偏差（マイクロ秒）
    int iterations = 0;                  ///< 計測反復回数
    bool passed = true;                  ///< リグレッション検出時はfalse
};

/// @brief リグレッション検出用ベースラインエントリ
struct BenchmarkBaseline
{
    gx::String name;                ///< ベンチマーク名
    double avgMicroseconds = 0.0;    ///< ベースラインの平均実行時間（マイクロ秒）
};

/// @brief ベースライン比較付きマイクロベンチマークランナー
class BenchmarkRunner
{
public:
    BenchmarkRunner() = default;
    ~BenchmarkRunner() = default;

    /// @brief ベンチマーク関数を登録する
    /// @param name ベンチマーク名
    /// @param fn 計測対象の関数
    /// @param config ベンチマーク設定（省略時はデフォルト値）
    void Register(const gx::String& name, std::function<void()> fn,
                  const BenchmarkConfig& config = {});

    /// @brief 登録済み全ベンチマークを実行する
    void RunAll();

    /// @brief 名前を指定してベンチマークを1つ実行する
    /// @param name 実行するベンチマーク名
    /// @return ベンチマークが見つかり実行された場合true
    bool Run(const gx::String& name);

    /// @brief 最後の実行結果を取得する
    /// @return 結果配列への参照
    const gx::Vector<BenchmarkResult>& GetResults() const { return m_results; }

    /// @brief 名前で結果を検索する
    /// @param name ベンチマーク名
    /// @return 見つかった結果へのポインタ（見つからない場合は nullptr）
    const BenchmarkResult* FindResult(const gx::String& name) const;

    /// @brief 全結果のテキストレポートを生成する
    /// @return 人間可読なレポート文字列
    gx::String GenerateReport() const;

    /// @brief テキストファイルからベースラインを読み込む
    /// @param filePath ベースラインファイルのパス
    /// @return 成功でtrue
    bool LoadBaseline(const gx::String& filePath);

    /// @brief 現在の結果をベースラインとして保存する
    /// @param filePath 保存先ファイルパス
    /// @return 成功でtrue
    bool SaveBaseline(const gx::String& filePath) const;

    /// @brief 読み込んだベースラインと現在の結果を比較する
    /// @param regressionThreshold パーセンテージ閾値（デフォルト5% = 0.05）
    /// @return 検出されたリグレッション数
    int CompareWithBaseline(float regressionThreshold = 0.05f);

    /// @brief 登録済みベンチマーク数を取得する
    /// @return ベンチマークの登録数
    int GetBenchmarkCount() const { return static_cast<int>(m_benchmarks.size()); }

    /// @brief 全ベンチマークと結果をクリアする
    void Clear();

private:
    BenchmarkResult RunSingle(const gx::String& name, std::function<void()>& fn,
                               const BenchmarkConfig& config);

    /// @brief 登録済みベンチマークエントリ
    struct BenchmarkEntry
    {
        gx::String name;              ///< ベンチマーク名
        std::function<void()> fn;      ///< 計測対象の関数
        BenchmarkConfig config;        ///< ベンチマーク設定
    };

    gx::Vector<BenchmarkEntry> m_benchmarks;      ///< 登録済みベンチマーク配列
    gx::Vector<BenchmarkResult> m_results;         ///< 最後の実行結果配列
    gx::Vector<BenchmarkBaseline> m_baselines;     ///< 読み込み済みベースライン配列
};

} // namespace gx
/// @}
