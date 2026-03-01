/// @file PrefabVariant.cpp
/// @brief PrefabVariant の実装
#include "pch_common.h"
#include "Core/Scene/PrefabVariant.h"
#include "Core/Logger.h"

#include <fstream>
#include <unordered_set>

namespace gx
{

// ============================================================================
// Prefab management
// ============================================================================

uint64_t PrefabVariantSystem::RegisterPrefab(const PrefabData& data)
{
    PrefabData copy = data;
    if (copy.prefabId == 0)
    {
        copy.prefabId = m_nextPrefabId++;
    }
    else
    {
        // Ensure next ID stays ahead of manually assigned IDs
        if (copy.prefabId >= m_nextPrefabId)
            m_nextPrefabId = copy.prefabId + 1;
    }

    uint64_t id = copy.prefabId;
    m_prefabs[id] = std::move(copy);
    return id;
}

bool PrefabVariantSystem::UnregisterPrefab(uint64_t prefabId)
{
    auto it = m_prefabs.find(prefabId);
    if (it == m_prefabs.end())
        return false;

    m_prefabs.erase(it);

    // Also remove any variants that reference this prefab
    for (auto vit = m_variants.begin(); vit != m_variants.end(); )
    {
        if (vit->second.basePrefabId == prefabId)
            vit = m_variants.erase(vit);
        else
            ++vit;
    }

    return true;
}

const PrefabData* PrefabVariantSystem::GetPrefab(uint64_t prefabId) const
{
    auto it = m_prefabs.find(prefabId);
    if (it == m_prefabs.end())
        return nullptr;
    return &it->second;
}

size_t PrefabVariantSystem::GetPrefabCount() const
{
    return m_prefabs.size();
}

bool PrefabVariantSystem::HasPrefab(uint64_t prefabId) const
{
    return m_prefabs.find(prefabId) != m_prefabs.end();
}

// ============================================================================
// Variant management
// ============================================================================

uint64_t PrefabVariantSystem::CreateVariant(uint64_t basePrefabId, const std::string& name)
{
    // Base can be either a prefab or another variant
    if (!HasPrefab(basePrefabId) && !HasVariant(basePrefabId))
        return 0;

    PrefabVariantData variant;
    variant.variantId = m_nextVariantId++;
    variant.basePrefabId = basePrefabId;
    variant.name = name;

    uint64_t id = variant.variantId;
    m_variants[id] = std::move(variant);
    return id;
}

bool PrefabVariantSystem::DeleteVariant(uint64_t variantId)
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return false;

    m_variants.erase(it);
    return true;
}

const PrefabVariantData* PrefabVariantSystem::GetVariant(uint64_t variantId) const
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return nullptr;
    return &it->second;
}

size_t PrefabVariantSystem::GetVariantCount() const
{
    return m_variants.size();
}

bool PrefabVariantSystem::HasVariant(uint64_t variantId) const
{
    return m_variants.find(variantId) != m_variants.end();
}

std::vector<uint64_t> PrefabVariantSystem::GetVariantsOf(uint64_t basePrefabId) const
{
    std::vector<uint64_t> result;
    for (const auto& [id, variant] : m_variants)
    {
        if (variant.basePrefabId == basePrefabId)
            result.push_back(id);
    }
    return result;
}

// ============================================================================
// Override management
// ============================================================================

ComponentOverride* PrefabVariantSystem::FindOrCreateComponentOverride(
    PrefabVariantData& variant, const std::string& componentName)
{
    for (auto& co : variant.overrides)
    {
        if (co.componentName == componentName)
            return &co;
    }

    // Create new component override entry
    ComponentOverride newOverride;
    newOverride.componentName = componentName;
    variant.overrides.push_back(std::move(newOverride));
    return &variant.overrides.back();
}

const ComponentOverride* PrefabVariantSystem::FindComponentOverride(
    const PrefabVariantData& variant, const std::string& componentName) const
{
    for (const auto& co : variant.overrides)
    {
        if (co.componentName == componentName)
            return &co;
    }
    return nullptr;
}

