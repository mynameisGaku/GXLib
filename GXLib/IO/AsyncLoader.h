#pragma once
/// @file AsyncLoader.h
/// @brief バックグラウンドスレッドによる非同期アセットローダー
///
/// ワーカースレッドでファイルを読み込み、メインスレッドでコールバックを発火する。
/// Update() をフレームループ内で呼び出すこと。

#include "IO/FileSystem.h"

namespace GX {

/// @brief 読み込みリクエストの状態
enum class LoadStatus {
    Pending,    ///< キューに入っている (未開始)
    Loading,    ///< 読み込み中
    Complete,   ///< 完了
    Error       ///< エラー発生
};

/// @brief 読み込みリクエスト (内部用)
struct LoadRequest {
    std::string path;                           ///< ファイルパス
    LoadStatus status = LoadStatus::Pending;    ///< 現在の状態
    FileData result;                            ///< 読み込み結果
    std::function<void(FileData&)> onComplete;  ///< 完了コールバック
};

/// @brief 非同期アセットローダー
///
/// バックグラウンドスレッドでファイルを読み込み、
/// メインスレッドの Update() 呼び出し時にコールバックを発火する。
class AsyncLoader
{
public:
    /// @brief ワーカースレッドを起動する
    AsyncLoader();

    /// @brief ワーカースレッドを停止し、保留リクエストをキャンセルする
    ~AsyncLoader();

    /// @brief 非同期読み込みリクエストを送信する
    /// @param path ファイルパス (VFS経由)
    /// @param onComplete 完了時のコールバック (メインスレッドで呼ばれる)
    /// @return リクエストID (GetStatus()で状態確認に使用)
    uint32_t Load(const std::string& path,
                  std::function<void(FileData&)> onComplete);

    /// @brief 完了したリクエストのコールバックを発火する (メインスレッドで毎フレーム呼ぶ)
    void Update();

    /// @brief 全ての保留リクエストをキャンセルする
    void CancelAll();

    /// @brief リクエストの現在の状態を取得する
    /// @param requestId Load()が返したリクエストID
    /// @return 読み込み状態
    LoadStatus GetStatus(uint32_t requestId) const;

private:
    std::thread m_workerThread;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_running = true;
    uint32_t m_nextId = 1;

    std::vector<std::pair<uint32_t, std::shared_ptr<LoadRequest>>> m_pendingQueue;
    std::vector<std::shared_ptr<LoadRequest>> m_completedQueue;
    std::unordered_map<uint32_t, LoadStatus> m_statusMap;

    void WorkerLoop();
};

} // namespace GX
