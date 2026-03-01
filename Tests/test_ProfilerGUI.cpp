/// @file test_ProfilerGUI.cpp
/// @brief ProfilerGUI のテスト — 設定、更新、スナップショット、履歴、統計

#include <gtest/gtest.h>
#include "Core/ProfilerGUI.h"
#include "Core/Profiler.h"

using namespace gx;

// ============================================================================
// 設定テスト
// ============================================================================

TEST(ProfilerGUITest, DefaultConfig)
{
    ProfilerGUI gui;
    const auto& cfg = gui.GetConfig();

    EXPECT_TRUE(cfg.showCPU);
    EXPECT_TRUE(cfg.showGPU);
    EXPECT_TRUE(cfg.showMemory);
    EXPECT_TRUE(cfg.showFrameGraph);
    EXPECT_EQ(cfg.maxVisibleSections, 50u);
    EXPECT_FLOAT_EQ(cfg.barHeight, 20.0f);
    EXPECT_FLOAT_EQ(cfg.timelineWidthMs, 33.3f);
    EXPECT_EQ(cfg.colorScheme, ProfilerColorScheme::Dark);
    EXPECT_FLOAT_EQ(cfg.updateInterval, 0.1f);
}

TEST(ProfilerGUITest, SetConfig)
{
    ProfilerGUI gui;

    ProfilerGUIConfig cfg;
    cfg.showCPU = false;
    cfg.showGPU = false;
    cfg.maxVisibleSections = 100;
    cfg.barHeight = 30.0f;
    cfg.colorScheme = ProfilerColorScheme::Light;
    cfg.updateInterval = 0.5f;

    gui.SetConfig(cfg);

    EXPECT_FALSE(gui.GetConfig().showCPU);
    EXPECT_FALSE(gui.GetConfig().showGPU);
    EXPECT_EQ(gui.GetConfig().maxVisibleSections, 100u);
    EXPECT_FLOAT_EQ(gui.GetConfig().barHeight, 30.0f);
    EXPECT_EQ(gui.GetConfig().colorScheme, ProfilerColorScheme::Light);
    EXPECT_FLOAT_EQ(gui.GetConfig().updateInterval, 0.5f);
}

// ============================================================================
// 初期状態テスト
// ============================================================================

TEST(ProfilerGUITest, InitialState)
{
    ProfilerGUI gui;

    EXPECT_FALSE(gui.IsVisible());
    EXPECT_FALSE(gui.IsPaused());
    EXPECT_TRUE(gui.GetSelectedSection().empty());
    EXPECT_EQ(gui.GetSortMode(), ProfilerSortMode::ByTime);
    EXPECT_FLOAT_EQ(gui.GetAverageFrameTime(), 0.0f);
    EXPECT_FLOAT_EQ(gui.GetPeakFrameTime(), 0.0f);
}

// ============================================================================
// 更新テスト
// ============================================================================

TEST(ProfilerGUITest, UpdateEmpty)
{
    ProfilerGUI gui;

    // Profilerにデータがなくてもクラッシュしないこと
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f; // 即時更新
    gui.SetConfig(cfg);

    gui.Update(0.016f);

    const auto& snapshot = gui.GetCurrentSnapshot();
    // 空のスナップショットでもフレーム時間はProfilerから取得される
    EXPECT_GE(snapshot.frameTimeMs, 0.0f);
}

TEST(ProfilerGUITest, UpdateAccumulates)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f; // 即時更新
    gui.SetConfig(cfg);

    // Profilerにデータを入れる
    auto& profiler = Profiler::Instance();
    profiler.SetEnabled(true);

    profiler.BeginFrame();
    profiler.BeginSection("TestSection");
    profiler.EndSection("TestSection");
    profiler.EndFrame();

    // GUI更新
    gui.Update(0.016f);
    gui.Update(0.016f);
    gui.Update(0.016f);

    auto history = gui.GetFrameHistory(3);
    EXPECT_GE(history.size(), 1u);
}

// ============================================================================
// スナップショットテスト
// ============================================================================

