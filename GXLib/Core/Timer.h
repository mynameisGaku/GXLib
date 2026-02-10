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
    /// @brief コンストラクタ（パフォーマンスカウンタの周波数を取得する）
    Timer();

    /// @brief タイマーをリセットする（ゲーム開始時に呼ぶ）
    void Reset();

    /// @brief フレームごとに呼び出してデルタタイムを更新する
    void Tick();

    /// @brief 前フレームからの経過時間を取得する
    /// @return デルタタイム（秒）
    float GetDeltaTime() const { return m_deltaTime; }

    /// @brief ゲーム開始からの総経過時間を取得する
    /// @return 総経過時間（秒）
    double GetTotalTime() const { return m_totalTime; }

    /// @brief 現在のフレームレートを取得する
    /// @return FPS（フレーム毎秒）
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
