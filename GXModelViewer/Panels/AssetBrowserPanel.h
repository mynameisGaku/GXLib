#pragma once
/// @file AssetBrowserPanel.h
/// @brief プロジェクトフォルダブラウザパネル（D&Dサポート付き）
///
/// ファイルシステムをナビゲーションし、モデル/アニメーションファイルを
/// ダブルクリックでインポートしたり、ビューポートにD&Dできる。

#include <string>
#include <vector>

class GXModelViewerApp;

/// @brief ファイル/フォルダブラウザ。ダブルクリックでインポート、D&Dでビューポートへ投入。
class AssetBrowserPanel
{
public:
    /// @brief アセットブラウザパネルを描画する
    /// @param app インポート機能を呼び出すためのアプリ参照
    void Draw(GXModelViewerApp& app);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    /// @param app インポート機能を呼び出すためのアプリ参照
    void DrawContent(GXModelViewerApp& app);

    /// @brief ルートパスを設定する（起動時にカレントディレクトリ等で初期化する）
    /// @param path ルートディレクトリのパス
    void SetRootPath(const std::string& path);

private:
    /// @brief 1件のファイル/フォルダエントリ
    struct FileEntry
    {
        std::string name;      ///< ファイル名
        std::string fullPath;  ///< フルパス
        bool isDirectory;      ///< ディレクトリならtrue
    };

    /// @brief 現在のパスのファイル一覧を再読み込みする
    void RefreshEntries();

    /// @brief 拡張子がモデルファイルか判定する（.fbx/.gltf/.glb/.obj/.gxmd）
    /// @return モデルファイルならtrue
    bool IsModelFile(const std::string& ext) const;

    /// @brief 拡張子がアニメーションファイルか判定する（.gxan）
    /// @return アニメーションファイルならtrue
    bool IsAnimFile(const std::string& ext) const;

    std::string              m_rootPath;       ///< ブラウザのルートパス（戻り上限）
    std::string              m_currentPath;    ///< 現在表示中のディレクトリ
    std::vector<FileEntry>   m_entries;        ///< 現在ディレクトリのファイル一覧
    bool                     m_needsRefresh = true;  ///< 一覧の再読み込みが必要か
    char                     m_pathBuffer[512] = {}; ///< パスバーの編集バッファ
};