TEST(ProfilerGUITest, SnapshotFields)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f;
    gui.SetConfig(cfg);

    auto& profiler = Profiler::Instance();
    profiler.SetEnabled(true);
    profiler.BeginFrame();
    profiler.EndFrame();

    gui.Update(0.016f);

    const auto& snapshot = gui.GetCurrentSnapshot();
    EXPECT_GE(snapshot.frameTimeMs, 0.0f);
    EXPECT_GE(snapshot.cpuTimeMs, 0.0f);
    EXPECT_GE(snapshot.memoryUsedMB, 0.0f);
}

// ============================================================================
// フレーム履歴テスト
// ============================================================================

TEST(ProfilerGUITest, FrameHistoryEmpty)
{
    ProfilerGUI gui;
    auto history = gui.GetFrameHistory(10);
    EXPECT_TRUE(history.empty());
}

TEST(ProfilerGUITest, FrameHistoryFill)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f;
    gui.SetConfig(cfg);

    auto& profiler = Profiler::Instance();
    profiler.SetEnabled(true);

    // 5フレーム分のデータを蓄積
    for (int i = 0; i < 5; ++i)
    {
        profiler.BeginFrame();
        profiler.EndFrame();
        gui.Update(0.016f);
    }

    auto history = gui.GetFrameHistory(5);
    EXPECT_EQ(history.size(), 5u);

    // 要求数より多い場合は利用可能な分だけ返す
    auto historyAll = gui.GetFrameHistory(100);
    EXPECT_EQ(historyAll.size(), 5u);
}

// ============================================================================
// 平均/ピーク時間テスト
// ============================================================================

TEST(ProfilerGUITest, AverageFrameTime)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f;
    gui.SetConfig(cfg);

    auto& profiler = Profiler::Instance();
    profiler.SetEnabled(true);

    for (int i = 0; i < 3; ++i)
    {
        profiler.BeginFrame();
        profiler.EndFrame();
        gui.Update(0.016f);
    }

    float avg = gui.GetAverageFrameTime();
    EXPECT_GE(avg, 0.0f);
}

TEST(ProfilerGUITest, PeakFrameTime)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f;
    gui.SetConfig(cfg);

    auto& profiler = Profiler::Instance();
    profiler.SetEnabled(true);

    for (int i = 0; i < 3; ++i)
    {
        profiler.BeginFrame();
        profiler.EndFrame();
        gui.Update(0.016f);
    }

    float peak = gui.GetPeakFrameTime();
    float avg  = gui.GetAverageFrameTime();
    EXPECT_GE(peak, avg); // ピーク >= 平均
}

// ============================================================================
// 一時停止テスト
// ============================================================================

TEST(ProfilerGUITest, PausedNoUpdate)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.updateInterval = 0.0f;
    gui.SetConfig(cfg);

    auto& profiler = Profiler::Instance();
    profiler.SetEnabled(true);

    // 最初に1フレーム蓄積
    profiler.BeginFrame();
    profiler.EndFrame();
    gui.Update(0.016f);

    auto historyBefore = gui.GetFrameHistory(100);

    // 一時停止して更新
    gui.SetPaused(true);
    EXPECT_TRUE(gui.IsPaused());

    profiler.BeginFrame();
    profiler.EndFrame();
    gui.Update(0.016f);

    auto historyAfter = gui.GetFrameHistory(100);

    // 一時停止中は履歴が増えないはず
    EXPECT_EQ(historyBefore.size(), historyAfter.size());
}

// ============================================================================
// セクション選択テスト
// ============================================================================

TEST(ProfilerGUITest, SelectSection)
{
    ProfilerGUI gui;
    gui.SelectSection("Render");
    EXPECT_EQ(gui.GetSelectedSection(), "Render");
}

TEST(ProfilerGUITest, ClearSelection)
{
    ProfilerGUI gui;
    gui.SelectSection("Physics");
    gui.ClearSelection();
    EXPECT_TRUE(gui.GetSelectedSection().empty());
}

