#pragma once
/// @file AssetBrowser.h
/// @brief ImGui ベースのアセットブラウザ -- プロジェクトアセットのファイルブラウジング
///
/// 左ペインにディレクトリツリー、右ペインにアセットグリッドを表示し、
/// 検索バー、右クリックメニュー、ダブルクリック選択をサポートする。
/// @addtogroup grp_editor
/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief プロジェクトアセットのファイルブラウザパネル
class AssetBrowser
{
public:
    /// @brief パネルを描画する
    void Draw();

    /// @brief 選択中のアセットパスを取得する
    gx::String GetSelectedAssetPath() const { return m_selectedPath; }

    /// @brief パネルの表示/非表示を切り替える
    void Toggle() { m_visible = !m_visible; }

    /// @brief パネルが表示中かどうかを返す
    bool IsVisible() const { return m_visible; }

    /// @brief パネルの表示/非表示を設定する
    void SetVisible(bool v) { m_visible = v; }

    /// @brief 現在のディレクトリパスを取得する
    const gx::String& GetCurrentDirectory() const { return m_currentDir; }

    /// @brief 現在のディレクトリを設定する
    void SetCurrentDirectory(const gx::String& dir) { m_currentDir = dir; }

private:
    bool m_visible = true;
    gx::String m_currentDir = "Assets";
    gx::String m_selectedPath;
    gx::String m_searchFilter;
    char m_searchBuffer[256] = {};

    /// @brief ディレクトリツリーを再帰描画する
    void DrawDirectoryTree(const gx::String& path);

    /// @brief アセットグリッドを描画する
    void DrawAssetGrid();

    /// @brief アセットタイプに対応するラベル文字列を返す
    static const char* GetAssetTypeIcon(const gx::String& extension);
};

} // namespace gx
/// @}
