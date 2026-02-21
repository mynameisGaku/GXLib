/// @file Model.cpp
/// @brief モデルクラスの実装
#include "pch.h"
#include "Graphics/3D/Model.h"

namespace GX
{

const SubMesh* Model::GetSubMesh(uint32_t index) const
{
    const auto& subs = m_mesh.GetSubMeshes();
    if (index >= subs.size())
        return nullptr;
    return &subs[index];
}

SubMesh* Model::GetSubMesh(uint32_t index)
{
    auto& subs = m_mesh.GetSubMeshes();
    if (index >= subs.size())
        return nullptr;
    return &subs[index];
}

bool Model::SetSubMeshMaterial(uint32_t index, int materialHandle)
{
    SubMesh* sub = GetSubMesh(index);
    if (!sub)
        return false;
    sub->materialHandle = materialHandle;
    return true;
}

bool Model::SetSubMeshShader(uint32_t index, int shaderHandle)
{
    SubMesh* sub = GetSubMesh(index);
    if (!sub)
        return false;
    sub->shaderHandle = shaderHandle;
    return true;
}

void Model::SetCPUData(MeshCPUData data)
{
    m_cpuData = std::move(data);
    m_hasCpuData = true;
}

bool Model::ComputeAABB(XMFLOAT3& outMin, XMFLOAT3& outMax) const
{
    if (!m_hasCpuData)
        return false;

    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    bool hasVertices = false;

    // 静的頂点
    for (const auto& v : m_cpuData.staticVertices)
    {
        minX = (std::min)(minX, v.position.x);
        minY = (std::min)(minY, v.position.y);
        minZ = (std::min)(minZ, v.position.z);
        maxX = (std::max)(maxX, v.position.x);
        maxY = (std::max)(maxY, v.position.y);
        maxZ = (std::max)(maxZ, v.position.z);
        hasVertices = true;
    }

    // スキニング頂点
    for (const auto& v : m_cpuData.skinnedVertices)
    {
        minX = (std::min)(minX, v.position.x);
        minY = (std::min)(minY, v.position.y);
        minZ = (std::min)(minZ, v.position.z);
        maxX = (std::max)(maxX, v.position.x);
        maxY = (std::max)(maxY, v.position.y);
        maxZ = (std::max)(maxZ, v.position.z);
        hasVertices = true;
    }

    if (!hasVertices)
        return false;

    outMin = { minX, minY, minZ };
    outMax = { maxX, maxY, maxZ };
    return true;
}

int Model::FindAnimationIndex(const std::string& name) const
{
    for (uint32_t i = 0; i < m_animations.size(); ++i)
    {
        if (m_animations[i].GetName() == name)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace GX
