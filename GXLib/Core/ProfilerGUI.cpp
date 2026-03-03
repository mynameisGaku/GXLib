/// @file ProfilerGUI.cpp
/// @brief プロファイラGUIの実装
#include "pch_common.h"
#include "Core/ProfilerGUI.h"
#include "Core/Profiler.h"
#include "Core/Logger.h"

#include <algorithm>

namespace gx
{

// ============================================================================
// カテゴリカラーテーブル
// ============================================================================

static const struct
{
    const char* name;
    uint32_t    color;
} k_CategoryColors[] =
{
    { "Render",  0xFF4488FF },  // 青
    { "Physics", 0xFF44FF44 },  // 緑
    { "AI",      0xFFFFFF44 },  // 黄
    { "Audio",   0xFFAA44FF },  // 紫
    { "Script",  0xFFFF8844 },  // オレンジ
    { "Input",   0xFF44FFFF },  // シアン
    { "Scene",   0xFFFF4488 },  // ピンク
    { "IO",      0xFF888888 },  // グレー
};

static constexpr size_t k_NumCategoryColors = sizeof(k_CategoryColors) / sizeof(k_CategoryColors[0]);

// ============================================================================
// コンストラクタ
// ============================================================================

ProfilerGUI::ProfilerGUI()
{
    for (int i = 0; i < k_HistorySize; ++i)
    {
        m_history[i] = FrameSnapshot{};
    }
}

// ============================================================================
// カテゴリ推定
// ============================================================================

gx::String ProfilerGUI::InferCategory(const gx::String& sectionName)
{
    // セクション名からカテゴリを推定する
    // 大文字小文字を考慮せず部分文字列でマッチ
    gx::String lower = sectionName;
    for (auto& c : lower) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

    if (lower.find("render") != gx::String::npos || lower.find("draw") != gx::String::npos ||
        lower.find("shadow") != gx::String::npos || lower.find("gbuffer") != gx::String::npos ||
        lower.find("light") != gx::String::npos || lower.find("post") != gx::String::npos)
        return "Render";

    if (lower.find("physics") != gx::String::npos || lower.find("collisi") != gx::String::npos ||
        lower.find("simulate") != gx::String::npos)
        return "Physics";

    if (lower.find("ai") != gx::String::npos || lower.find("nav") != gx::String::npos ||
        lower.find("pathfind") != gx::String::npos || lower.find("behavior") != gx::String::npos)
        return "AI";

    if (lower.find("audio") != gx::String::npos || lower.find("sound") != gx::String::npos ||
        lower.find("music") != gx::String::npos)
        return "Audio";

    if (lower.find("script") != gx::String::npos || lower.find("lua") != gx::String::npos)
        return "Script";

    if (lower.find("input") != gx::String::npos || lower.find("gamepad") != gx::String::npos)
        return "Input";

    if (lower.find("scene") != gx::String::npos || lower.find("entity") != gx::String::npos)
        return "Scene";

    if (lower.find("io") != gx::String::npos || lower.find("file") != gx::String::npos ||
        lower.find("load") != gx::String::npos)
        return "IO";

    return "Render"; // デフォルト
}

// ============================================================================
// カテゴリカラー
// ============================================================================

uint32_t ProfilerGUI::GetCategoryColor(const gx::String& category)
{
    for (size_t i = 0; i < k_NumCategoryColors; ++i)
    {
        if (category == k_CategoryColors[i].name)
            return k_CategoryColors[i].color;
    }
    return 0xFF888888; // デフォルトグレー
}

// ============================================================================
// 更新
// ============================================================================

void ProfilerGUI::Update(float deltaTime)
{
    if (m_paused)
        return;

    m_timeSinceUpdate += deltaTime;

    // 更新間隔チェック
    if (m_timeSinceUpdate < m_config.updateInterval)
        return;

    m_timeSinceUpdate = 0.0f;

    // Profilerからデータを取得
    Profiler& profiler = Profiler::Instance();
    const auto& results = profiler.GetResults();

    FrameSnapshot snapshot;
    snapshot.frameTimeMs = profiler.GetFrameCPUTimeMs();
    snapshot.cpuTimeMs   = profiler.GetFrameCPUTimeMs();
    snapshot.gpuTimeMs   = 0.0f; // GPUProfilerが利用可能な場合に設定

    // メモリ使用量
    size_t memBytes = Profiler::GetMemoryUsage();
    snapshot.memoryUsedMB = static_cast<float>(memBytes) / (1024.0f * 1024.0f);

    // CPUセクションをTimelineBarに変換
    float accumulatedTime = 0.0f;
    for (const auto& section : results)
    {
        TimelineBar bar;
        bar.name       = section.name;
        bar.startMs    = accumulatedTime;
        bar.durationMs = section.timeMs;
        bar.depth      = section.depth;
        bar.category   = InferCategory(section.name);
        bar.color      = GetCategoryColor(bar.category);

        snapshot.cpuBars.push_back(std::move(bar));
        if (section.depth == 0)
            accumulatedTime += section.timeMs;
    }

    // maxVisibleSections に制限
    if (snapshot.cpuBars.size() > m_config.maxVisibleSections)
    {
        snapshot.cpuBars.resize(m_config.maxVisibleSections);
    }

    // リングバッファに追加
    m_history[m_historyIndex] = std::move(snapshot);
    m_historyIndex = (m_historyIndex + 1) % k_HistorySize;
    if (m_historyCount < k_HistorySize)
        ++m_historyCount;
}

// ============================================================================
// スナップショット取得
// ============================================================================

const FrameSnapshot& ProfilerGUI::GetCurrentSnapshot() const
{
    if (m_historyCount == 0)
        return m_emptySnapshot;

    int currentIdx = (m_historyIndex - 1 + k_HistorySize) % k_HistorySize;
    return m_history[currentIdx];
}

gx::Vector<FrameSnapshot> ProfilerGUI::GetFrameHistory(uint32_t count) const
{
    gx::Vector<FrameSnapshot> result;

    uint32_t available = static_cast<uint32_t>(m_historyCount);
    uint32_t toReturn = std::min(count, available);

    result.reserve(toReturn);

    for (uint32_t i = 0; i < toReturn; ++i)
    {
        // 最新から順に取得
        int idx = (m_historyIndex - 1 - static_cast<int>(i) + k_HistorySize * 2) % k_HistorySize;
        result.push_back(m_history[idx]);
    }

    return result;
}

// ============================================================================
// 統計
// ============================================================================

float ProfilerGUI::GetAverageFrameTime() const
{
    if (m_historyCount == 0)
        return 0.0f;

    float sum = 0.0f;
    for (int i = 0; i < m_historyCount; ++i)
    {
        sum += m_history[i].frameTimeMs;
    }
    return sum / static_cast<float>(m_historyCount);
}

float ProfilerGUI::GetPeakFrameTime() const
{
    if (m_historyCount == 0)
        return 0.0f;

    float peak = 0.0f;
    for (int i = 0; i < m_historyCount; ++i)
    {
        peak = std::max(peak, m_history[i].frameTimeMs);
    }
    return peak;
}

SectionStats ProfilerGUI::GetSectionStats(const gx::String& name) const
{
    SectionStats stats;
    stats.minMs = 1e30f;
    stats.maxMs = 0.0f;
    stats.avgMs = 0.0f;
    stats.sampleCount = 0;

    float totalMs = 0.0f;

    for (int i = 0; i < m_historyCount; ++i)
    {
        const auto& snapshot = m_history[i];
        for (const auto& bar : snapshot.cpuBars)
        {
            if (bar.name == name)
            {
                stats.minMs = std::min(stats.minMs, bar.durationMs);
                stats.maxMs = std::max(stats.maxMs, bar.durationMs);
                totalMs += bar.durationMs;
                ++stats.sampleCount;
                break; // 1フレーム1サンプル
            }
        }
    }

    if (stats.sampleCount > 0)
    {
        stats.avgMs = totalMs / static_cast<float>(stats.sampleCount);
    }
    else
    {
        stats.minMs = 0.0f;
    }

    return stats;
}

} // namespace gx
