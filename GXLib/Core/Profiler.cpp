/// @file Profiler.cpp
/// @brief CPUプロファイラの実装
#include "pch_common.h"
#include "Core/Profiler.h"
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace gx
{

Profiler::Profiler()
{
    QueryPerformanceFrequency(&m_frequency);
    m_frameStartTime.QuadPart = 0;
}

Profiler& Profiler::Instance()
{
    static Profiler instance;
    return instance;
}

void Profiler::BeginFrame()
{
    if (!m_enabled) return;
    m_currentResults.clear();
    m_openSections.clear();
    m_currentDepth = 0;
    QueryPerformanceCounter(&m_frameStartTime);
}

void Profiler::EndFrame()
{
    if (!m_enabled) return;

    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);

    m_lastFrameTimeMs = static_cast<float>(
        static_cast<double>(endTime.QuadPart - m_frameStartTime.QuadPart) * 1000.0 /
        static_cast<double>(m_frequency.QuadPart));

    // フレーム時間をリングバッファに記録
    m_frameHistory[m_frameHistoryIndex] = m_lastFrameTimeMs;
    m_frameHistoryIndex = (m_frameHistoryIndex + 1) % k_FrameHistorySize;
    if (m_frameHistoryCount < k_FrameHistorySize)
        ++m_frameHistoryCount;

    // バッファをスワップ
    m_lastResults = std::move(m_currentResults);
    m_currentResults.clear();
}

void Profiler::BeginSection(const char* name)
{
    if (!m_enabled) return;

    SectionEntry entry;
    entry.name = name;
    entry.depth = m_currentDepth;
    QueryPerformanceCounter(&entry.startTime);
    m_openSections.push_back(std::move(entry));
    m_currentDepth++;
}

void Profiler::EndSection(const char* name)
{
    if (!m_enabled) return;

    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);

    // 対応するオープンセクションを検索（LIFO順）
    for (int i = static_cast<int>(m_openSections.size()) - 1; i >= 0; --i)
    {
        if (m_openSections[i].name == name)
        {
            float timeMs = static_cast<float>(
                static_cast<double>(endTime.QuadPart - m_openSections[i].startTime.QuadPart) * 1000.0 /
                static_cast<double>(m_frequency.QuadPart));

            // 同名のセクションが既にあれば callCount をインクリメントし時間を加算
            bool found = false;
            for (auto& result : m_currentResults)
            {
                if (result.name == name && result.depth == m_openSections[i].depth)
                {
                    result.timeMs += timeMs;
                    result.callCount++;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                SectionResult result;
                result.name = m_openSections[i].name;
                result.timeMs = timeMs;
                result.depth = m_openSections[i].depth;
                result.callCount = 1;
                m_currentResults.push_back(std::move(result));
            }

            m_openSections.erase(m_openSections.begin() + i);
            m_currentDepth = std::max(0, m_currentDepth - 1);
            break;
        }
    }
}

const std::vector<SectionResult>& Profiler::GetResults() const
{
    return m_lastResults;
}

float Profiler::GetAverageFrameTimeMs() const
{
    if (m_frameHistoryCount == 0)
        return 0.0f;

    float sum = 0.0f;
    for (int i = 0; i < m_frameHistoryCount; ++i)
        sum += m_frameHistory[i];

    return sum / static_cast<float>(m_frameHistoryCount);
}

size_t Profiler::GetMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.WorkingSetSize;
    return 0;
}

} // namespace gx
