/// @file SceneGraph.cpp
/// @brief SceneGraph implementation

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

    // Clear the entity data
    m_entities[index] = SceneEntity{};
    m_entities[index].name.clear(); // empty name marks as removed
    m_freeIndices.push_back(index);

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

SceneEntity* SceneGraph::GetEntity(int index)
{
    if (index < 0 || index >= static_cast<int>(m_entities.size()))
        return nullptr;
    if (m_entities[index].name.empty())
        return nullptr; // removed slot
    return &m_entities[index];
}

const SceneEntity* SceneGraph::GetEntity(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_entities.size()))
        return nullptr;
    if (m_entities[index].name.empty())
        return nullptr;
    return &m_entities[index];
}
