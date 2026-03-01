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

std::string ProfilerGUI::InferCategory(const std::string& sectionName)
{
    // セクション名からカテゴリを推定する
    // 大文字小文字を考慮せず部分文字列でマッチ
    std::string lower = sectionName;
    for (auto& c : lower) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

    if (lower.find("render") != std::string::npos || lower.find("draw") != std::string::npos ||
        lower.find("shadow") != std::string::npos || lower.find("gbuffer") != std::string::npos ||
        lower.find("light") != std::string::npos || lower.find("post") != std::string::npos)
        return "Render";

    if (lower.find("physics") != std::string::npos || lower.find("collisi") != std::string::npos ||
        lower.find("simulate") != std::string::npos)
        return "Physics";

    if (lower.find("ai") != std::string::npos || lower.find("nav") != std::string::npos ||
        lower.find("pathfind") != std::string::npos || lower.find("behavior") != std::string::npos)
        return "AI";

    if (lower.find("audio") != std::string::npos || lower.find("sound") != std::string::npos ||
        lower.find("music") != std::string::npos)
        return "Audio";

    if (lower.find("script") != std::string::npos || lower.find("lua") != std::string::npos)
        return "Script";

    if (lower.find("input") != std::string::npos || lower.find("gamepad") != std::string::npos)
        return "Input";

    if (lower.find("scene") != std::string::npos || lower.find("entity") != std::string::npos)
        return "Scene";

    if (lower.find("io") != std::string::npos || lower.find("file") != std::string::npos ||
        lower.find("load") != std::string::npos)
        return "IO";

    return "Render"; // デフォルト
}

// ============================================================================
// カテゴリカラー
// ============================================================================

uint32_t ProfilerGUI::GetCategoryColor(const std::string& category)
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

std::vector<FrameSnapshot> ProfilerGUI::GetFrameHistory(uint32_t count) const
{
    std::vector<FrameSnapshot> result;

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

SectionStats ProfilerGUI::GetSectionStats(const std::string& name) const
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