bool PrefabVariantSystem::AddPropertyOverride(uint64_t variantId, const std::string& componentName,
                                               const std::string& propertyName, const std::string& value)
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return false;

    auto* compOverride = FindOrCreateComponentOverride(it->second, componentName);

    // Check if property already overridden, update if so
    for (auto& prop : compOverride->properties)
    {
        if (prop.propertyName == propertyName)
        {
            prop.value = value;
            prop.isRemoved = false;
            return true;
        }
    }

    // Add new property override
    PropertyOverride propOverride;
    propOverride.componentName = componentName;
    propOverride.propertyName = propertyName;
    propOverride.value = value;
    compOverride->properties.push_back(std::move(propOverride));
    return true;
}

bool PrefabVariantSystem::RemovePropertyOverride(uint64_t variantId, const std::string& componentName,
                                                  const std::string& propertyName)
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return false;

    for (auto& co : it->second.overrides)
    {
        if (co.componentName == componentName)
        {
            for (auto pit = co.properties.begin(); pit != co.properties.end(); ++pit)
            {
                if (pit->propertyName == propertyName)
                {
                    co.properties.erase(pit);

                    // If no properties and not a component-level override, remove the component override
                    if (co.properties.empty() && !co.isAdded && !co.isRemoved)
                    {
                        for (auto cit = it->second.overrides.begin();
                             cit != it->second.overrides.end(); ++cit)
                        {
                            if (cit->componentName == componentName)
                            {
                                it->second.overrides.erase(cit);
                                break;
                            }
                        }
                    }
                    return true;
                }
            }
            break;
        }
    }
    return false;
}

bool PrefabVariantSystem::AddComponentOverride(uint64_t variantId, const std::string& componentName, bool isAdded)
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return false;

    auto* compOverride = FindOrCreateComponentOverride(it->second, componentName);
    compOverride->isAdded = isAdded;
    compOverride->isRemoved = !isAdded;
    return true;
}

bool PrefabVariantSystem::RemoveComponentOverride(uint64_t variantId, const std::string& componentName)
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return false;

    for (auto cit = it->second.overrides.begin(); cit != it->second.overrides.end(); ++cit)
    {
        if (cit->componentName == componentName)
        {
            it->second.overrides.erase(cit);
            return true;
        }
    }
    return false;
}

std::vector<ComponentOverride> PrefabVariantSystem::GetOverrides(uint64_t variantId) const
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return {};
    return it->second.overrides;
}

size_t PrefabVariantSystem::GetOverrideCount(uint64_t variantId) const
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return 0;

    size_t count = 0;
    for (const auto& co : it->second.overrides)
    {
        count += co.properties.size();
        // Count component-level overrides (add/remove) as one override each
        if (co.isAdded || co.isRemoved)
            ++count;
    }
    return count;
}

bool PrefabVariantSystem::IsOverridden(uint64_t variantId, const std::string& componentName,
                                        const std::string& propertyName) const
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return false;

    const auto* compOverride = FindComponentOverride(it->second, componentName);
    if (!compOverride)
        return false;

    for (const auto& prop : compOverride->properties)
    {
        if (prop.propertyName == propertyName)
            return true;
    }
    return false;
}

bool PrefabVariantSystem::RevertOverride(uint64_t variantId, const std::string& componentName,
                                          const std::string& propertyName)
{
    return RemovePropertyOverride(variantId, componentName, propertyName);
}

void PrefabVariantSystem::RevertAllOverrides(uint64_t variantId)
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return;
    it->second.overrides.clear();
    it->second.childOverrides.clear();
}

