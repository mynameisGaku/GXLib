#pragma once
/// @file TimelineEditor.h
/// @brief ImGui ベースのタイムラインエディタ -- アニメーション/カットシーン用タイムライン
///
/// Timeline のトラック・キーフレームを視覚的に表示し、
/// 再生/停止/シーク操作、キーフレーム移動、ズーム操作をサポートする。
/// @addtogroup grp_editor
/// @{

#include "pch_graphics.h"

namespace gx
{

class Timeline;

/// @brief タイムラインエディタパネル
class TimelineEditor
{
public:
    /// @brief パネルを描画する
    /// @param timeline 表示するタイムライン（nullptr の場合は「Not available」表示）
    void Draw(Timeline* timeline);

    /// @brief パネルの表示/非表示を切り替える
    void Toggle() { m_visible = !m_visible; }

    /// @brief パネルが表示中かどうかを返す
    bool IsVisible() const { return m_visible; }

    /// @brief パネルの表示/非表示を設定する
    void SetVisible(bool v) { m_visible = v; }

    /// @brief ズーム倍率を取得する
    float GetZoom() const { return m_zoom; }

    /// @brief ズーム倍率を設定する
    void SetZoom(float z) { m_zoom = z; }

    /// @brief 水平スクロール位置を取得する
    float GetScrollX() const { return m_scrollX; }

    /// @brief 水平スクロール位置を設定する
    void SetScrollX(float x) { m_scrollX = x; }

    /// @brief 選択中のトラックインデックスを取得する
    int GetSelectedTrack() const { return m_selectedTrack; }

    /// @brief 選択中のキーフレームインデックスを取得する
    int GetSelectedKey() const { return m_selectedKey; }

    /// @brief 現在のプレイヘッド時刻を取得する
    float GetCurrentTime() const { return m_currentTime; }

    /// @brief プレイヘッド時刻を設定する
    void SetCurrentTime(float t) { m_currentTime = t; }

private:
    bool m_visible = true;
    float m_zoom = 1.0f;
    float m_scrollX = 0.0f;
    float m_currentTime = 0.0f;
    int m_selectedTrack = -1;
    int m_selectedKey = -1;

    /// @brief ツールバー（再生/停止/シーク）を描画する
    void DrawToolbar(Timeline* timeline);

    /// @brief トラックリストを描画する
    void DrawTrackList(Timeline* timeline);

    /// @brief タイムラインエリア（キーフレーム、プレイヘッド）を描画する
    void DrawTimelineArea(Timeline* timeline);
};

} // namespace gx
/// @}
