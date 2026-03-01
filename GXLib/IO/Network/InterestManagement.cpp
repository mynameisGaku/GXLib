/// @file InterestManagement.cpp
/// @brief インタレスト管理（ネットワーク同期最適化）の実装
#include "pch_common.h"
#include "IO/Network/InterestManagement.h"
#include <cmath>
#include <algorithm>

namespace gx
{

void InterestManagement::Initialize(const InterestConfig& config)
{
    m_config = config;
    m_entities.clear();
    m_grid.clear();
}

void InterestManagement::RegisterEntity(uint32_t entityId, float x, float y, float z)
{
    InterestEntity e;
    e.entityId = entityId;
    e.x = x; e.y = y; e.z = z;
    m_entities[entityId] = e;
    UpdateGrid(entityId);
}

void InterestManagement::UpdateEntityPosition(uint32_t entityId, float x, float y, float z)
{
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;

    // Remove from old cell
    int64_t oldCell = GetCellId(it->second.x, it->second.z);
    m_grid[oldCell].erase(entityId);

    it->second.x = x; it->second.y = y; it->second.z = z;
    UpdateGrid(entityId);
}

void InterestManagement::RemoveEntity(uint32_t entityId)
{
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;

    int64_t cell = GetCellId(it->second.x, it->second.z);
    m_grid[cell].erase(entityId);
    m_entities.erase(it);
}

std::vector<uint32_t> InterestManagement::QueryRelevantEntities(uint32_t observerEntityId) const
{
    std::vector<uint32_t> result;
    auto obsIt = m_entities.find(observerEntityId);
    if (obsIt == m_entities.end()) return result;

    const auto& obs = obsIt->second;
    float aoi = m_config.aoiRadius;
    float aoiSq = aoi * aoi;

    // Check nearby cells
    int cellRange = static_cast<int>(std::ceil(aoi / m_config.gridCellSize));
    int64_t obsCellX = static_cast<int64_t>(std::floor(obs.x / m_config.gridCellSize));
    int64_t obsCellZ = static_cast<int64_t>(std::floor(obs.z / m_config.gridCellSize));

    for (int64_t cx = obsCellX - cellRange; cx <= obsCellX + cellRange; cx++)
    {
        for (int64_t cz = obsCellZ - cellRange; cz <= obsCellZ + cellRange; cz++)
        {
            int64_t cellId = cx * 100000 + cz;
            auto cellIt = m_grid.find(cellId);
            if (cellIt == m_grid.end()) continue;

            for (uint32_t eid : cellIt->second)
            {
                if (eid == observerEntityId) continue;
                auto eit = m_entities.find(eid);
                if (eit == m_entities.end()) continue;

                float dx = eit->second.x - obs.x;
                float dy = eit->second.y - obs.y;
                float dz = eit->second.z - obs.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                if (distSq <= aoiSq)
                    result.push_back(eid);
            }
        }
    }

    return result;
}

InterestPriority InterestManagement::GetPriority(uint32_t observerEntity, uint32_t targetEntity) const
{
    float dist = GetDistance(observerEntity, targetEntity);
    float aoiRatio = dist / m_config.aoiRadius;

    if (aoiRatio < m_config.nearThreshold)
        return InterestPriority::Near;
    if (aoiRatio < m_config.farThreshold)
        return InterestPriority::Mid;
    return InterestPriority::Far;
}

bool InterestManagement::ShouldUpdate(InterestPriority priority, uint32_t frameNumber) const
{
    switch (priority)
    {
    case InterestPriority::Near: return (frameNumber % m_config.nearInterval) == 0;
    case InterestPriority::Mid:  return (frameNumber % m_config.midInterval) == 0;
    case InterestPriority::Far:  return (frameNumber % m_config.farInterval) == 0;
    default: return false;
    }
}

int64_t InterestManagement::GetCellId(float x, float z) const
{
    int64_t cx = static_cast<int64_t>(std::floor(x / m_config.gridCellSize));
    int64_t cz = static_cast<int64_t>(std::floor(z / m_config.gridCellSize));
    return cx * 100000 + cz;
}

float InterestManagement::GetDistance(uint32_t entityA, uint32_t entityB) const
{
    auto itA = m_entities.find(entityA);
    auto itB = m_entities.find(entityB);
    if (itA == m_entities.end() || itB == m_entities.end())
        return std::numeric_limits<float>::max();

    float dx = itB->second.x - itA->second.x;
    float dy = itB->second.y - itA->second.y;
    float dz = itB->second.z - itA->second.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void InterestManagement::Clear()
{
    m_entities.clear();
    m_grid.clear();
}

void InterestManagement::UpdateGrid(uint32_t entityId)
{
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;

    int64_t cell = GetCellId(it->second.x, it->second.z);
    m_grid[cell].insert(entityId);
}

} // namespace gx