std::vector<uint8_t> PrefabVariantSystem::ApplyOverrides(uint64_t variantId,
                                                          const std::vector<uint8_t>& targetData) const
{
    auto it = m_variants.find(variantId);
    if (it == m_variants.end())
        return targetData;

    // Build the full inheritance chain of overrides
    std::vector<uint64_t> chain = GetInheritanceChain(variantId);

    // Start with the base data
    std::vector<uint8_t> result = targetData;

    // Apply overrides from each level in the chain (skip the root prefab which is first)
    for (size_t i = 1; i < chain.size(); ++i)
    {
        auto vit = m_variants.find(chain[i]);
        if (vit == m_variants.end())
            continue;

        const auto& variant = vit->second;

        // For each component override, encode it as a simple marker in the data
        for (const auto& co : variant.overrides)
        {
            // Append override data as simple tagged format:
            // [TAG:1 byte] [componentName len:4] [componentName] [propName len:4] [propName] [value len:4] [value]
            for (const auto& prop : co.properties)
            {
                if (prop.isRemoved)
                    continue;

                // Tag byte: 0x01 = property override
                result.push_back(0x01);

                uint32_t compLen = static_cast<uint32_t>(prop.componentName.size());
                const uint8_t* compLenBytes = reinterpret_cast<const uint8_t*>(&compLen);
                result.insert(result.end(), compLenBytes, compLenBytes + 4);
                result.insert(result.end(), prop.componentName.begin(), prop.componentName.end());

                uint32_t propLen = static_cast<uint32_t>(prop.propertyName.size());
                const uint8_t* propLenBytes = reinterpret_cast<const uint8_t*>(&propLen);
                result.insert(result.end(), propLenBytes, propLenBytes + 4);
                result.insert(result.end(), prop.propertyName.begin(), prop.propertyName.end());

                uint32_t valLen = static_cast<uint32_t>(prop.value.size());
                const uint8_t* valLenBytes = reinterpret_cast<const uint8_t*>(&valLen);
                result.insert(result.end(), valLenBytes, valLenBytes + 4);
                result.insert(result.end(), prop.value.begin(), prop.value.end());
            }

            // Component add/remove markers
            if (co.isAdded)
            {
                result.push_back(0x02); // Tag: component added
                uint32_t nameLen = static_cast<uint32_t>(co.componentName.size());
                const uint8_t* nameLenBytes = reinterpret_cast<const uint8_t*>(&nameLen);
                result.insert(result.end(), nameLenBytes, nameLenBytes + 4);
                result.insert(result.end(), co.componentName.begin(), co.componentName.end());
            }
            else if (co.isRemoved)
            {
                result.push_back(0x03); // Tag: component removed
                uint32_t nameLen = static_cast<uint32_t>(co.componentName.size());
                const uint8_t* nameLenBytes = reinterpret_cast<const uint8_t*>(&nameLen);
                result.insert(result.end(), nameLenBytes, nameLenBytes + 4);
                result.insert(result.end(), co.componentName.begin(), co.componentName.end());
            }
        }
    }

    return result;
}

// ============================================================================
// Nested variants
// ============================================================================

uint64_t PrefabVariantSystem::CreateNestedVariant(uint64_t parentVariantId, const std::string& name)
{
    if (!HasVariant(parentVariantId))
        return 0;

    PrefabVariantData variant;
    variant.variantId = m_nextVariantId++;
    variant.basePrefabId = parentVariantId;  // Parent is a variant, not a prefab
    variant.name = name;

    uint64_t id = variant.variantId;
    m_variants[id] = std::move(variant);
    return id;
}

std::vector<uint64_t> PrefabVariantSystem::GetInheritanceChain(uint64_t variantId) const
{
    std::vector<uint64_t> chain;
    std::unordered_set<uint64_t> visited;

    // Walk up the inheritance chain
    uint64_t currentId = variantId;
    while (true)
    {
        // Prevent infinite loops
        if (visited.count(currentId) > 0)
            break;
        visited.insert(currentId);

        chain.push_back(currentId);

        // If current is a prefab, we've reached the root
        if (HasPrefab(currentId) && m_variants.find(currentId) == m_variants.end())
            break;

        // Check if current is a variant
        auto vit = m_variants.find(currentId);
        if (vit != m_variants.end())
        {
            currentId = vit->second.basePrefabId;
            continue;
        }

        // Neither variant nor prefab - broken chain
        break;
    }

    // Reverse so root prefab is first
    std::reverse(chain.begin(), chain.end());
    return chain;
}

// ============================================================================
// Serialization
// ============================================================================

