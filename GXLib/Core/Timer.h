#pragma once
/// @file Timer.h
/// @brief 高精度タイマークラス
///
/// 【初学者向け解説】
/// ゲームでは「前のフレームから何秒経ったか」（デルタタイム）が重要です。
/// キャラクターの移動速度やアニメーションをフレームレートに依存させないために、
/// 経過時間を正確に測定する必要があります。
///
/// このTimerクラスは、WindowsのQueryPerformanceCounter（高精度カウンタ）を使い、
/// マイクロ秒レベルの精度で時間を測定します。
///
/// 使い方：
/// 1. Reset()でタイマーを初期化
/// 2. 毎フレームTick()を呼ぶ
/// 3. GetDeltaTime()で前フレームからの経過時間を取得
/// 4. GetFPS()で現在のフレームレートを取得

#include "pch.h"

namespace GX
{

/// @brief QueryPerformanceCounterベースの高精度タイマー
class Timer
{
public:
    Timer();

    /// タイマーをリセット（ゲーム開始時に呼ぶ）
    void Reset();

    /// フレームごとに呼び出してデルタタイムを更新
    void Tick();

    /// 前フレームからの経過時間（秒）を取得
    float GetDeltaTime() const { return m_deltaTime; }

    /// ゲーム開始からの総経過時間（秒）を取得
    double GetTotalTime() const { return m_totalTime; }

    /// 現在のFPS（フレーム毎秒）を取得
    float GetFPS() const { return m_fps; }

private:
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_lastTime;
    LARGE_INTEGER m_startTime;

    float  m_deltaTime  = 0.0f;
    double m_totalTime  = 0.0;
    float  m_fps        = 0.0f;

    int    m_frameCount = 0;
    float  m_elapsed    = 0.0f;
};

} // namespace GX
