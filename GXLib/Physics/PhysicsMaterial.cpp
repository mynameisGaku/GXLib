/// @file PhysicsMaterial.cpp
/// @brief PhysicsMaterial の実装
#include "pch_common.h"
#include "Physics/PhysicsMaterial.h"

namespace gx
{

PhysicsMaterialParams PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset preset)
{
    PhysicsMaterialParams p;
    switch (preset)
    {
    case PhysicsMaterialPreset::Default:
        p = { 0.5f, 0.3f, 1.0f, 0.05f, 0.05f };
        break;
    case PhysicsMaterialPreset::Metal:
        p = { 0.3f, 0.2f, 7.8f, 0.02f, 0.02f };
        break;
    case PhysicsMaterialPreset::Wood:
        p = { 0.6f, 0.3f, 0.6f, 0.1f, 0.1f };
        break;
    case PhysicsMaterialPreset::Rubber:
        p = { 0.9f, 0.8f, 1.1f, 0.1f, 0.1f };
        break;
    case PhysicsMaterialPreset::Ice:
        p = { 0.05f, 0.1f, 0.9f, 0.01f, 0.01f };
        break;
    case PhysicsMaterialPreset::Concrete:
        p = { 0.7f, 0.1f, 2.4f, 0.05f, 0.05f };
        break;
    case PhysicsMaterialPreset::Glass:
        p = { 0.4f, 0.5f, 2.5f, 0.02f, 0.02f };
        break;
    case PhysicsMaterialPreset::Mud:
        p = { 0.8f, 0.05f, 1.5f, 0.3f, 0.3f };
        break;
    }
    return p;
}

PhysicsMaterialLibrary& PhysicsMaterialLibrary::Instance()
{
    static PhysicsMaterialLibrary instance;
    return instance;
}

uint32_t PhysicsMaterialLibrary::Register(const gx::String& name, const PhysicsMaterialParams& params)
{
    auto it = m_nameIndex.find(name);
    if (it != m_nameIndex.end())
    {
        // 既存エントリを更新
        m_entries[it->second].params = params;
        return it->second;
    }

    uint32_t id = static_cast<uint32_t>(m_entries.size());
    m_entries.push_back({ name, params });
    m_nameIndex[name] = id;
    return id;
}

const PhysicsMaterialParams* PhysicsMaterialLibrary::Find(const gx::String& name) const
{
    auto it = m_nameIndex.find(name);
    if (it == m_nameIndex.end()) return nullptr;
    return &m_entries[it->second].params;
}

const PhysicsMaterialParams* PhysicsMaterialLibrary::FindById(uint32_t id) const
{
    if (id >= m_entries.size()) return nullptr;
    return &m_entries[id].params;
}

void PhysicsMaterialLibrary::RegisterDefaults()
{
    Register("Default",  PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Default));
    Register("Metal",    PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Metal));
    Register("Wood",     PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Wood));
    Register("Rubber",   PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Rubber));
    Register("Ice",      PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Ice));
    Register("Concrete", PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Concrete));
    Register("Glass",    PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Glass));
    Register("Mud",      PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Mud));
}

void PhysicsMaterialLibrary::Clear()
{
    m_entries.clear();
    m_nameIndex.clear();
}

} // namespace gx
