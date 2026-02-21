/// @file Timer.cpp
/// @brief Timer クラスの実装
#include "pch.h"
#include "Core/Timer.h"

namespace GX
{

Timer::Timer()
{
    // 周波数は起動中変わらないので、コンストラクタで 1 回だけ取得
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

    // カウンタ差分 / 周波数 ＝ 経過秒数
    m_deltaTime = static_cast<float>(
        static_cast<double>(currentTime.QuadPart - m_lastTime.QuadPart)
        / static_cast<double>(m_frequency.QuadPart)
    );

    m_totalTime = static_cast<double>(currentTime.QuadPart - m_startTime.QuadPart)
        / static_cast<double>(m_frequency.QuadPart);

    m_lastTime = currentTime;

    // デバッグブレークやスリープで長時間止まった場合、巨大なデルタタイムが
    // 物理演算やアニメーションを破壊するので、1/60 秒にクランプする
    if (m_deltaTime > 0.5f)
    {
        m_deltaTime = 1.0f / 60.0f;
    }

    // FPS は 1 秒分のフレーム数を蓄積してから算出（毎フレーム更新すると値がブレる）
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
