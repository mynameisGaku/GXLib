/// @file SceneGraph.cpp
/// @brief シーングラフ実装
///
/// フリーリスト方式でスロットを再利用する。削除はGPUリソースがインフライト中の
/// 可能性があるため、_pendingRemovalフラグで遅延し、GPUフラッシュ後に実削除する。

#include "SceneGraph.h"

int SceneGraph::AddEntity(const std::string& name)
{
    if (!m_freeIndices.empty())
    {
        int idx = m_freeIndices.back();
        m_freeIndices.pop_back();
        m_entities[idx] = SceneEntity{};
        m_entities[idx].name = name;
        return idx;
    }

    int idx = static_cast<int>(m_entities.size());
    SceneEntity entity;
    entity.name = name;
    m_entities.push_back(std::move(entity));
    return idx;
}

void SceneGraph::RemoveEntity(int index)
{
    if (index < 0 || index >= static_cast<int>(m_entities.size()))
        return;
    if (m_entities[index].name.empty() || m_entities[index]._pendingRemoval)
        return;

    // Mark for deferred removal (GPU resources may still be in-flight)
    m_entities[index]._pendingRemoval = true;
    m_pendingRemovals.push_back(index);

    // Deselect if the removed entity was selected
    if (selectedEntity == index)
        selectedEntity = -1;

    // Clear parent references pointing to this entity
    for (auto& e : m_entities)
    {
        if (e.parentIndex == index)
            e.parentIndex = -1;
    }
}

void SceneGraph::ProcessPendingRemovals()
{
    for (int index : m_pendingRemovals)
    {
        m_entities[index] = SceneEntity{};
        m_entities[index].name.clear();
        m_freeIndices.push_back(index);
    }
    m_pendingRemovals.clear();
}

SceneEntity* SceneGraph::GetEntity(int index)
{
    if (index < 0 || index >= static_cast<int>(m_entities.size()))
        return nullptr;
    if (m_entities[index].name.empty() || m_entities[index]._pendingRemoval)
        return nullptr;
    return &m_entities[index];
}

const SceneEntity* SceneGraph::GetEntity(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_entities.size()))
        return nullptr;
    if (m_entities[index].name.empty() || m_entities[index]._pendingRemoval)
        return nullptr;
    return &m_entities[index];
}
