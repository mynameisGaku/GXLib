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

} // namespace GX
