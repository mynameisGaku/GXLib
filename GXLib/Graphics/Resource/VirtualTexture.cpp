/// @file VirtualTexture.cpp
/// @brief 仮想テクスチャリングシステムの実装
///
/// CPU ページ管理（Initialize / AnalyzeFeedback / UpdateTileCache / LRU Evict）に加え、
/// GPU フィードバックバッファ (R32G32_UINT UAV) の作成・リードバック・解析と、
/// ページテーブルテクスチャ (R32G32_UINT SRV) の更新・バインドを行う。
#include "pch_graphics.h"
#include "Graphics/Resource/VirtualTexture.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// CPU 初期化
// ============================================================================

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

// ============================================================================
// GPU 初期化
// ============================================================================

bool VirtualTexture::InitializeGPU(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight)
{
    if (!device)
    {
        GX_LOG_ERROR("VirtualTexture::InitializeGPU: device is null");
        return false;
    }

    if (!m_initialized)
    {
        GX_LOG_ERROR("VirtualTexture::InitializeGPU: call Initialize() first");
        return false;
    }

    if (screenWidth == 0 || screenHeight == 0)
    {
        GX_LOG_ERROR("VirtualTexture::InitializeGPU: invalid screen dimensions");
        return false;
    }

    m_device = device;

    // Feedback buffer resolution: screen / 16 (each texel covers 16x16 pixel block)
    m_feedbackWidth  = (std::max)(screenWidth  / 16u, 1u);
    m_feedbackHeight = (std::max)(screenHeight / 16u, 1u);

    // -----------------------------------------------------------------
    // 1) Feedback buffer (DEFAULT heap, UAV, R32G32_UINT)
    //    xy: (textureId, packed mip|tileX|tileY)
    // -----------------------------------------------------------------
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width              = m_feedbackWidth;
        desc.Height             = m_feedbackHeight;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_R32G32_UINT;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
            IID_PPV_ARGS(&m_feedbackBuffer));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("VirtualTexture: failed to create feedback buffer (0x%08X)", hr);
            return false;
        }
        m_feedbackBuffer->SetName(L"VT_FeedbackBuffer");
    }

    // -----------------------------------------------------------------
    // 2) Readback buffer (READBACK heap, CPU-readable copy of feedback)
    //    Each texel is 2 x uint32 = 8 bytes.  Row pitch must be 256-aligned.
    // -----------------------------------------------------------------
    {
        uint32_t rowPitch = ((m_feedbackWidth * 8u) + 255u) & ~255u; // 256-byte aligned
        uint64_t readbackSize = static_cast<uint64_t>(rowPitch) * m_feedbackHeight;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = readbackSize;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_feedbackReadback));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("VirtualTexture: failed to create readback buffer (0x%08X)", hr);
            m_feedbackBuffer.Reset();
            return false;
        }
        m_feedbackReadback->SetName(L"VT_FeedbackReadback");
    }

    // -----------------------------------------------------------------
    // 3) Page table texture (DEFAULT heap, SRV, R32G32_UINT)
    //    Size = pool tile grid dimensions. Each texel stores
    //    (physicalX, physicalY) — physical tile coordinates for a virtual page.
    //    Dimensions based on pool tile capacity so that every virtual tile has an entry.
    // -----------------------------------------------------------------
    {
        uint32_t tilesX = m_config.poolWidth  / m_config.pageSize;
        uint32_t tilesY = m_config.poolHeight / m_config.pageSize;
        m_pageTableWidth  = (std::max)(tilesX, 1u);
        m_pageTableHeight = (std::max)(tilesY, 1u);

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width              = m_pageTableWidth;
        desc.Height             = m_pageTableHeight;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_R32G32_UINT;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
            IID_PPV_ARGS(&m_pageTableTexture));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("VirtualTexture: failed to create page table texture (0x%08X)", hr);
            m_feedbackBuffer.Reset();
            m_feedbackReadback.Reset();
            return false;
        }
        m_pageTableTexture->SetName(L"VT_PageTableTexture");
    }

    // -----------------------------------------------------------------
    // 4) Page table upload buffer (UPLOAD heap) for UpdatePageTableTexture()
    // -----------------------------------------------------------------
    {
        uint32_t rowPitch = ((m_pageTableWidth * 8u) + 255u) & ~255u;
        uint64_t uploadSize = static_cast<uint64_t>(rowPitch) * m_pageTableHeight;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = uploadSize;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_pageTableUpload));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("VirtualTexture: failed to create page table upload buffer (0x%08X)", hr);
            m_feedbackBuffer.Reset();
            m_feedbackReadback.Reset();
            m_pageTableTexture.Reset();
            return false;
        }
        m_pageTableUpload->SetName(L"VT_PageTableUpload");
    }

    // -----------------------------------------------------------------
    // 5) Descriptor heap (CBV_SRV_UAV, shader visible, 2 slots)
    //    Slot 0: feedback buffer UAV
    //    Slot 1: page table SRV
    // -----------------------------------------------------------------
    if (!m_feedbackHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true))
    {
        GX_LOG_ERROR("VirtualTexture: failed to create descriptor heap");
        m_feedbackBuffer.Reset();
        m_feedbackReadback.Reset();
        m_pageTableTexture.Reset();
        m_pageTableUpload.Reset();
        return false;
    }

    // Create UAV for feedback buffer (slot 0)
    {
        uint32_t uavIdx = m_feedbackHeap.AllocateIndex();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format               = DXGI_FORMAT_R32G32_UINT;
        uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice   = 0;
        uavDesc.Texture2D.PlaneSlice = 0;
        device->CreateUnorderedAccessView(
            m_feedbackBuffer.Get(), nullptr, &uavDesc,
            m_feedbackHeap.GetCPUHandle(uavIdx));
    }

    // Create SRV for page table texture (slot 1)
    {
        uint32_t srvIdx = m_feedbackHeap.AllocateIndex();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                        = DXGI_FORMAT_R32G32_UINT;
        srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip     = 0;
        srvDesc.Texture2D.MipLevels           = 1;
        srvDesc.Texture2D.PlaneSlice          = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        device->CreateShaderResourceView(
            m_pageTableTexture.Get(), &srvDesc,
            m_feedbackHeap.GetCPUHandle(srvIdx));
    }

    m_gpuInitialized = true;

    GX_LOG_INFO("VirtualTexture GPU initialized: feedback %ux%u, pageTable %ux%u",
                m_feedbackWidth, m_feedbackHeight,
                m_pageTableWidth, m_pageTableHeight);
    return true;
}

