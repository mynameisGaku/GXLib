/// @file Material.cpp
/// @brief マテリアルマネージャーの実装
#include "pch.h"
#include "Graphics/3D/Material.h"

namespace GX
{

int MaterialManager::AllocateHandle()
{
    if (!m_freeHandles.empty())
    {
        int handle = m_freeHandles.back();
        m_freeHandles.pop_back();
        return handle;
    }

    int handle = m_nextHandle++;
    if (handle >= static_cast<int>(m_entries.size()))
        m_entries.resize(handle + 1);
    return handle;
}

int MaterialManager::CreateMaterial(const Material& material)
{
    int handle = AllocateHandle();
    m_entries[handle].material = material;
    m_entries[handle].active = true;
    return handle;
}

Material* MaterialManager::GetMaterial(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return nullptr;
    if (!m_entries[handle].active)
        return nullptr;
    return &m_entries[handle].material;
}

void MaterialManager::ReleaseMaterial(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;
    m_entries[handle].active = false;
    m_freeHandles.push_back(handle);
}

bool MaterialManager::SetTexture(int handle, MaterialTextureSlot slot, int textureHandle)
{
    Material* mat = GetMaterial(handle);
    if (!mat) return false;

    switch (slot)
    {
    case MaterialTextureSlot::Albedo:
        mat->albedoMapHandle = textureHandle;
        if (textureHandle >= 0) mat->constants.flags |= MaterialFlags::HasAlbedoMap;
        else mat->constants.flags &= ~MaterialFlags::HasAlbedoMap;
        break;
    case MaterialTextureSlot::Normal:
        mat->normalMapHandle = textureHandle;
        if (textureHandle >= 0) mat->constants.flags |= MaterialFlags::HasNormalMap;
        else mat->constants.flags &= ~MaterialFlags::HasNormalMap;
        break;
    case MaterialTextureSlot::MetalRoughness:
        mat->metRoughMapHandle = textureHandle;
        if (textureHandle >= 0) mat->constants.flags |= MaterialFlags::HasMetRoughMap;
        else mat->constants.flags &= ~MaterialFlags::HasMetRoughMap;
        break;
    case MaterialTextureSlot::AO:
        mat->aoMapHandle = textureHandle;
        if (textureHandle >= 0) mat->constants.flags |= MaterialFlags::HasAOMap;
        else mat->constants.flags &= ~MaterialFlags::HasAOMap;
        break;
    case MaterialTextureSlot::Emissive:
        mat->emissiveMapHandle = textureHandle;
        if (textureHandle >= 0) mat->constants.flags |= MaterialFlags::HasEmissiveMap;
        else mat->constants.flags &= ~MaterialFlags::HasEmissiveMap;
        break;
    default:
        return false;
    }
    return true;
}

bool MaterialManager::SetShaderHandle(int handle, int shaderHandle)
{
    Material* mat = GetMaterial(handle);
    if (!mat) return false;
    mat->shaderHandle = shaderHandle;
    return true;
}

} // namespace GX
