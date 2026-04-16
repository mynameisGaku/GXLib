#pragma once
/// @file HotReloadManager.h
/// @brief ホットリロードマネージャー — シェーダーやスクリプトの変更を実行中に自動適用する
///
/// FileWatcher でディレクトリを監視し、ファイルが変わったら
/// 登録したコールバックを呼び出す。デバウンス付きなので
/// 連続した書き込みでも1回だけ通知される。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx {

class FileWatcher;

/// @brief ホットリロード対象のアセット種別
enum class HotReloadAssetType
{
    Shader,      ///< シェーダー (.hlsl, .hlsli)
    Texture,     ///< テクスチャ (.png, .jpg, .dds 等)
    Script,      ///< スクリプト (.lua)
    StyleSheet,  ///< スタイルシート (.css, .style)
    Config       ///< コンフィグ (.json, .xml, .ini)
};

/// @brief ホットリロード通知コールバック型
using HotReloadCallback = std::function<void(const gx::WString& filePath, HotReloadAssetType type)>;

/// @brief ホットリロードマネージャー
///
/// FileWatcher と連携してディレクトリを監視し、ファイル変更時に
/// 登録済みコールバックを呼び出す。デバウンス処理により
/// 連続した書き込みイベントを1回の通知にまとめる。
///
/// @code
/// HotReloadManager mgr;
/// mgr.Initialize(&fileWatcher);
/// mgr.WatchAssetDirectory(L"Shaders/", HotReloadAssetType::Shader);
/// mgr.OnReload(HotReloadAssetType::Shader, [](const gx::WString& path, HotReloadAssetType) {
///     // シェーダーを再コンパイル
/// });
/// @endcode
class HotReloadManager {
public:
    /// @brief コンストラクタ
    HotReloadManager();
    /// @brief デストラクタ
    ~HotReloadManager();

    /// @brief FileWatcher を使って初期化する
    /// @param watcher ファイル監視オブジェクト（外部所有）
    /// @return 成功で true
    bool Initialize(FileWatcher* watcher);

    /// @brief シャットダウンしてリソースを解放する
    void Shutdown();

    /// @brief アセットディレクトリを監視対象に追加する
    /// @param dir 監視対象ディレクトリパス
    /// @param type そのディレクトリに含まれるアセットの種別
    void WatchAssetDirectory(const gx::WString& dir, HotReloadAssetType type);

    /// @brief アセット種別ごとのリロードコールバックを設定する
    /// @param type アセット種別
    /// @param callback ファイル変更時に呼ばれるコールバック
    void OnReload(HotReloadAssetType type, HotReloadCallback callback);

    /// @brief デバウンス時間を設定する（連続変更を1回にまとめる待機時間）
    /// @param seconds デバウンス時間（秒）
    void SetDebounceTime(float seconds);

    /// @brief デバウンス時間を取得する
    /// @return デバウンス時間（秒）
    float GetDebounceTime() const { return m_debounceTime; }

    /// @brief 毎フレーム呼び出し（デバウンスタイマー処理）
    /// @param deltaTime デルタタイム（秒）
    void Update(float deltaTime);

    /// @brief 監視中ディレクトリ数を取得する
    /// @return 監視中ディレクトリ数
    uint32_t GetWatchedDirectoryCount() const;

private:
    /// @brief ファイルパスからアセット種別を判定する
    /// @param path ファイルパス
    /// @return 判定されたアセット種別
    HotReloadAssetType ClassifyFile(const gx::WString& path) const;

    /// @brief デバウンス中の変更情報
    struct PendingChange {
        gx::WString filePath;       ///< 変更されたファイルパス
        HotReloadAssetType type;     ///< アセット種別
        float timer = 0.0f;          ///< デバウンスタイマー（残り秒数）
    };

    FileWatcher* m_watcher = nullptr;                           ///< ファイル監視オブジェクト（非所有）
    float m_debounceTime = 0.3f;                                ///< デバウンス時間（秒）
    gx::Vector<PendingChange> m_pendingChanges;                ///< デバウンス中の変更リスト
    gx::HashMap<int, HotReloadCallback> m_callbacks;     ///< アセット種別ごとのコールバック
    uint32_t m_watchedDirCount = 0;                             ///< 監視中ディレクトリ数
    bool m_initialized = false;                                 ///< 初期化済みフラグ
};

} // namespace gx
/// @}