TEST(ProfilerGUITest, GetSectionStatsEmpty)
{
    ProfilerGUI gui;
    SectionStats stats = gui.GetSectionStats("NonExistent");
    EXPECT_EQ(stats.sampleCount, 0u);
    EXPECT_FLOAT_EQ(stats.avgMs, 0.0f);
}

// ============================================================================
// 表示制御テスト
// ============================================================================

TEST(ProfilerGUITest, VisibilityToggle)
{
    ProfilerGUI gui;
    EXPECT_FALSE(gui.IsVisible());

    gui.SetVisible(true);
    EXPECT_TRUE(gui.IsVisible());

    gui.SetVisible(false);
    EXPECT_FALSE(gui.IsVisible());
}

// ============================================================================
// ソートモードテスト
// ============================================================================

TEST(ProfilerGUITest, SortModeDefault)
{
    ProfilerGUI gui;
    EXPECT_EQ(gui.GetSortMode(), ProfilerSortMode::ByTime);
}

TEST(ProfilerGUITest, SortModeSet)
{
    ProfilerGUI gui;
    gui.SetSortMode(ProfilerSortMode::ByName);
    EXPECT_EQ(gui.GetSortMode(), ProfilerSortMode::ByName);

    gui.SetSortMode(ProfilerSortMode::ByCalls);
    EXPECT_EQ(gui.GetSortMode(), ProfilerSortMode::ByCalls);
}

// ============================================================================
// カラースキーム/カテゴリテスト
// ============================================================================

TEST(ProfilerGUITest, ColorScheme)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.colorScheme = ProfilerColorScheme::Custom;
    gui.SetConfig(cfg);
    EXPECT_EQ(gui.GetConfig().colorScheme, ProfilerColorScheme::Custom);
}

TEST(ProfilerGUITest, TimelineBarLayout)
{
    TimelineBar bar;
    bar.name = "TestBar";
    bar.startMs = 0.0f;
    bar.durationMs = 5.0f;
    bar.depth = 1;
    bar.color = 0xFF4488FF;
    bar.category = "Render";

    EXPECT_EQ(bar.name, "TestBar");
    EXPECT_FLOAT_EQ(bar.startMs, 0.0f);
    EXPECT_FLOAT_EQ(bar.durationMs, 5.0f);
    EXPECT_EQ(bar.depth, 1);
    EXPECT_EQ(bar.color, 0xFF4488FFu);
    EXPECT_EQ(bar.category, "Render");
}

TEST(ProfilerGUITest, CategoryColors)
{
    // 各カテゴリに固有の色が割り当てられていることを確認
    uint32_t renderColor  = ProfilerGUI::GetCategoryColor("Render");
    uint32_t physicsColor = ProfilerGUI::GetCategoryColor("Physics");
    uint32_t aiColor      = ProfilerGUI::GetCategoryColor("AI");
    uint32_t audioColor   = ProfilerGUI::GetCategoryColor("Audio");
    uint32_t scriptColor  = ProfilerGUI::GetCategoryColor("Script");

    // 青
    EXPECT_EQ(renderColor, 0xFF4488FFu);
    // 緑
    EXPECT_EQ(physicsColor, 0xFF44FF44u);
    // 黄
    EXPECT_EQ(aiColor, 0xFFFFFF44u);
    // 紫
    EXPECT_EQ(audioColor, 0xFFAA44FFu);
    // オレンジ
    EXPECT_EQ(scriptColor, 0xFFFF8844u);

    // 全て異なる
    EXPECT_NE(renderColor, physicsColor);
    EXPECT_NE(physicsColor, aiColor);
    EXPECT_NE(aiColor, audioColor);
    EXPECT_NE(audioColor, scriptColor);
}

TEST(ProfilerGUITest, MaxVisibleSections)
{
    ProfilerGUI gui;
    ProfilerGUIConfig cfg;
    cfg.maxVisibleSections = 5;
    cfg.updateInterval = 0.0f;
    gui.SetConfig(cfg);

    EXPECT_EQ(gui.GetConfig().maxVisibleSections, 5u);
}
