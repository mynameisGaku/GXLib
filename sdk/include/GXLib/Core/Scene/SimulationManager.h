#pragma once
/// @file SimulationManager.h
/// @brief Play-in-Editor のシーン再生・一時停止・停止を制御
///
/// Play 開始時にシーンをスナップショットに退避し、
/// Stop 時に元の状態へ戻す。Step() で 1 フレーム単位の
/// コマ送りデバッグも行える。
/// @addtogroup grp_scene/// @{

#include "pch_common.h"
#include "Core/Scene/SceneSnapshot.h"

namespace gx
{

class Scene;

/// @brief シミュレーションの実行状態
enum class SimulationState : uint32_t
{
    Stopped,   ///< 停止中（エディタモード）
    Playing,   ///< 再生中（シミュレーション実行中）
    Paused,    ///< 一時停止中
};

/// @brief Play-in-Editor のシミュレーション管理クラス
///
/// Play開始時にシーン状態をスナップショットとして保存し、
/// Stop時にスナップショットから復元する。
/// Pause/Resume でシミュレーションの一時停止・再開を制御する。
class SimulationManager
{
public:
    SimulationManager() = default;
    ~SimulationManager() = default;

    /// @brief シミュレーションを開始する（シーン状態をキャプチャしてから再生開始）
    /// @param scene 対象のシーン
    void Play(Scene& scene);

    /// @brief シミュレーションを一時停止する
    void Pause();

    /// @brief 一時停止を解除して再開する
    void Resume();

    /// @brief シミュレーションを停止し、シーンをキャプチャ時の状態に復元する
    /// @param scene 対象のシーン
    void Stop(Scene& scene);

    /// @brief 1ステップだけシミュレーションを進める（一時停止中のデバッグ用）
    /// @param scene 対象のシーン
    /// @param fixedDt ステップの時間幅（デフォルト: 1/60秒）
    void Step(Scene& scene, float fixedDt = 1.0f / 60.0f);

    /// @brief 現在のシミュレーション状態を取得する
    /// @return 現在の SimulationState
    SimulationState GetState() const { return m_state; }

    /// @brief 再生中かどうか
    /// @return 再生中なら true
    bool IsPlaying() const { return m_state == SimulationState::Playing; }

    /// @brief 一時停止中かどうか
    /// @return 一時停止中なら true
    bool IsPaused() const { return m_state == SimulationState::Paused; }

    /// @brief 停止中かどうか
    /// @return 停止中なら true
    bool IsStopped() const { return m_state == SimulationState::Stopped; }

    /// @brief シミュレーション開始からの経過時間を取得する
    /// @return 経過時間（秒）
    float GetElapsedTime() const { return m_elapsedTime; }

    /// @brief シミュレーションのステップ実行回数を取得する
    /// @return ステップ実行回数
    uint32_t GetStepCount() const { return m_stepCount; }

private:
    SimulationState m_state = SimulationState::Stopped; ///< 現在の実行状態
    SceneSnapshot m_snapshot;                           ///< Play開始時のシーンスナップショット
    float m_elapsedTime = 0.0f;                         ///< シミュレーション開始からの経過時間（秒）
    uint32_t m_stepCount = 0;                           ///< ステップ実行回数
};

} // namespace gx
/// @}