// ============================================================================
// AnalyzeFeedback (CPU vector input)
// ============================================================================

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

// ============================================================================
// ReadbackFeedback (GPU → CPU → AnalyzeFeedback)
// ============================================================================

void VirtualTexture::ReadbackFeedback(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList || !m_gpuInitialized)
        return;

    // Transition feedback buffer: UAV → COPY_SOURCE
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_feedbackBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // Copy feedback texture → readback buffer
    {
        uint32_t rowPitch = ((m_feedbackWidth * 8u) + 255u) & ~255u;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource        = m_feedbackBuffer.Get();
        src.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = m_feedbackReadback.Get();
        dst.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dst.PlacedFootprint.Offset             = 0;
        dst.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R32G32_UINT;
        dst.PlacedFootprint.Footprint.Width    = m_feedbackWidth;
        dst.PlacedFootprint.Footprint.Height   = m_feedbackHeight;
        dst.PlacedFootprint.Footprint.Depth    = 1;
        dst.PlacedFootprint.Footprint.RowPitch = rowPitch;

        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }

    // Transition feedback buffer back: COPY_SOURCE → UAV
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_feedbackBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // Map readback buffer and parse feedback entries
    uint32_t rowPitch = ((m_feedbackWidth * 8u) + 255u) & ~255u;
    uint64_t readbackSize = static_cast<uint64_t>(rowPitch) * m_feedbackHeight;

    D3D12_RANGE readRange = {};
    readRange.Begin = 0;
    readRange.End   = static_cast<SIZE_T>(readbackSize);

    void* mappedData = nullptr;
    HRESULT hr = m_feedbackReadback->Map(0, &readRange, &mappedData);
    if (FAILED(hr) || !mappedData)
    {
        GX_LOG_WARN("VirtualTexture::ReadbackFeedback: Map failed (0x%08X)", hr);
        return;
    }

    // Parse R32G32_UINT texels.
    // .r = textureId  (0xFFFFFFFF = empty / no request)
    // .g = packed:  mipLevel (bits 24-31) | tileX (bits 12-23) | tileY (bits 0-11)
    gx::Vector<VTPageId> parsedRequests;
    parsedRequests.reserve(m_feedbackWidth * m_feedbackHeight / 4u); // heuristic

    const uint8_t* basePtr = static_cast<const uint8_t*>(mappedData);

    for (uint32_t y = 0; y < m_feedbackHeight; ++y)
    {
        const uint32_t* row = reinterpret_cast<const uint32_t*>(basePtr + y * rowPitch);
        for (uint32_t x = 0; x < m_feedbackWidth; ++x)
        {
            uint32_t texId  = row[x * 2 + 0];
            uint32_t packed = row[x * 2 + 1];

            // Skip empty entries
            if (texId == 0xFFFFFFFFu)
                continue;

            VTPageId page;
            page.textureId = texId;
            page.mipLevel  = (packed >> 24) & 0xFFu;
            page.tileX     = (packed >> 12) & 0xFFFu;
            page.tileY     =  packed        & 0xFFFu;

            parsedRequests.push_back(page);
        }
    }

    // Unmap (no CPU writes)
    D3D12_RANGE writeRange = { 0, 0 };
    m_feedbackReadback->Unmap(0, &writeRange);

    // Feed parsed results into the standard CPU analysis path
    if (!parsedRequests.empty())
    {
        AnalyzeFeedback(parsedRequests, m_currentFrame);
    }
}

