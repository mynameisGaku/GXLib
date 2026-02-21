#pragma once
/// @file FileWatcher.h
/// @brief ファイルシステム変更監視 — ReadDirectoryChangesW ベース
///
/// 指定ディレクトリの変更を非同期で監視し、変更検出時にコールバックを発火する。
/// シェーダーホットリロードやアセットの自動更新に使用される。
/// Update() をフレームループ内で呼び出すこと。

namespace GX {

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
    void Watch(const std::string& directory,
               std::function<void(const std::string& path)> onChange);

    /// @brief 保留中の変更通知コールバックを発火する (メインスレッドで毎フレーム呼ぶ)
    void Update();

    /// @brief 全ての監視を停止する
    void Stop();

private:
    struct WatchEntry {
        std::string directory;
        HANDLE hDir = INVALID_HANDLE_VALUE;
        OVERLAPPED overlapped = {};
        std::vector<uint8_t> buffer;
        std::function<void(const std::string&)> onChange;
        bool active = false;
    };

    std::vector<std::unique_ptr<WatchEntry>> m_watches;
    std::thread m_watchThread;
    std::mutex m_mutex;
    std::atomic<bool> m_running{ false };
    HANDLE m_stopEvent = nullptr;
    std::vector<std::pair<std::string, std::function<void(const std::string&)>>> m_pendingNotifications;

    void WatchLoop();
    static void StartRead(WatchEntry* entry);
};

} // namespace GX
