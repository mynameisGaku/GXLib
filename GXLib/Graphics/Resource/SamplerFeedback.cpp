/// @file SamplerFeedback.cpp
/// @brief サンプラーフィードバックの実装
#include "pch_graphics.h"
#include "Graphics/Resource/SamplerFeedback.h"
#include "Core/Logger.h"

namespace gx
{

bool SamplerFeedback::Initialize(ID3D12Device* device, uint32_t maxTrackedTextures)
{
    if (!device)
    {
        GX_LOG_ERROR("SamplerFeedback::Initialize: device is null");
        return false;
    }

    m_tier = SamplerFeedbackTier::None;
    m_maxTracked = maxTrackedTextures;
    m_trackedCount = 0;
    m_mipRequests.clear();

    // サンプラーフィードバック対応レベルを検出 (D3D12_FEATURE_DATA_D3D12_OPTIONS7)
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    HRESULT hr = device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

    if (SUCCEEDED(hr))
    {
        if (options7.SamplerFeedbackTier >= D3D12_SAMPLER_FEEDBACK_TIER_1_0)
        {
            m_tier = SamplerFeedbackTier::Tier1_0;
            GX_LOG_INFO("Sampler Feedback Tier 1.0 supported (MinMip + MipRegionUsed)");
        }
        else if (options7.SamplerFeedbackTier >= D3D12_SAMPLER_FEEDBACK_TIER_0_9)
        {
            m_tier = SamplerFeedbackTier::Tier0_9;
            GX_LOG_INFO("Sampler Feedback Tier 0.9 supported (MinMip only)");
        }
        else
        {
            GX_LOG_INFO("Sampler Feedback not supported on this GPU");
        }
    }
    else
    {
        GX_LOG_INFO("Sampler Feedback feature query failed -- not available");
    }

    // 追跡配列を事前確保
    if (m_tier != SamplerFeedbackTier::None && maxTrackedTextures > 0)
    {
        m_feedbackMaps.resize(maxTrackedTextures);
        m_resolveBuffers.resize(maxTrackedTextures);
        m_mipRequests.reserve(maxTrackedTextures);
    }

    return true;
}

bool SamplerFeedback::CreateFeedbackFor(ID3D12Device* device, uint32_t texIndex,
                                         ID3D12Resource* pairedTexture)
{
    if (!device || !pairedTexture)
    {
        GX_LOG_ERROR("SamplerFeedback::CreateFeedbackFor: invalid parameters");
        return false;
    }

    if (m_tier == SamplerFeedbackTier::None)
    {
        GX_LOG_WARN("Sampler Feedback not available -- cannot create feedback map");
        return false;
    }

    if (texIndex >= m_maxTracked)
    {
        GX_LOG_ERROR("SamplerFeedback: texIndex %u exceeds max tracked (%u)", texIndex, m_maxTracked);
        return false;
    }

    // 対象テクスチャの情報を取得
    D3D12_RESOURCE_DESC texDesc = pairedTexture->GetDesc();

    // フィードバックマップの解像度はタイル粒度で決まる
    // MinMipフィードバックの場合、テクスチャの各64x64タイルに対して最小ミップレベルを記録
    // ここではテクスチャの幅・高さを64で割った解像度を使用
    uint32_t feedbackWidth  = static_cast<uint32_t>((texDesc.Width  + 63) / 64);
    uint32_t feedbackHeight = (texDesc.Height + 63) / 64;

    if (feedbackWidth == 0)  feedbackWidth  = 1;
    if (feedbackHeight == 0) feedbackHeight = 1;

    // フィードバックマップリソースを作成
    // MinMipフィードバックはR8_UINTで各テクセルが最小使用ミップレベルを格納
    D3D12_RESOURCE_DESC fbDesc = {};
    fbDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    fbDesc.Width              = feedbackWidth;
    fbDesc.Height             = feedbackHeight;
    fbDesc.DepthOrArraySize   = 1;
    fbDesc.MipLevels          = 1;
    fbDesc.Format             = DXGI_FORMAT_R8_UINT;
    fbDesc.SampleDesc.Count   = 1;
    fbDesc.SampleDesc.Quality = 0;
    fbDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    fbDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &fbDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&m_feedbackMaps[texIndex])
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create feedback map for texture %u (HRESULT: 0x%08X)", texIndex, hr);
        return false;
    }

    // リゾルブ先バッファ（READBACKヒープ、CPU読み取り用）
    uint64_t resolveSize = static_cast<uint64_t>(feedbackWidth) * feedbackHeight * sizeof(uint8_t);
    // 256バイトアラインメント
    resolveSize = (resolveSize + 255) & ~255ULL;

    D3D12_RESOURCE_DESC resolveDesc = {};
    resolveDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resolveDesc.Width              = resolveSize;
    resolveDesc.Height             = 1;
    resolveDesc.DepthOrArraySize   = 1;
    resolveDesc.MipLevels          = 1;
    resolveDesc.Format             = DXGI_FORMAT_UNKNOWN;
    resolveDesc.SampleDesc.Count   = 1;
    resolveDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resolveDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES readbackHeap = {};
    readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;

    hr = device->CreateCommittedResource(
        &readbackHeap,
        D3D12_HEAP_FLAG_NONE,
        &resolveDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_resolveBuffers[texIndex])
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create resolve buffer for texture %u (HRESULT: 0x%08X)", texIndex, hr);
        m_feedbackMaps[texIndex].Reset();
        return false;
    }

    if (texIndex >= m_trackedCount)
    {
        m_trackedCount = texIndex + 1;
    }

    GX_LOG_INFO("Sampler Feedback map created for texture %u (%ux%u tiles)",
                texIndex, feedbackWidth, feedbackHeight);
    return true;
}

