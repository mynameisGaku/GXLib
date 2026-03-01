#pragma once
/// @file JobSystem.h
/// @brief マルチスレッドジョブシステム — 重い処理を複数スレッドに分散して実行する
///
/// Submit() でタスクを登録し、ワーカースレッドが並列に処理する。
/// SubmitAfter() で依存関係を指定でき、ParallelFor() で
/// ループを自動分割して並列化できる。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <unordered_set>

namespace gx {

/// @brief ジョブ関数の型
using JobFunction = std::function<void()>;

/// @brief ジョブハンドル（Submit の戻り値、Wait/IsComplete で使用）
struct JobHandle {
    uint64_t id = 0;  ///< ジョブID（0は無効）

    /// @brief ハンドルが有効かどうか
    /// @return 有効なら true
    bool IsValid() const { return id != 0; }
};

/// @brief ジョブの優先度
enum class JobPriority
{
    Low,     ///< 低優先度（バックグラウンド処理向け）
    Normal,  ///< 通常優先度
    High     ///< 高優先度（即時実行が望ましいもの）
};

/// @brief マルチスレッドジョブシステム（シングルトン）
class JobSystem {
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return JobSystemインスタンスへの参照
    static JobSystem& Instance();

    /// @brief ジョブシステムを初期化する
    /// @param workerCount ワーカースレッド数（0=自動検出）
    /// @return 成功なら true
    bool Initialize(uint32_t workerCount = 0);

    /// @brief ジョブシステムをシャットダウンする
    void Shutdown();

    /// @brief ジョブを登録する
    /// @param job 実行する関数
    /// @param priority 優先度（デフォルト: Normal）
    /// @return ジョブハンドル
    JobHandle Submit(JobFunction job, JobPriority priority = JobPriority::Normal);

    /// @brief 依存関係付きジョブを登録する
    /// @param dependency 依存先のジョブハンドル
    /// @param job 実行する関数
    /// @param priority 優先度（デフォルト: Normal）
    /// @return ジョブハンドル
    JobHandle SubmitAfter(JobHandle dependency, JobFunction job, JobPriority priority = JobPriority::Normal);

    /// @brief 指定ジョブの完了を待つ
    /// @param handle 待機するジョブのハンドル
    void Wait(JobHandle handle);

    /// @brief 全ジョブの完了を待つ
    void WaitAll();

    /// @brief 指定ジョブが完了済みか確認する
    /// @param handle 確認するジョブのハンドル
    /// @return 完了済みなら true
    bool IsComplete(JobHandle handle) const;

    /// @brief ループを自動分割して並列実行する
    /// @param count ループ回数
    /// @param body 各イテレーションで実行する関数（引数はインデックス）
    /// @param minBatchSize 1バッチの最小サイズ（デフォルト: 64）
    void ParallelFor(uint32_t count, const std::function<void(uint32_t)>& body, uint32_t minBatchSize = 64);

    /// @brief ワーカースレッド数を取得する
    /// @return ワーカースレッド数
    uint32_t GetWorkerCount() const { return m_workerCount; }

    /// @brief 初期化済みかどうか
    /// @return 初期化済みなら true
    bool IsInitialized() const { return m_initialized; }

private:
    JobSystem() = default;
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    /// @brief 内部ジョブデータ
    struct Job {
        JobFunction function;                        ///< 実行する関数
        JobPriority priority = JobPriority::Normal;  ///< ジョブの優先度
        uint64_t id = 0;                             ///< ジョブID
        uint64_t dependencyId = 0;                   ///< 依存先ジョブID（0=依存なし）
        bool completed = false;                      ///< 完了フラグ
    };

    struct PriorityCompare {
        bool operator()(const Job& a, const Job& b) const {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority);
        }
    };

    void WorkerThread();

    std::vector<std::thread> m_workers;                   ///< ワーカースレッド配列
    std::vector<Job> m_jobQueue;                          ///< ジョブキュー
    mutable std::mutex m_mutex;                           ///< キュー保護用ミューテックス
    std::condition_variable m_condition;                   ///< ジョブ追加通知用条件変数
    std::condition_variable m_completionCondition;         ///< ジョブ完了通知用条件変数
    std::atomic<bool> m_running{false};                   ///< システム稼働フラグ
    std::atomic<uint64_t> m_nextJobId{1};                 ///< 次に割り当てるジョブID
    std::unordered_set<uint64_t> m_completedJobs;         ///< 完了済みジョブIDの集合
    uint32_t m_workerCount = 0;                           ///< ワーカースレッド数
    bool m_initialized = false;                           ///< 初期化済みフラグ
};

} // namespace gx
/// @}
