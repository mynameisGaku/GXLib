/// @file Timer.cpp
/// @brief 高精度タイマー実装
#include "pch.h"
#include "Core/Timer.h"

namespace GX
{

Timer::Timer()
{
    QueryPerformanceFrequency(&m_frequency);
    Reset();
}

void Timer::Reset()
{
    QueryPerformanceCounter(&m_startTime);
    m_lastTime   = m_startTime;
    m_deltaTime  = 0.0f;
    m_totalTime  = 0.0;
    m_fps        = 0.0f;
    m_frameCount = 0;
    m_elapsed    = 0.0f;
}

void Timer::Tick()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    m_deltaTime = static_cast<float>(
        static_cast<double>(currentTime.QuadPart - m_lastTime.QuadPart)
        / static_cast<double>(m_frequency.QuadPart)
    );

    m_totalTime = static_cast<double>(currentTime.QuadPart - m_startTime.QuadPart)
        / static_cast<double>(m_frequency.QuadPart);

    m_lastTime = currentTime;

    // デルタタイムが異常に大きい場合（デバッグ中断など）はクランプ
    if (m_deltaTime > 0.5f)
    {
        m_deltaTime = 1.0f / 60.0f;
    }

    // FPS計算（1秒ごとに更新）
    m_frameCount++;
    m_elapsed += m_deltaTime;
    if (m_elapsed >= 1.0f)
    {
        m_fps = static_cast<float>(m_frameCount) / m_elapsed;
        m_frameCount = 0;
        m_elapsed = 0.0f;
    }
}

} // namespace GX
