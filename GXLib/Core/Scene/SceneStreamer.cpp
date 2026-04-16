#include "pch_common.h"
/// @file SceneStreamer.cpp
/// @brief シーンストリーミング実装

#include "Core/Scene/SceneStreamer.h"
#include "Core/Scene/Scene.h"

namespace gx
{

// ============================================================================
// Scene::MergeFrom / ClearAllEntities 実装
// ============================================================================

void Scene::MergeFrom(Scene& other)
{
    for (auto& entity : other.m_entities)
    {
        entity->SetID(m_nextEntityID++);
        m_rootEntities.push_back(entity.get());
        m_entities.push_back(std::move(entity));
    }
    other.m_entities.clear();
    other.m_rootEntities.clear();
}

void Scene::ClearAllEntities()
{
    m_pendingDestroy.clear();
    m_rootEntities.clear();
    m_entities.clear();
}

// ============================================================================
// SceneStreamer 実装
// ============================================================================

int SceneStreamer::AddVolume(const StreamingVolume& volume)
{
    int handle = static_cast<int>(m_volumes.size());
    VolumeEntry entry;
    entry.volume = volume;
    entry.valid = true;
    m_volumes.push_back(std::move(entry));
    return handle;
}

void SceneStreamer::RemoveVolume(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_volumes.size())) return;
    if (!m_volumes[handle].valid) return;

    // 読み込み済みならシーンを解放
    auto it = m_loadedScenes.find(handle);
    if (it != m_loadedScenes.end())
    {
        m_loadedScenes.erase(it);
    }

    m_volumes[handle].volume.state = StreamingState::Unloaded;
    m_volumes[handle].valid = false;
}

void SceneStreamer::Update(float playerX, float playerY, float playerZ)
{
    for (int i = 0; i < static_cast<int>(m_volumes.size()); ++i)
    {
        auto& entry = m_volumes[i];
        if (!entry.valid) continue;

        auto& vol = entry.volume;
        float dist = DistanceTo(playerX, playerY, playerZ,
                                vol.position.x, vol.position.y, vol.position.z);

        switch (vol.state)
        {
        case StreamingState::Unloaded:
            if (dist < vol.loadRadius)
            {
                // 読み込み開始
                vol.state = StreamingState::Loading;

                // シーンを作成（同期的）
                auto scene = std::make_unique<Scene>(vol.scenePath);
                Scene* scenePtr = scene.get();
                m_loadedScenes[i] = std::move(scene);

                vol.state = StreamingState::Loaded;

                // コールバック発火
                if (m_onLoaded)
                {
                    m_onLoaded(i, *scenePtr);
                }
            }
            break;

        case StreamingState::Loaded:
            if (dist > vol.unloadRadius)
            {
                // 解放開始
                vol.state = StreamingState::Unloading;

                // シーンを解放
                m_loadedScenes.erase(i);

                vol.state = StreamingState::Unloaded;

                // コールバック発火
                if (m_onUnloaded)
                {
                    m_onUnloaded(i);
                }
            }
            break;

        case StreamingState::Loading:
        case StreamingState::Unloading:
            // 遷移中は何もしない（非同期将来対応用）
            break;
        }
    }
}

void SceneStreamer::SetOnSceneLoaded(std::function<void(int volumeHandle, Scene&)> callback)
{
    m_onLoaded = std::move(callback);
}

void SceneStreamer::SetOnSceneUnloaded(std::function<void(int volumeHandle)> callback)
{
    m_onUnloaded = std::move(callback);
}

StreamingState SceneStreamer::GetVolumeState(int handle) const
{
    if (handle < 0 || handle >= static_cast<int>(m_volumes.size())) return StreamingState::Unloaded;
    if (!m_volumes[handle].valid) return StreamingState::Unloaded;
    return m_volumes[handle].volume.state;
}

uint32_t SceneStreamer::GetVolumeCount() const
{
    uint32_t count = 0;
    for (const auto& entry : m_volumes)
    {
        if (entry.valid) ++count;
    }
    return count;
}

const StreamingVolume* SceneStreamer::GetVolume(int handle) const
{
    if (handle < 0 || handle >= static_cast<int>(m_volumes.size())) return nullptr;
    if (!m_volumes[handle].valid) return nullptr;
    return &m_volumes[handle].volume;
}

void SceneStreamer::ForceLoad(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_volumes.size())) return;
    if (!m_volumes[handle].valid) return;

    auto& vol = m_volumes[handle].volume;

    // 既に読み込み済みなら何もしない
    if (vol.state == StreamingState::Loaded) return;

    vol.state = StreamingState::Loading;

    auto scene = std::make_unique<Scene>(vol.scenePath);
    Scene* scenePtr = scene.get();
    m_loadedScenes[handle] = std::move(scene);

    vol.state = StreamingState::Loaded;

    if (m_onLoaded)
    {
        m_onLoaded(handle, *scenePtr);
    }
}

void SceneStreamer::ForceUnload(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_volumes.size())) return;
    if (!m_volumes[handle].valid) return;

    auto& vol = m_volumes[handle].volume;

    // 既に解放済みなら何もしない
    if (vol.state == StreamingState::Unloaded) return;

    vol.state = StreamingState::Unloading;

    m_loadedScenes.erase(handle);

    vol.state = StreamingState::Unloaded;

    if (m_onUnloaded)
    {
        m_onUnloaded(handle);
    }
}

float SceneStreamer::DistanceTo(float px, float py, float pz,
                                float vx, float vy, float vz)
{
    float dx = px - vx;
    float dy = py - vy;
    float dz = pz - vz;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace gx
