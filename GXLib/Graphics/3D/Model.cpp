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