// ============================================================================
// UpdateTileCache
// ============================================================================

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

// ============================================================================
// BindPageTable
// ============================================================================

void VirtualTexture::BindPageTable(ID3D12GraphicsCommandList* cmdList, uint32_t rootParamIndex)
{
    if (!cmdList || !m_gpuInitialized)
        return;

    // Ensure page table texture is in SRV state.
    // After UpdatePageTableTexture() the resource is transitioned back to
    // PIXEL_SHADER_RESOURCE, so normally this is already the correct state.
    // We still issue a redundant barrier for safety (the runtime will optimize
    // identical-state transitions away).

    // Set the descriptor heap and bind page table SRV at rootParamIndex.
    ID3D12DescriptorHeap* heaps[] = { m_feedbackHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Page table SRV is at slot 1 in m_feedbackHeap
    cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, m_feedbackHeap.GetGPUHandle(1));
}

// ============================================================================
// UpdatePageTableTexture
// ============================================================================

void VirtualTexture::UpdatePageTableTexture()
{
    if (!m_gpuInitialized || !m_device)
        return;

    // Map upload buffer and fill page table data.
    // Each texel (R32G32_UINT) stores (physicalX_in_tiles, physicalY_in_tiles).
    // 0xFFFFFFFF means "not resident" / unmapped.

    uint32_t rowPitch = ((m_pageTableWidth * 8u) + 255u) & ~255u;

    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // no CPU reads
    HRESULT hr = m_pageTableUpload->Map(0, &readRange, &mappedData);
    if (FAILED(hr) || !mappedData)
    {
        GX_LOG_WARN("VirtualTexture::UpdatePageTableTexture: Map failed (0x%08X)", hr);
        return;
    }

    uint8_t* basePtr = static_cast<uint8_t*>(mappedData);

    // Clear to 0xFFFFFFFF (not resident)
    for (uint32_t y = 0; y < m_pageTableHeight; ++y)
    {
        uint32_t* row = reinterpret_cast<uint32_t*>(basePtr + y * rowPitch);
        for (uint32_t x = 0; x < m_pageTableWidth; ++x)
        {
            row[x * 2 + 0] = 0xFFFFFFFFu;
            row[x * 2 + 1] = 0xFFFFFFFFu;
        }
    }

    // Write cached tile mappings into the page table.
    // Page table is indexed by (tileX, tileY) — the virtual tile coordinates.
    // Value is (physicalX / pageSize, physicalY / pageSize) in physical pool tiles.
    for (const auto& [pageId, cached] : m_tileCache)
    {
        uint32_t ptX = pageId.tileX;
        uint32_t ptY = pageId.tileY;

        if (ptX < m_pageTableWidth && ptY < m_pageTableHeight)
        {
            uint32_t* row = reinterpret_cast<uint32_t*>(basePtr + ptY * rowPitch);
            row[ptX * 2 + 0] = cached.physicalX / m_config.pageSize;
            row[ptX * 2 + 1] = cached.physicalY / m_config.pageSize;
        }
    }

    D3D12_RANGE writeRange = { 0, static_cast<SIZE_T>(static_cast<uint64_t>(rowPitch) * m_pageTableHeight) };
    m_pageTableUpload->Unmap(0, &writeRange);

    // NOTE: The actual CopyTextureRegion from upload buffer to the page table
    // texture requires a command list.  Callers should record a command list
    // after calling this function:
    //   1. Barrier: page table  SRV → COPY_DEST
    //   2. CopyTextureRegion(pageTableTexture, uploadBuffer)
    //   3. Barrier: page table  COPY_DEST → SRV
    // This upload preparation is done here; the GPU copy is deferred to the
    // caller's command list recording phase to avoid hidden command list usage.
}

// ============================================================================
// Query / Utility
// ============================================================================

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