void SamplerFeedback::ResolveFeedback(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList || m_tier == SamplerFeedbackTier::None)
        return;

    // 各フィードバックマップをリゾルブバッファにコピーする
    for (uint32_t i = 0; i < m_trackedCount; ++i)
    {
        if (!m_feedbackMaps[i] || !m_resolveBuffers[i])
            continue;

        // フィードバックマップを COPY_SOURCE に遷移
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_feedbackMaps[i].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        // テクスチャからバッファへコピー
        D3D12_RESOURCE_DESC fbDesc = m_feedbackMaps[i]->GetDesc();

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource        = m_feedbackMaps[i].Get();
        src.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = m_resolveBuffers[i].Get();
        dst.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dst.PlacedFootprint.Offset = 0;
        dst.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8_UINT;
        dst.PlacedFootprint.Footprint.Width    = static_cast<UINT>(fbDesc.Width);
        dst.PlacedFootprint.Footprint.Height   = fbDesc.Height;
        dst.PlacedFootprint.Footprint.Depth    = 1;
        // RowPitch must be 256-byte aligned
        dst.PlacedFootprint.Footprint.RowPitch =
            static_cast<UINT>((fbDesc.Width + 255) & ~255ULL);

        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // フィードバックマップを UAV に戻す
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        cmdList->ResourceBarrier(1, &barrier);
    }
}

void SamplerFeedback::AnalyzeFeedback(uint32_t frameIndex)
{
    (void)frameIndex;

    m_mipRequests.clear();

    if (m_tier == SamplerFeedbackTier::None)
        return;

    // リゾルブバッファをCPU側で読み取り、ミップリクエストを生成する
    for (uint32_t i = 0; i < m_trackedCount; ++i)
    {
        if (!m_resolveBuffers[i])
            continue;

        // READBACKバッファをマップして読み取る
        void* mappedData = nullptr;
        D3D12_RANGE readRange = {};
        readRange.Begin = 0;

        D3D12_RESOURCE_DESC resolveDesc = m_resolveBuffers[i]->GetDesc();
        readRange.End = static_cast<SIZE_T>(resolveDesc.Width);

        HRESULT hr = m_resolveBuffers[i]->Map(0, &readRange, &mappedData);
        if (FAILED(hr) || !mappedData)
            continue;

        // フィードバックマップの各タイルの最小ミップレベルを調べる
        const uint8_t* feedbackData = static_cast<const uint8_t*>(mappedData);

        // フィードバックマップのサイズを取得
        D3D12_RESOURCE_DESC fbDesc = m_feedbackMaps[i] ?
            m_feedbackMaps[i]->GetDesc() : D3D12_RESOURCE_DESC{};
        uint32_t tileCount = static_cast<uint32_t>(fbDesc.Width) * fbDesc.Height;

        // 全タイルの中で最も高解像度（最小ミップ番号）のリクエストを見つける
        uint32_t minMip = 0xFF;
        for (uint32_t t = 0; t < tileCount && t < readRange.End; ++t)
        {
            uint8_t mipLevel = feedbackData[t];
            if (mipLevel < minMip && mipLevel != 0xFF)
            {
                minMip = mipLevel;
            }
        }

        // アンマップ
        D3D12_RANGE writeRange = { 0, 0 }; // CPU側書き込みなし
        m_resolveBuffers[i]->Unmap(0, &writeRange);

        // リクエストを追加
        if (minMip != 0xFF)
        {
            MipRequest req = {};
            req.textureIndex     = i;
            req.requestedMipLevel = minMip;
            req.needsStreaming    = (minMip < 4); // ミップ4未満は高解像度を要求
            m_mipRequests.push_back(req);
        }
    }
}

} // namespace gx
