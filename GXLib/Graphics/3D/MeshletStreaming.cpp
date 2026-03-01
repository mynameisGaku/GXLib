/// @file MeshletStreaming.cpp
/// @brief メッシュレットストリーミングシステムの実装
///
/// メッシュ登録・解除・LOD管理・統計収集のCPU側ロジック。
/// GPU側のメッシュレットバッファ管理はMesh Shader Pipeline側で行う想定。
#include "pch_graphics.h"
#include "Graphics/3D/MeshletStreaming.h"
#include "Core/Logger.h"

#include <algorithm>

namespace gx
{

// ---------------------------------------------------------------------------
// 設定
// ---------------------------------------------------------------------------

void MeshletStreaming::SetConfig(const MeshletStreamingConfig& config)
{
    m_config = config;
}

const MeshletStreamingConfig& MeshletStreaming::GetConfig() const
{
    return m_config;
}

// ---------------------------------------------------------------------------
// メッシュ登録
// ---------------------------------------------------------------------------

uint32_t MeshletStreaming::RegisterMesh(const StreamableMesh& mesh)
{
    uint32_t id;

    if (!m_freeIds.empty())
    {
        // フリーリストからIDを再利用
        id = m_freeIds.back();
        m_freeIds.pop_back();

        MeshState& state = m_meshes[id];
        state.mesh = mesh;
        state.currentLOD = 0;
        state.resident = false;
        state.valid = true;
    }
    else
    {
        // 新規ID割り当て
        id = static_cast<uint32_t>(m_meshes.size());
        MeshState state;
        state.mesh = mesh;
        state.currentLOD = 0;
        state.resident = false;
        state.valid = true;
        m_meshes.push_back(std::move(state));
    }

    GX_LOG_INFO("MeshletStreaming: Registered mesh '%s' (id=%u, meshlets=%u, lods=%zu)",
                mesh.assetPath.c_str(), id, mesh.totalMeshletCount, mesh.lods.size());

    return id;
}

// ---------------------------------------------------------------------------
// メッシュ登録解除
// ---------------------------------------------------------------------------

void MeshletStreaming::UnregisterMesh(uint32_t meshId)
{
    if (meshId >= m_meshes.size())
        return;

    MeshState& state = m_meshes[meshId];
    if (!state.valid)
        return;

    GX_LOG_INFO("MeshletStreaming: Unregistered mesh '%s' (id=%u)",
                state.mesh.assetPath.c_str(), meshId);

    state.valid = false;
    state.mesh = StreamableMesh{};
    state.currentLOD = 0;
    state.resident = false;

    m_freeIds.push_back(meshId);
}

// ---------------------------------------------------------------------------
// LOD取得
// ---------------------------------------------------------------------------

int MeshletStreaming::GetActiveLOD(uint32_t meshId) const
{
    if (meshId >= m_meshes.size())
        return -1;

    const MeshState& state = m_meshes[meshId];
    if (!state.valid)
        return -1;

    return state.currentLOD;
}

// ---------------------------------------------------------------------------
// 統計
// ---------------------------------------------------------------------------

uint32_t MeshletStreaming::GetResidentMeshletCount() const
{
    uint32_t total = 0;
    for (const auto& state : m_meshes)
    {
        if (!state.valid)
            continue;

        // 現在のLODのメッシュレット数を集計
        int lod = state.currentLOD;
        if (lod >= 0 && lod < static_cast<int>(state.mesh.lods.size()))
        {
            total += state.mesh.lods[lod].meshletCount;
        }
    }
    return total;
}

uint32_t MeshletStreaming::GetRegisteredMeshCount() const
{
    uint32_t count = 0;
    for (const auto& state : m_meshes)
    {
        if (state.valid)
            ++count;
    }
    return count;
}

MeshletStreaming::Stats MeshletStreaming::GetStats() const
{
    Stats stats;
    stats.residentMeshlets = GetResidentMeshletCount();
    stats.registeredMeshes = GetRegisteredMeshCount();
    return stats;
}

} // namespace gx