bool PrefabVariantSystem::Save(const std::string& filePath) const
{
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open())
        return false;

    // Write header magic
    const char magic[] = "GXPV";
    file.write(magic, 4);

    // Write version
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Write prefab count
    uint32_t prefabCount = static_cast<uint32_t>(m_prefabs.size());
    file.write(reinterpret_cast<const char*>(&prefabCount), sizeof(prefabCount));

    for (const auto& [id, prefab] : m_prefabs)
    {
        file.write(reinterpret_cast<const char*>(&prefab.prefabId), sizeof(prefab.prefabId));

        uint32_t nameLen = static_cast<uint32_t>(prefab.name.size());
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(prefab.name.data(), nameLen);

        uint32_t dataLen = static_cast<uint32_t>(prefab.entityData.size());
        file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));
        if (dataLen > 0)
            file.write(reinterpret_cast<const char*>(prefab.entityData.data()), dataLen);

        uint32_t childCount = static_cast<uint32_t>(prefab.childPrefabIds.size());
        file.write(reinterpret_cast<const char*>(&childCount), sizeof(childCount));
        for (uint64_t childId : prefab.childPrefabIds)
            file.write(reinterpret_cast<const char*>(&childId), sizeof(childId));
    }

    // Write variant count
    uint32_t variantCount = static_cast<uint32_t>(m_variants.size());
    file.write(reinterpret_cast<const char*>(&variantCount), sizeof(variantCount));

    for (const auto& [id, variant] : m_variants)
    {
        file.write(reinterpret_cast<const char*>(&variant.variantId), sizeof(variant.variantId));
        file.write(reinterpret_cast<const char*>(&variant.basePrefabId), sizeof(variant.basePrefabId));

        uint32_t nameLen = static_cast<uint32_t>(variant.name.size());
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(variant.name.data(), nameLen);

        // Write overrides
        uint32_t overrideCount = static_cast<uint32_t>(variant.overrides.size());
        file.write(reinterpret_cast<const char*>(&overrideCount), sizeof(overrideCount));

        for (const auto& co : variant.overrides)
        {
            uint32_t compNameLen = static_cast<uint32_t>(co.componentName.size());
            file.write(reinterpret_cast<const char*>(&compNameLen), sizeof(compNameLen));
            file.write(co.componentName.data(), compNameLen);

            uint8_t flags = 0;
            if (co.isAdded) flags |= 0x01;
            if (co.isRemoved) flags |= 0x02;
            file.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

            uint32_t propCount = static_cast<uint32_t>(co.properties.size());
            file.write(reinterpret_cast<const char*>(&propCount), sizeof(propCount));

            for (const auto& prop : co.properties)
            {
                uint32_t pCompLen = static_cast<uint32_t>(prop.componentName.size());
                file.write(reinterpret_cast<const char*>(&pCompLen), sizeof(pCompLen));
                file.write(prop.componentName.data(), pCompLen);

                uint32_t pPropLen = static_cast<uint32_t>(prop.propertyName.size());
                file.write(reinterpret_cast<const char*>(&pPropLen), sizeof(pPropLen));
                file.write(prop.propertyName.data(), pPropLen);

                uint32_t pValLen = static_cast<uint32_t>(prop.value.size());
                file.write(reinterpret_cast<const char*>(&pValLen), sizeof(pValLen));
                file.write(prop.value.data(), pValLen);

                uint8_t removed = prop.isRemoved ? 1 : 0;
                file.write(reinterpret_cast<const char*>(&removed), sizeof(removed));
            }
        }
    }

    return file.good();
}

