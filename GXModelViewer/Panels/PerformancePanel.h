#pragma once
/// @file PerformancePanel.h
/// @brief ImGui panel for real-time performance monitoring

/// @brief ImGui panel that shows FPS, frame time graph, and statistics
class PerformancePanel
{
public:
    /// Draw the performance panel.
    /// @param deltaTime Current frame delta time in seconds.
    /// @param fps Current frames per second.
    void Draw(float deltaTime, float fps);

private:
    static constexpr int k_HistorySize = 120;

    float m_frameTimeHistory[k_HistorySize] = {};
    int   m_historyOffset = 0;
    bool  m_historyFilled = false;
};
