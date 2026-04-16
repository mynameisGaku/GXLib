#pragma once
/// @file ConsoleWindow.h
/// @brief ImGui ベースのコンソールウィンドウ -- ゲームコンソールのログ表示とコマンド入力
///
/// GameConsole の履歴を色分け表示し、フィルタボタン、検索、
/// コマンド入力行を備えたデバッグコンソール UI。
/// @addtogroup grp_editor
/// @{

#include "pch_graphics.h"

namespace gx
{

class GameConsole;

/// @brief ゲームコンソールのログ/コマンド入力パネル
class ConsoleWindow
{
public:
    /// @brief パネルを描画する
    /// @param console ゲームコンソール（nullptr の場合は「Not available」表示）
    void Draw(GameConsole* console);

    /// @brief パネルの表示/非表示を切り替える
    void Toggle() { m_visible = !m_visible; }

    /// @brief パネルが表示中かどうかを返す
    bool IsVisible() const { return m_visible; }

    /// @brief パネルの表示/非表示を設定する
    void SetVisible(bool v) { m_visible = v; }

    /// @brief 自動スクロールの有効/無効を設定する
    void SetAutoScroll(bool v) { m_autoScroll = v; }

    /// @brief 自動スクロールが有効かどうかを返す
    bool IsAutoScroll() const { return m_autoScroll; }

    /// @brief フィルタ設定を取得する
    bool IsShowInfo() const { return m_showInfo; }
    bool IsShowWarn() const { return m_showWarn; }
    bool IsShowError() const { return m_showError; }

    /// @brief フィルタ設定を変更する
    void SetShowInfo(bool v) { m_showInfo = v; }
    void SetShowWarn(bool v) { m_showWarn = v; }
    void SetShowError(bool v) { m_showError = v; }

private:
    bool m_visible = true;
    bool m_autoScroll = true;
    bool m_showInfo = true;
    bool m_showWarn = true;
    bool m_showError = true;
    gx::String m_filter;
    char m_inputBuffer[256] = {};
    char m_filterBuffer[128] = {};
    bool m_scrollToBottom = false;
};

} // namespace gx
/// @}