bool PrefabVariantSystem::Load(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
        return false;

    // Read and verify magic
    char magic[4] = {};
    file.read(magic, 4);
    if (magic[0] != 'G' || magic[1] != 'X' || magic[2] != 'P' || magic[3] != 'V')
        return false;

    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1)
        return false;

    Clear();

    // Read prefabs
    uint32_t prefabCount = 0;
    file.read(reinterpret_cast<char*>(&prefabCount), sizeof(prefabCount));

    for (uint32_t i = 0; i < prefabCount; ++i)
    {
        PrefabData prefab;
        file.read(reinterpret_cast<char*>(&prefab.prefabId), sizeof(prefab.prefabId));

        uint32_t nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        prefab.name.resize(nameLen);
        if (nameLen > 0)
            file.read(prefab.name.data(), nameLen);

        uint32_t dataLen = 0;
        file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));
        prefab.entityData.resize(dataLen);
        if (dataLen > 0)
            file.read(reinterpret_cast<char*>(prefab.entityData.data()), dataLen);

        uint32_t childCount = 0;
        file.read(reinterpret_cast<char*>(&childCount), sizeof(childCount));
        prefab.childPrefabIds.resize(childCount);
        for (uint32_t c = 0; c < childCount; ++c)
            file.read(reinterpret_cast<char*>(&prefab.childPrefabIds[c]), sizeof(uint64_t));

        if (prefab.prefabId >= m_nextPrefabId)
            m_nextPrefabId = prefab.prefabId + 1;

        m_prefabs[prefab.prefabId] = std::move(prefab);
    }

    // Read variants
    uint32_t variantCount = 0;
    file.read(reinterpret_cast<char*>(&variantCount), sizeof(variantCount));

    for (uint32_t i = 0; i < variantCount; ++i)
    {
        PrefabVariantData variant;
        file.read(reinterpret_cast<char*>(&variant.variantId), sizeof(variant.variantId));
        file.read(reinterpret_cast<char*>(&variant.basePrefabId), sizeof(variant.basePrefabId));

        uint32_t nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        variant.name.resize(nameLen);
        if (nameLen > 0)
            file.read(variant.name.data(), nameLen);

        uint32_t overrideCount = 0;
        file.read(reinterpret_cast<char*>(&overrideCount), sizeof(overrideCount));

        for (uint32_t o = 0; o < overrideCount; ++o)
        {
            ComponentOverride co;

            uint32_t compNameLen = 0;
            file.read(reinterpret_cast<char*>(&compNameLen), sizeof(compNameLen));
            co.componentName.resize(compNameLen);
            if (compNameLen > 0)
                file.read(co.componentName.data(), compNameLen);

            uint8_t flags = 0;
            file.read(reinterpret_cast<char*>(&flags), sizeof(flags));
            co.isAdded = (flags & 0x01) != 0;
            co.isRemoved = (flags & 0x02) != 0;

            uint32_t propCount = 0;
            file.read(reinterpret_cast<char*>(&propCount), sizeof(propCount));

            for (uint32_t p = 0; p < propCount; ++p)
            {
                PropertyOverride prop;

                uint32_t pCompLen = 0;
                file.read(reinterpret_cast<char*>(&pCompLen), sizeof(pCompLen));
                prop.componentName.resize(pCompLen);
                if (pCompLen > 0)
                    file.read(prop.componentName.data(), pCompLen);

                uint32_t pPropLen = 0;
                file.read(reinterpret_cast<char*>(&pPropLen), sizeof(pPropLen));
                prop.propertyName.resize(pPropLen);
                if (pPropLen > 0)
                    file.read(prop.propertyName.data(), pPropLen);

                uint32_t pValLen = 0;
                file.read(reinterpret_cast<char*>(&pValLen), sizeof(pValLen));
                prop.value.resize(pValLen);
                if (pValLen > 0)
                    file.read(prop.value.data(), pValLen);

                uint8_t removed = 0;
                file.read(reinterpret_cast<char*>(&removed), sizeof(removed));
                prop.isRemoved = (removed != 0);

                co.properties.push_back(std::move(prop));
            }

            variant.overrides.push_back(std::move(co));
        }

        if (variant.variantId >= m_nextVariantId)
            m_nextVariantId = variant.variantId + 1;

        m_variants[variant.variantId] = std::move(variant);
    }

    return file.good();
}

void PrefabVariantSystem::Clear()
{
    m_prefabs.clear();
    m_variants.clear();
    m_nextPrefabId = 1;
    m_nextVariantId = 0x100000;
}

} // namespace gx
