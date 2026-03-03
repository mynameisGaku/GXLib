/// @file CheckpointSystem.cpp
/// @brief チェックポイントシステム実装

#include "pch_common.h"
#include "Core/CheckpointSystem.h"
#include <algorithm>
#include <chrono>

namespace gx
{

CheckpointSystem::CheckpointSystem(uint32_t maxCheckpoints)
    : m_maxCheckpoints(maxCheckpoints)
{
}

uint32_t CheckpointSystem::CreateCheckpoint(const gx::String& name, const gx::String& location)
{
    CheckpointData cp;
    cp.id = m_nextId++;
    cp.name = name;
    cp.locationName = location;
    cp.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    m_checkpoints.push_back(cp);
    EnforceMaxCheckpoints();

    // Fire callback
    if (m_onCreate)
    {
        // Find the checkpoint we just added (it might be the last one after enforcement)
        const CheckpointData* created = GetCheckpoint(cp.id);
        if (created)
            m_onCreate(*created);
    }

    return cp.id;
}

const CheckpointData* CheckpointSystem::GetCheckpoint(uint32_t id) const
{
    for (const auto& cp : m_checkpoints)
    {
        if (cp.id == id)
            return &cp;
    }
    return nullptr;
}

bool CheckpointSystem::RemoveCheckpoint(uint32_t id)
{
    auto it = std::find_if(m_checkpoints.begin(), m_checkpoints.end(),
                           [id](const CheckpointData& cp) { return cp.id == id; });
    if (it != m_checkpoints.end())
    {
        m_checkpoints.erase(it);
        return true;
    }
    return false;
}

gx::Vector<CheckpointData> CheckpointSystem::GetAllCheckpoints() const
{
    return m_checkpoints;
}

void CheckpointSystem::SetMetadata(uint32_t checkpointId, const gx::String& key, const gx::String& value)
{
    for (auto& cp : m_checkpoints)
    {
        if (cp.id == checkpointId)
        {
            cp.metadata[key] = value;
            return;
        }
    }
}

gx::String CheckpointSystem::GetMetadata(uint32_t checkpointId, const gx::String& key) const
{
    for (const auto& cp : m_checkpoints)
    {
        if (cp.id == checkpointId)
        {
            auto it = cp.metadata.find(key);
            if (it != cp.metadata.end())
                return it->second;
            return "";
        }
    }
    return "";
}

uint32_t CheckpointSystem::QuickSave(const gx::String& location)
{
    // Remove previous quick save if it exists
    if (m_quickSaveId != 0)
        RemoveCheckpoint(m_quickSaveId);

    m_quickSaveId = CreateCheckpoint("QuickSave", location);
    return m_quickSaveId;
}

const CheckpointData* CheckpointSystem::QuickLoad() const
{
    if (m_quickSaveId == 0)
        return nullptr;
    return GetCheckpoint(m_quickSaveId);
}

void CheckpointSystem::NotifySceneChange(const gx::String& sceneName)
{
    if (m_autoOnSceneChange)
        CreateCheckpoint("Auto_" + sceneName, sceneName);
}

void CheckpointSystem::ClearAll()
{
    m_checkpoints.clear();
    m_quickSaveId = 0;
}

void CheckpointSystem::EnforceMaxCheckpoints()
{
    // Remove oldest checkpoints when over limit
    while (static_cast<uint32_t>(m_checkpoints.size()) > m_maxCheckpoints)
    {
        m_checkpoints.erase(m_checkpoints.begin());
    }
}

} // namespace gx
