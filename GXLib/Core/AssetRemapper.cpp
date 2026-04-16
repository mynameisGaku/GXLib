/// @file AssetRemapper.cpp
/// @brief ホットリロードRemapper実装
#include "pch_common.h"
#include "Core/AssetRemapper.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// シングルトン
// ============================================================================

AssetRemapper& AssetRemapper::Instance()
{
    static AssetRemapper instance;
    return instance;
}

// ============================================================================
// Register
// ============================================================================

void AssetRemapper::Register(const GUID& guid, const gx::String& path,
                              uint32_t slotIndex, ReloadFunc reloadFunc)
{
    RemapEntry entry;
    entry.guid = guid;
    entry.path = path;
    entry.slotIndex = slotIndex;
    entry.reloadFunc = std::move(reloadFunc);

    m_entries[guid] = std::move(entry);
    m_pathToGuid[path] = guid;
}

// ============================================================================
// Unregister
// ============================================================================

void AssetRemapper::Unregister(const GUID& guid)
{
    auto it = m_entries.find(guid);
    if (it == m_entries.end())
        return;

    m_pathToGuid.erase(it->second.path);
    m_entries.erase(it);
}

// ============================================================================
// OnFileChanged
// ============================================================================

bool AssetRemapper::OnFileChanged(const gx::String& path)
{
    auto guidIt = m_pathToGuid.find(path);
    if (guidIt == m_pathToGuid.end())
        return false;

    auto entryIt = m_entries.find(guidIt->second);
    if (entryIt == m_entries.end())
        return false;

    const auto& entry = entryIt->second;
    ++m_stats.totalReloads;

    if (entry.reloadFunc)
    {
        bool success = entry.reloadFunc(entry.path, entry.slotIndex);
        if (success)
        {
            ++m_stats.successCount;
        }
        else
        {
            ++m_stats.failureCount;
            GX_LOG_WARN("AssetRemapper: reload failed for '%s' (slot=%u)",
                        entry.path.c_str(), entry.slotIndex);
        }
        return success;
    }

    return false;
}

// ============================================================================
// GetSlot
// ============================================================================

uint32_t AssetRemapper::GetSlot(const GUID& guid) const
{
    auto it = m_entries.find(guid);
    if (it != m_entries.end())
        return it->second.slotIndex;
    return UINT32_MAX;
}

// ============================================================================
// GetPath
// ============================================================================

gx::String AssetRemapper::GetPath(const GUID& guid) const
{
    auto it = m_entries.find(guid);
    if (it != m_entries.end())
        return it->second.path;
    return gx::String();
}

// ============================================================================
// Clear
// ============================================================================

void AssetRemapper::Clear()
{
    m_entries.clear();
    m_pathToGuid.clear();
    m_stats = RemapStats{};
}

} // namespace gx
