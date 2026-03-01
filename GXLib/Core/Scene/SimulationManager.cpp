/// @file SimulationManager.cpp
/// @brief SimulationManager の実装
#include "pch_common.h"

#include "Core/Scene/SimulationManager.h"
#include "Core/Scene/Scene.h"
#include "Core/Logger.h"

namespace gx
{

void SimulationManager::Play(Scene& scene)
{
    if (m_state == SimulationState::Playing)
    {
        GX_LOG_WARN("[SimulationManager] Already playing");
        return;
    }

    if (m_state == SimulationState::Paused)
    {
        // 一時停止から再開する場合は Resume を使うべき
        Resume();
        return;
    }

    // 停止状態からの再生開始：シーンをキャプチャ
    m_snapshot.Capture(scene);
    m_state = SimulationState::Playing;
    m_elapsedTime = 0.0f;
    m_stepCount = 0;

    GX_LOG_INFO("[SimulationManager] Play started (scene: '{}', {} entities)",
                scene.GetName(), scene.GetEntityCount());
}

void SimulationManager::Pause()
{
    if (m_state != SimulationState::Playing)
    {
        GX_LOG_WARN("[SimulationManager] Cannot pause - not playing (state: {})",
                    static_cast<uint32_t>(m_state));
        return;
    }

    m_state = SimulationState::Paused;
    GX_LOG_INFO("[SimulationManager] Paused at {:.3f}s (step {})",
                m_elapsedTime, m_stepCount);
}

void SimulationManager::Resume()
{
    if (m_state != SimulationState::Paused)
    {
        GX_LOG_WARN("[SimulationManager] Cannot resume - not paused (state: {})",
                    static_cast<uint32_t>(m_state));
        return;
    }

    m_state = SimulationState::Playing;
    GX_LOG_INFO("[SimulationManager] Resumed at {:.3f}s", m_elapsedTime);
}

void SimulationManager::Stop(Scene& scene)
{
    if (m_state == SimulationState::Stopped)
    {
        GX_LOG_WARN("[SimulationManager] Already stopped");
        return;
    }

    // スナップショットからシーンを復元
    if (m_snapshot.IsValid())
    {
        m_snapshot.Restore(scene);
    }
    else
    {
        GX_LOG_WARN("[SimulationManager] No valid snapshot to restore");
    }

    m_state = SimulationState::Stopped;
    GX_LOG_INFO("[SimulationManager] Stopped after {:.3f}s ({} steps)",
                m_elapsedTime, m_stepCount);

    m_elapsedTime = 0.0f;
    m_stepCount = 0;
}

void SimulationManager::Step(Scene& scene, float fixedDt)
{
    if (m_state == SimulationState::Stopped)
    {
        GX_LOG_WARN("[SimulationManager] Cannot step - simulation is stopped. Call Play() first.");
        return;
    }

    // 一時停止中でもステップ実行は許可する
    // 再生中の場合もステップとして扱う

    scene.Update(fixedDt);
    m_elapsedTime += fixedDt;
    m_stepCount++;
}

} // namespace gx
