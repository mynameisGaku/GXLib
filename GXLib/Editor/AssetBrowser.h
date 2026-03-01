#pragma once
/// @file AssetBrowser.h
/// @brief プロジェクトのファイル・フォルダをブラウズするアセットブラウザ
///
/// ルートパスを設定するとディレクトリ内のファイル一覧を取得でき、
/// サブフォルダへの移動・上の階層への移動・一覧更新が行える。
/// UnityのProjectウィンドウに相当する機能。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <functional>
#include <filesystem>

namespace gx
{

/// @brief アセットブラウザの単一エントリ（ファイルまたはディレクトリ）
struct AssetBrowserEntry
{
    std::string name;           ///< ファイル/ディレクトリ名
    std::string path;           ///< フルパス
    std::string extension;      ///< ファイル拡張子（ディレクトリの場合は空）
    uint64_t    fileSize = 0;   ///< ファイルサイズ（バイト単位、ディレクトリの場合は0）
    bool        isDirectory = false; ///< ディレクトリならtrue
};

/// @brief プロジェクトファイルをナビゲート・選択するアセットブラウザパネル
class AssetBrowser
{
public:
    AssetBrowser() = default;
    ~AssetBrowser() = default;

    /// @brief ブラウジングのルートディレクトリを設定する
    void SetRootPath(const std::string& rootPath);

    /// @brief ルートパスを取得する
    const std::string& GetRootPath() const { return m_rootPath; }

    /// @brief サブディレクトリに移動する
    /// @return 移動に成功した場合true
    bool NavigateTo(const std::string& path);

    /// @brief 一つ上のディレクトリ階層に移動する
    /// @return 移動に成功した場合true（既にルートの場合はfalse）
    bool NavigateUp();

    /// @brief 現在のディレクトリパスを取得する
    const std::string& GetCurrentPath() const { return m_currentPath; }

    /// @brief 現在のディレクトリを再スキャンする
    void Refresh();

    /// @brief 現在のディレクトリのエントリ一覧を取得する
    const std::vector<AssetBrowserEntry>& GetEntries() const { return m_entries; }

    /// @brief 拡張子フィルタを設定する（例: ".hlsl", ".gxmdl"）。空文字列で全表示
    void SetFilter(const std::string& extension);

    /// @brief 現在のフィルタを取得する
    const std::string& GetFilter() const { return m_filter; }

    /// @brief エントリのダブルクリックを処理する
    void HandleDoubleClick(int index);

    /// @brief アセットファイルが開かれた時のコールバックを設定する
    using AssetOpenedCallback = std::function<void(const std::string& path)>;
    void SetOnAssetOpened(AssetOpenedCallback cb) { m_onAssetOpened = std::move(cb); }

    /// @brief 現在のビューのエントリ数を取得する
    int GetEntryCount() const { return static_cast<int>(m_entries.size()); }

    /// @brief 現在ルートディレクトリにいるかチェックする
    bool IsAtRoot() const { return m_currentPath == m_rootPath; }

private:
    void ScanDirectory();

    std::string m_rootPath;                       ///< ブラウジングのルートディレクトリ
    std::string m_currentPath;                    ///< 現在表示中のディレクトリパス
    std::string m_filter;                         ///< 拡張子フィルタ（空=全表示）
    std::vector<AssetBrowserEntry> m_entries;      ///< 現在のディレクトリのエントリ一覧
    AssetOpenedCallback m_onAssetOpened;           ///< アセット開封時コールバック
};

} // namespace gx
/// @}
