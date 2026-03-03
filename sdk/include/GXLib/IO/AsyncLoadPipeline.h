#pragma once
/// @file AsyncLoadPipeline.h
/// @brief 2スレッド非同期ロードパイプライン
///
/// IO スレッド (読み込み) + Parse スレッド (展開) + メインスレッド (統合) の
/// 3段パイプラインでアセットを非同期ロードする。
/// @addtogroup grp_io
/// @{

#include "IO/FileSystem.h"

namespace gx
{

/// @brief ロード優先度
enum class LoadPriority : uint8_t
{
    Low      = 0,
    Normal   = 1,
    High     = 2,
    Critical = 3,
};

/// @brief ロードタスクの状態
enum class PipelineLoadStatus
{
    Pending,       ///< 投入済み・未開始
    IO,            ///< IOスレッドで読み込み中
    Parsing,       ///< Parseスレッドで展開中
    WaitIntegrate, ///< メインスレッド統合待ち
    Complete,      ///< 完了
    Error,         ///< エラー
    Cancelled,     ///< キャンセル済み
};

/// @brief Parse 関数型 — Parseスレッドで実行される
using ParseFunc = std::function<std::shared_ptr<void>(const FileData&)>;

/// @brief Integrate 関数型 — メインスレッドで実行される
using IntegrateFunc = std::function<void(std::shared_ptr<void>)>;

/// @brief バッチハンドル
struct BatchHandle
{
    uint32_t id = 0; ///< バッチID (0=無効)
    bool IsValid() const { return id != 0; }
};

/// @brief 2スレッド非同期ロードパイプライン
///
/// Submit() → IOキュー → [IOスレッド: VFS読み込み]
///   → Parseキュー → [Parseスレッド: 展開]
///   → Integrateキュー → [メインスレッド: Update()]
class AsyncLoadPipeline
{
public:
    AsyncLoadPipeline();
    ~AsyncLoadPipeline();

    /// @brief 単一アセットのロードを投入する
    /// @param path VFSパス
    /// @param parseFunc Parseスレッドで実行する展開関数
    /// @param integrateFunc メインスレッドで実行する統合関数
    /// @param priority ロード優先度
    /// @return タスクID
    uint32_t Submit(const gx::String& path,
                    ParseFunc parseFunc,
                    IntegrateFunc integrateFunc,
                    LoadPriority priority = LoadPriority::Normal);

    /// @brief 依存付きロードを投入する（先行タスク完了後に開始）
    /// @param dependsOn 先行タスクID
    /// @param path VFSパス
    /// @param parseFunc Parse関数
    /// @param integrateFunc Integrate関数
    /// @param priority ロード優先度
    /// @return タスクID
    uint32_t SubmitAfter(uint32_t dependsOn,
                         const gx::String& path,
                         ParseFunc parseFunc,
                         IntegrateFunc integrateFunc,
                         LoadPriority priority = LoadPriority::Normal);

    /// @brief バッチロードを投入する
    /// @param paths VFSパスリスト
    /// @param parseFunc Parse関数（全タスク共通）
    /// @param onAllComplete 全タスク完了時コールバック（メインスレッド）
    /// @param priority ロード優先度
    /// @return バッチハンドル
    BatchHandle SubmitBatch(const gx::Vector<gx::String>& paths,
                            ParseFunc parseFunc,
                            std::function<void()> onAllComplete,
                            LoadPriority priority = LoadPriority::Normal);

    /// @brief メインスレッドで完了キューを処理する (毎フレーム呼ぶ)
    void Update();

    /// @brief タスクの状態を取得する
    /// @param taskId タスクID
    /// @return ロード状態
    PipelineLoadStatus GetStatus(uint32_t taskId) const;

    /// @brief バッチの進捗を取得する
    /// @param batch バッチハンドル
    /// @return 0.0～1.0 の進捗率
    float GetProgress(BatchHandle batch) const;

    /// @brief タスクをキャンセルする
    /// @param taskId タスクID
    void Cancel(uint32_t taskId);

    /// @brief 全タスクをキャンセルする
    void CancelAll();

    /// @brief 保留中/処理中のタスク数を取得する
    /// @return アクティブなタスク数
    uint32_t GetActiveCount() const;

private:
    struct LoadTask
    {
        uint32_t id = 0;
        gx::String path;
        ParseFunc parseFunc;
        IntegrateFunc integrateFunc;
        LoadPriority priority = LoadPriority::Normal;
        PipelineLoadStatus status = PipelineLoadStatus::Pending;
        FileData ioResult;
        std::shared_ptr<void> parseResult;
        uint32_t dependsOn = 0;    // 0 = 依存なし
        uint32_t batchId = 0;      // 0 = バッチなし
        std::atomic<bool> cancelled{false};
    };

    struct BatchInfo
    {
        uint32_t id = 0;
        gx::Vector<uint32_t> taskIds;
        std::function<void()> onAllComplete;
        uint32_t completedCount = 0;
        bool fired = false;
    };

    void IOWorkerLoop();
    void ParseWorkerLoop();
    void EnqueueForIO(std::shared_ptr<LoadTask> task);
    void CheckPendingDependencies();

    mutable std::mutex m_mutex;
    std::condition_variable m_ioCv;
    std::condition_variable m_parseCv;

    std::thread m_ioThread;
    std::thread m_parseThread;
    std::atomic<bool> m_running{true};

    uint32_t m_nextTaskId = 1;
    uint32_t m_nextBatchId = 1;

    // IO キュー（優先度降順ソート）
    gx::Vector<std::shared_ptr<LoadTask>> m_ioQueue;
    // Parse キュー
    gx::Vector<std::shared_ptr<LoadTask>> m_parseQueue;
    // Integrate キュー（メインスレッドで処理）
    gx::Vector<std::shared_ptr<LoadTask>> m_integrateQueue;

    // 依存待ちタスク
    gx::Vector<std::shared_ptr<LoadTask>> m_pendingDeps;

    // タスク状態マップ
    gx::HashMap<uint32_t, std::shared_ptr<LoadTask>> m_tasks;

    // バッチ情報
    gx::HashMap<uint32_t, BatchInfo> m_batches;
};

} // namespace gx
/// @}
