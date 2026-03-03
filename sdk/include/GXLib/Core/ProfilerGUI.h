#pragma once
/// @file ProfilerGUI.h
/// @brief プロファイラGUI（タイムラインバー表示、フレーム履歴、セクション統計）
///
/// Profiler からフレームデータを収集し、タイムラインバー形式のスナップショットを管理する。
/// リングバッファによるフレーム履歴の保持、平均/ピーク計算、セクション選択をサポート。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief カラースキーム
enum class ProfilerColorScheme
{
    Light,   ///< ライトテーマ
    Dark,    ///< ダークテーマ
    Custom   ///< カスタムテーマ
};

/// @brief ソートモード
enum class ProfilerSortMode
{
    ByTime,  ///< 処理時間でソート
    ByName,  ///< 名前でソート
    ByCalls  ///< 呼び出し回数でソート
};

/// @brief プロファイラGUI設定
struct ProfilerGUIConfig
{
    bool                showCPU            = true;                          ///< CPU情報を表示
    bool                showGPU            = true;                          ///< GPU情報を表示
    bool                showMemory         = true;                          ///< メモリ情報を表示
    bool                showFrameGraph     = true;                          ///< フレームグラフを表示
    uint32_t            maxVisibleSections = 50;                            ///< 最大表示セクション数
    float               barHeight          = 20.0f;                         ///< バーの高さ（ピクセル）
    float               timelineWidthMs    = 33.3f;                         ///< タイムライン幅（ミリ秒）
    ProfilerColorScheme colorScheme        = ProfilerColorScheme::Dark;     ///< カラースキーム
    float               updateInterval     = 0.1f;                          ///< 更新間隔（秒）
};

/// @brief タイムラインバー
struct TimelineBar
{
    gx::String name;          ///< セクション名
    float       startMs  = 0.0f;  ///< 開始時刻（フレーム内ミリ秒）
    float       durationMs = 0.0f; ///< 処理時間（ミリ秒）
    int         depth    = 0;     ///< ネスト深度
    uint32_t    color    = 0xFF4488FF; ///< 表示色（ARGB）
    gx::String category;      ///< カテゴリ名
};

/// @brief フレームスナップショット
struct FrameSnapshot
{
    gx::Vector<TimelineBar> cpuBars;       ///< CPUタイムラインバー
    gx::Vector<TimelineBar> gpuBars;       ///< GPUタイムラインバー
    float                    frameTimeMs = 0.0f;  ///< フレーム全体の時間（ミリ秒）
    float                    cpuTimeMs   = 0.0f;  ///< CPU時間（ミリ秒）
    float                    gpuTimeMs   = 0.0f;  ///< GPU時間（ミリ秒）
    uint32_t                 drawCalls   = 0;     ///< ドローコール数
    uint32_t                 triangles   = 0;     ///< 三角形数
    float                    memoryUsedMB = 0.0f; ///< メモリ使用量（MB）
};

/// @brief セクション統計情報
struct SectionStats
{
    float minMs = 0.0f;    ///< 最小時間（ミリ秒）
    float maxMs = 0.0f;    ///< 最大時間（ミリ秒）
    float avgMs = 0.0f;    ///< 平均時間（ミリ秒）
    uint32_t sampleCount = 0; ///< サンプル数
};

/// @brief プロファイラGUI
///
/// Profiler のフレームデータをタイムラインバー形式で管理し、
/// フレーム履歴、統計情報、セクション選択などのGUI機能を提供する。
class ProfilerGUI
{
public:
    ProfilerGUI();
    ~ProfilerGUI() = default;

    /// @brief 設定を適用する
    /// @param config GUI設定
    void SetConfig(const ProfilerGUIConfig& config) { m_config = config; }

    /// @brief 現在の設定を取得する
    /// @return GUI設定
    const ProfilerGUIConfig& GetConfig() const { return m_config; }

    /// @brief フレームデータを更新する
    /// @param deltaTime 前フレームからの経過時間（秒）
    void Update(float deltaTime);

    /// @brief 現在のフレームスナップショットを取得する
    /// @return 最新のスナップショット
    const FrameSnapshot& GetCurrentSnapshot() const;

    /// @brief フレーム履歴を取得する
    /// @param count 取得するフレーム数
    /// @return スナップショットの配列
    gx::Vector<FrameSnapshot> GetFrameHistory(uint32_t count) const;

    /// @brief 平均フレーム時間を取得する
    /// @return 平均フレーム時間（ミリ秒）
    float GetAverageFrameTime() const;

    /// @brief ピークフレーム時間を取得する
    /// @return 履歴内のピークフレーム時間（ミリ秒）
    float GetPeakFrameTime() const;

    /// @brief 一時停止/再開を設定する
    /// @param paused trueで一時停止
    void SetPaused(bool paused) { m_paused = paused; }

    /// @brief 一時停止中か取得する
    /// @return 一時停止中ならtrue
    bool IsPaused() const { return m_paused; }

    /// @brief セクションを選択する
    /// @param name セクション名
    void SelectSection(const gx::String& name) { m_selectedSection = name; }

    /// @brief 選択中のセクション名を取得する
    /// @return セクション名（未選択の場合は空文字列）
    const gx::String& GetSelectedSection() const { return m_selectedSection; }

    /// @brief セクション選択を解除する
    void ClearSelection() { m_selectedSection.clear(); }

    /// @brief セクションの統計情報を取得する
    /// @param name セクション名
    /// @return 統計情報
    SectionStats GetSectionStats(const gx::String& name) const;

    /// @brief 表示/非表示を設定する
    /// @param visible trueで表示
    void SetVisible(bool visible) { m_visible = visible; }

    /// @brief 表示中か取得する
    /// @return 表示中ならtrue
    bool IsVisible() const { return m_visible; }

    /// @brief ソートモードを設定する
    /// @param mode ソートモード
    void SetSortMode(ProfilerSortMode mode) { m_sortMode = mode; }

    /// @brief ソートモードを取得する
    /// @return 現在のソートモード
    ProfilerSortMode GetSortMode() const { return m_sortMode; }

    /// @brief カテゴリに応じたデフォルトカラーを取得する
    /// @param category カテゴリ名
    /// @return ARGB色
    static uint32_t GetCategoryColor(const gx::String& category);

private:
    /// @brief カテゴリ名を推定する
    /// @param sectionName セクション名
    /// @return カテゴリ名
    static gx::String InferCategory(const gx::String& sectionName);

    ProfilerGUIConfig m_config;                                 ///< GUI設定
    ProfilerSortMode  m_sortMode = ProfilerSortMode::ByTime;   ///< 現在のソートモード
    bool              m_paused   = false;                       ///< 一時停止フラグ
    bool              m_visible  = false;                       ///< 表示中フラグ
    gx::String       m_selectedSection;                        ///< 選択中のセクション名
    float             m_timeSinceUpdate = 0.0f;                 ///< 前回更新からの経過時間

    static constexpr int k_HistorySize = 120;                   ///< 履歴バッファサイズ
    FrameSnapshot m_history[k_HistorySize];                     ///< フレーム履歴リングバッファ
    int           m_historyIndex = 0;                            ///< 履歴の書き込み位置
    int           m_historyCount = 0;                            ///< 記録済みフレーム数

    FrameSnapshot m_emptySnapshot; ///< 空のスナップショット（参照返し用）
};

} // namespace gx
/// @}
