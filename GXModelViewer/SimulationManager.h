#pragma once
/// @file SimulationManager.h
/// @brief シーンシミュレーション管理（Play/Pause/Stop + スナップショット復元）

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Transform3D.h"
#include <vector>
#include <string>

/// @brief シミュレーション状態
enum class SimulationState
{
    Editing,  ///< 通常編集モード
    Playing,  ///< シミュレーション再生中
    Paused    ///< シミュレーション一時停止中
};

/// @brief エンティティのスナップショット（シミュレーション開始前の状態）
struct EntitySnapshot
{
    std::string      name;
    GX::Transform3D  transform;
    int              parentIndex = -1;
    bool             visible = true;
};

/// @brief シーンのシミュレーション管理。Play開始時にスナップショットを保存し、Stop時に復元する。
class SimulationManager
{
public:
    /// @brief シミュレーションを開始する。現在のシーン状態をスナップショットとして保存。
    void Play(SceneGraph& scene);

    /// @brief シミュレーションを一時停止する
    void Pause();

    /// @brief シミュレーションを再開する
    void Resume();

    /// @brief シミュレーションを停止し、スナップショットからシーンを復元する
    void Stop(SceneGraph& scene);

    /// @brief シミュレーション中の更新処理（アニメーション等）
    void Update(float deltaTime, SceneGraph& scene);

    /// @brief 現在のシミュレーション状態を取得する
    SimulationState GetState() const { return m_state; }

    /// @brief 編集モードかどうか
    bool IsEditing() const { return m_state == SimulationState::Editing; }

    /// @brief 再生中かどうか（一時停止中は含まない）
    bool IsPlaying() const { return m_state == SimulationState::Playing; }

    /// @brief 一時停止中かどうか
    bool IsPaused() const { return m_state == SimulationState::Paused; }

private:
    SimulationState m_state = SimulationState::Editing;
    std::vector<EntitySnapshot> m_snapshots;
    float m_simulationTime = 0.0f;
};
