#pragma once
/// @file FileWatcher.h
/// @brief ファイルシステム変更監視 — ReadDirectoryChangesW ベース
///
/// 指定ディレクトリの変更を非同期で監視し、変更検出時にコールバックを発火する。
/// シェーダーホットリロードやアセットの自動更新に使用される。
/// Update() をフレームループ内で呼び出すこと。

namespace gx {
/// @addtogroup grp_io
/// @{

/// @brief ファイル変更監視クラス
class FileWatcher
{
public:
    /// @brief 停止イベントを作成する
    FileWatcher();
    /// @brief 全監視を停止しリソースを解放する
    ~FileWatcher();

    /// @brief ディレクトリの変更監視を開始する
    /// @param directory 監視対象ディレクトリパス
    /// @param onChange ファイル変更時のコールバック (変更されたファイルパスが引数)
    void Watch(const gx::String& directory,
               std::function<void(const gx::String& path)> onChange);

    /// @brief 保留中の変更通知コールバックを発火する (メインスレッドで毎フレーム呼ぶ)
    void Update();

    /// @brief 全ての監視を停止する
    void Stop();

private:
    struct WatchEntry {
        gx::String directory;                              ///< 監視対象ディレクトリパス
        HANDLE hDir = INVALID_HANDLE_VALUE;                 ///< ディレクトリハンドル
        OVERLAPPED overlapped = {};                         ///< 非同期I/O用OVERLAPPED構造体
        gx::Vector<uint8_t> buffer;                        ///< 変更通知バッファ
        std::function<void(const gx::String&)> onChange;   ///< 変更コールバック
        bool active = false;                                ///< 監視がアクティブか
    };

    gx::Vector<std::unique_ptr<WatchEntry>> m_watches;     ///< 監視エントリリスト
    std::thread m_watchThread;                               ///< 監視スレッド
    std::mutex m_mutex;                                      ///< 通知キュー用ミューテックス
    std::atomic<bool> m_running{ false };                     ///< スレッド実行フラグ
    HANDLE m_stopEvent = nullptr;                            ///< 停止シグナル用イベント
    gx::Vector<std::pair<gx::String, std::function<void(const gx::String&)>>> m_pendingNotifications; ///< メインスレッド配信待ち通知

    void WatchLoop();
    static void StartRead(WatchEntry* entry);
};

/// @}
} // namespace gx
