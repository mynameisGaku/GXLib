/// @file VirtualTexture.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Graphics/Resource/VirtualTexture.h"
#include "Core/Logger.h"

namespace gx
{

bool VirtualTexture::Initialize(ID3D12Device* device, const VirtualTextureConfig& config)
{
    m_config = config;

    if (config.pageSize == 0 || config.poolWidth == 0 || config.poolHeight == 0)
    {
        GX_LOG_ERROR("VirtualTexture::Initialize: invalid config");
        return false;
    }

    uint32_t tilesX = config.poolWidth / config.pageSize;
    uint32_t tilesY = config.poolHeight / config.pageSize;
    m_physicalTileUsed.resize(tilesX * tilesY, false);

    m_tileCache.clear();
    m_pendingRequests.clear();
    m_initialized = true;

    GX_LOG_INFO("VirtualTexture initialized: %ux%u pool, %u tile capacity",
                config.poolWidth, config.poolHeight, GetPoolTileCapacity());
    return true;
}

void VirtualTexture::AnalyzeFeedback(const gx::Vector<VTPageId>& requestedPages, uint64_t frameIndex)
{
    m_currentFrame = frameIndex;

    for (const auto& page : requestedPages)
    {
        auto it = m_tileCache.find(page);
        if (it != m_tileCache.end())
        {
            it->second.lastAccessFrame = frameIndex;
            continue;
        }

        // Not cached - add to pending if not already
        bool alreadyPending = false;
        for (const auto& p : m_pendingRequests)
        {
            if (p == page) { alreadyPending = true; break; }
        }
        if (!alreadyPending)
            m_pendingRequests.push_back(page);
    }
}

uint32_t VirtualTexture::UpdateTileCache()
{
    if (!m_initialized) return 0;

    uint32_t loaded = 0;
    uint32_t maxPerFrame = m_config.maxTilesPerFrame;

    while (!m_pendingRequests.empty() && loaded < maxPerFrame)
    {
        VTPageId page = m_pendingRequests.front();
        m_pendingRequests.pop_front();

        // Allocate physical tile
        uint32_t physIndex = AllocatePhysicalTile();
        if (physIndex == UINT32_MAX)
        {
            EvictOldestTile();
            physIndex = AllocatePhysicalTile();
            if (physIndex == UINT32_MAX)
                break;
        }

        uint32_t tilesX = m_config.poolWidth / m_config.pageSize;
        CachedTile tile;
        tile.pageId = page;
        tile.physicalX = (physIndex % tilesX) * m_config.pageSize;
        tile.physicalY = (physIndex / tilesX) * m_config.pageSize;
        tile.lastAccessFrame = m_currentFrame;

        m_tileCache[page] = tile;
        loaded++;
    }

    return loaded;
}

void VirtualTexture::BindPageTable(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList || !m_initialized) return;
    // In actual implementation, would bind page table texture as SRV
}

bool VirtualTexture::IsTileCached(const VTPageId& pageId) const
{
    return m_tileCache.find(pageId) != m_tileCache.end();
}

uint32_t VirtualTexture::GetPoolTileCapacity() const
{
    if (m_config.pageSize == 0) return 0;
    uint32_t tilesX = m_config.poolWidth / m_config.pageSize;
    uint32_t tilesY = m_config.poolHeight / m_config.pageSize;
    return tilesX * tilesY;
}

void VirtualTexture::ClearCache()
{
    m_tileCache.clear();
    m_pendingRequests.clear();
    std::fill(m_physicalTileUsed.begin(), m_physicalTileUsed.end(), false);
}

uint32_t VirtualTexture::AllocatePhysicalTile()
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_physicalTileUsed.size()); i++)
    {
        if (!m_physicalTileUsed[i])
        {
            m_physicalTileUsed[i] = true;
            return i;
        }
    }
    return UINT32_MAX;
}

void VirtualTexture::EvictOldestTile()
{
    if (m_tileCache.empty()) return;

    auto oldest = m_tileCache.begin();
    for (auto it = m_tileCache.begin(); it != m_tileCache.end(); ++it)
    {
        if (it->second.lastAccessFrame < oldest->second.lastAccessFrame)
            oldest = it;
    }

    uint32_t tilesX = m_config.poolWidth / m_config.pageSize;
    uint32_t tileIndexX = oldest->second.physicalX / m_config.pageSize;
    uint32_t tileIndexY = oldest->second.physicalY / m_config.pageSize;
    uint32_t physIndex = tileIndexY * tilesX + tileIndexX;

    if (physIndex < static_cast<uint32_t>(m_physicalTileUsed.size()))
        m_physicalTileUsed[physIndex] = false;

    m_tileCache.erase(oldest);
}

} // namespace gx
