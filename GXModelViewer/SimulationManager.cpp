/// @file SimulationManager.cpp
/// @brief シーンシミュレーション管理実装

#include "SimulationManager.h"

void SimulationManager::Play(SceneGraph& scene)
{
    if (m_state == SimulationState::Playing) return;

    if (m_state == SimulationState::Editing)
    {
        // Save snapshot of all entities
        m_snapshots.clear();
        int count = scene.GetEntityCount();
        m_snapshots.resize(count);
        for (int i = 0; i < count; ++i)
        {
            const SceneEntity* e = scene.GetEntity(i);
            if (e)
            {
                m_snapshots[i].name = e->name;
                m_snapshots[i].transform = e->transform;
                m_snapshots[i].parentIndex = e->parentIndex;
                m_snapshots[i].visible = e->visible;
            }
        }
        m_simulationTime = 0.0f;
    }

    m_state = SimulationState::Playing;
}

void SimulationManager::Pause()
{
    if (m_state == SimulationState::Playing)
        m_state = SimulationState::Paused;
}

void SimulationManager::Resume()
{
    if (m_state == SimulationState::Paused)
        m_state = SimulationState::Playing;
}

void SimulationManager::Stop(SceneGraph& scene)
{
    if (m_state == SimulationState::Editing) return;

    // Restore entities from snapshot
    int count = (std::min)(scene.GetEntityCount(), static_cast<int>(m_snapshots.size()));
    for (int i = 0; i < count; ++i)
    {
        SceneEntity* e = scene.GetEntity(i);
        if (e && !m_snapshots[i].name.empty())
        {
            e->transform = m_snapshots[i].transform;
            e->parentIndex = m_snapshots[i].parentIndex;
            e->visible = m_snapshots[i].visible;
        }
    }

    m_snapshots.clear();
    m_simulationTime = 0.0f;
    m_state = SimulationState::Editing;
}

void SimulationManager::Update(float deltaTime, SceneGraph& scene)
{
    if (m_state != SimulationState::Playing) return;

    m_simulationTime += deltaTime;

    // Update all animators
    int count = scene.GetEntityCount();
    for (int i = 0; i < count; ++i)
    {
        SceneEntity* e = scene.GetEntity(i);
        if (!e) continue;

        if (e->animator && e->animator->IsPlaying())
        {
            e->animator->Update(deltaTime);
        }
    }
}
