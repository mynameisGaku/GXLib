#pragma once
/// @file PerformancePanel.h
/// @brief リアルタイムパフォーマンス監視パネル
///
/// FPS表示、フレームタイムのMin/Max/Avg統計、ImPlotによるフレームタイムグラフを提供する。
/// 直近120フレーム分のデータをリングバッファで保持する。

/// @brief FPS・フレームタイム統計・グラフを表示するパフォーマンスパネル
class PerformancePanel
{
public:
    /// @brief パフォーマンスパネルを描画する
    /// @param deltaTime 現在フレームのデルタタイム（秒）
    /// @param fps 現在のFPS値
    void Draw(float deltaTime, float fps);

private:
    static constexpr int k_HistorySize = 120;  ///< フレームタイム履歴のサンプル数

    float m_frameTimeHistory[k_HistorySize] = {};  ///< リングバッファ（ミリ秒）
    int   m_historyOffset = 0;   ///< 次に書き込むインデックス
    bool  m_historyFilled = false;  ///< バッファが一周したか
};
