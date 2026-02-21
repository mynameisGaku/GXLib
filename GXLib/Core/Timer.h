#pragma once
/// @file Timer.h
/// @brief フレーム間のデルタタイム・FPS を計測する高精度タイマー
///
/// DxLib の GetNowCount() / WaitTimer() に相当する時間計測機能を提供する。
/// 内部では QueryPerformanceCounter を使っており、マイクロ秒精度で計測できる。
/// 毎フレーム Tick() を呼ぶとデルタタイムと FPS が自動更新される。

#include "pch.h"

namespace GX
{

/// @brief QueryPerformanceCounter ベースの高精度タイマー
/// DxLib の GetNowCount() はミリ秒単位だが、こちらは float 秒で返す。
class Timer
{
public:
    /// @brief コンストラクタ。カウンタ周波数の取得と Reset() を行う
    Timer();

    /// @brief タイマーを初期状態に戻す
    /// アプリ起動時や、シーン切り替え時などに呼ぶ。
    void Reset();

    /// @brief 1 フレーム分の時間を計測する
    /// 毎フレーム先頭で呼ぶこと。デルタタイム・総経過時間・FPS が更新される。
    void Tick();

    /// @brief 前フレームからの経過時間を取得する
    /// @return デルタタイム（秒）。移動量の計算などに使う
    float GetDeltaTime() const { return m_deltaTime; }

    /// @brief Reset() からの総経過時間を取得する
    /// @return 経過時間（秒、double 精度）
    double GetTotalTime() const { return m_totalTime; }

    /// @brief 現在の FPS（1 秒ごとに更新）を取得する
    /// @return フレームレート
    float GetFPS() const { return m_fps; }

private:
    LARGE_INTEGER m_frequency;   ///< カウンタ周波数（1 秒あたりのカウント数）
    LARGE_INTEGER m_lastTime;    ///< 前回 Tick() 時のカウンタ値
    LARGE_INTEGER m_startTime;   ///< Reset() 時のカウンタ値（総経過時間の基準）

    float  m_deltaTime  = 0.0f;  ///< 直近フレームのデルタタイム（秒）
    double m_totalTime  = 0.0;   ///< Reset() からの総経過時間（秒）
    float  m_fps        = 0.0f;  ///< 直近 1 秒間の平均 FPS

    int    m_frameCount = 0;     ///< FPS 計算用フレームカウンタ
    float  m_elapsed    = 0.0f;  ///< FPS 計算用経過秒数
};

} // namespace GX
