/// @file PerformancePanel.cpp
/// @brief ImGui panel for real-time performance monitoring

#include "PerformancePanel.h"
#include <imgui.h>
#include <implot.h>
#include <cstring>

void PerformancePanel::Draw(float deltaTime, float fps)
{
    if (!ImGui::Begin("Performance"))
    {
        ImGui::End();
        return;
    }

    // Record frame time into circular buffer
    float frameTimeMs = deltaTime * 1000.0f;
    m_frameTimeHistory[m_historyOffset] = frameTimeMs;
    m_historyOffset = (m_historyOffset + 1) % k_HistorySize;
    if (m_historyOffset == 0)
        m_historyFilled = true;

    // --- FPS Counter ---
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.2f ms", frameTimeMs);
    ImGui::Separator();

    // --- Statistics ---
    int count = m_historyFilled ? k_HistorySize : m_historyOffset;
    float minTime = 9999.0f;
    float maxTime = 0.0f;
    float avgTime = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        float t = m_frameTimeHistory[i];
        if (t < minTime) minTime = t;
        if (t > maxTime) maxTime = t;
        avgTime += t;
    }
    if (count > 0)
        avgTime /= static_cast<float>(count);
    else
        minTime = 0.0f;

    ImGui::Text("Min: %.2f ms  Max: %.2f ms  Avg: %.2f ms", minTime, maxTime, avgTime);
    ImGui::Separator();

    // --- Frame Time Graph ---
    // Build a linear array from the circular buffer for plotting
    float plotData[k_HistorySize];
    int plotCount = count;
    if (m_historyFilled)
    {
        // Reorder so oldest is at index 0
        for (int i = 0; i < k_HistorySize; ++i)
        {
            plotData[i] = m_frameTimeHistory[(m_historyOffset + i) % k_HistorySize];
        }
    }
    else
    {
        memcpy(plotData, m_frameTimeHistory, sizeof(float) * count);
    }

    // Use ImPlot if available for a nicer graph
    if (ImPlot::BeginPlot("Frame Time (ms)", ImVec2(-1, 150), ImPlotFlags_NoMenus))
    {
        ImPlot::SetupAxes("Frame", "ms", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Frame Time", plotData, plotCount);
        ImPlot::EndPlot();
    }

    ImGui::End();
}
