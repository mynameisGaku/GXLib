#include "pch_graphics.h"
/// @file TextureStreaming.cpp
/// @brief テクスチャストリーミングマネージャーの実装
///
/// 距離ベースの mip レベル選択と VRAM 予算管理を行う。
/// カメラ位置とテクスチャの使用状況に基づいて最適な mip レベルを決定し、
/// 予算超過時は LRU ポリシーで低解像度 mip へエビクトする。
///
/// device==nullptr の場合は CPU-only モード（テスト用途）で動作する。

#include "Graphics/Resource/TextureStreaming.h"
#include "Core/Logger.h"
#include <algorithm>
#include <cmath>

namespace gx
{

// ============================================================================
// コンストラクタ / デストラクタ
// ============================================================================

TextureStreamingManager::TextureStreamingManager() = default;

TextureStreamingManager::~TextureStreamingManager()
{
    Shutdown();
}

// ============================================================================
// 初期化
// ============================================================================

bool TextureStreamingManager::Initialize(ID3D12Device* device, uint64_t budgetBytes)
{
    if (m_initialized)
        return false;

    m_device = device;
    m_budgetBytes = budgetBytes;
    m_usedBytes = 0;
    m_currentFrame = 0;
    m_pendingRequests = 0;
    m_evictedCount = 0;
    m_textures.clear();

    // device が nullptr の場合は CPU-only モード（テスト用途）として成功を返す。
    // 将来的にはここで DirectStorage や非同期コピーキュー等を初期化する。

    if (device)
    {
        // ステージングバッファを作成 (UPLOAD ヒープ, 16MB)
        // mip データをCPUから書き込み、CopyTextureRegion で DEFAULT ヒープに転送する
        static constexpr uint64_t k_StagingBufferSize = 16ULL * 1024 * 1024; // 16MB

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width              = k_StagingBufferSize;
        bufDesc.Height             = 1;
        bufDesc.DepthOrArraySize   = 1;
        bufDesc.MipLevels          = 1;
        bufDesc.Format             = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count   = 1;
        bufDesc.SampleDesc.Quality = 0;
        bufDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_stagingBuffer));

        if (SUCCEEDED(hr))
        {
            m_stagingBuffer->SetName(L"TextureStreaming_StagingBuffer");
            GX_LOG_INFO("TextureStreaming: staging buffer created (16 MB)");
        }
        else
        {
            GX_LOG_WARN("TextureStreaming: Failed to create staging buffer (0x%08X) -- GPU copy unavailable", hr);
        }
    }

    m_initialized = true;
    return true;
}

// ============================================================================
// シャットダウン
// ============================================================================

void TextureStreamingManager::Shutdown()
{
    m_stagingBuffer.Reset();
    m_feedbackMap.Reset();
    m_feedbackReadback.Reset();
    m_feedbackData.clear();
    m_feedbackInitialized = false;
    m_feedbackReadbackReady = false;
    m_feedbackConfig = {};
    m_textures.clear();
    m_device = nullptr;
    m_initialized = false;
    m_usedBytes = 0;
    m_currentFrame = 0;
    m_pendingRequests = 0;
    m_evictedCount = 0;
}

// ============================================================================
// テクスチャ登録
// ============================================================================

void TextureStreamingManager::RegisterTexture(int handle, const gx::WString& path,
                                               uint32_t width, uint32_t height,
                                               uint32_t mipCount)
{
    if (!m_initialized)
        return;

    // 重複チェック
    for (const auto& tex : m_textures)
    {
        if (tex.handle == handle)
            return;
    }

    TextureStreamingEntry entry;
    entry.handle = handle;
    entry.filePath = path;
    entry.width = width;
    entry.height = height;
    entry.mipCount = (mipCount > 0) ? mipCount : 1;
    // 初期状態は最低解像度 mip（mipCount - 1）
    entry.currentMipLevel = entry.mipCount - 1;
    entry.requestedMipLevel = entry.currentMipLevel;
    entry.priority = MipPriority::Normal;
    entry.lastAccessFrame = m_currentFrame;

    // 現在の mip レベルの推定サイズを加算
    entry.estimatedBytes = EstimateMipSize(width, height, entry.currentMipLevel);
    m_usedBytes += entry.estimatedBytes;

    m_textures.push_back(entry);
}

// ============================================================================
// テクスチャ登録解除
// ============================================================================

void TextureStreamingManager::UnregisterTexture(int handle)
{
    auto it = std::find_if(m_textures.begin(), m_textures.end(),
        [handle](const TextureStreamingEntry& e) { return e.handle == handle; });

    if (it != m_textures.end())
    {
        // 使用メモリから差し引く
        if (m_usedBytes >= it->estimatedBytes)
            m_usedBytes -= it->estimatedBytes;
        else
            m_usedBytes = 0;

        m_textures.erase(it);
    }
}

// ============================================================================
// mip レベルリクエスト
// ============================================================================

void TextureStreamingManager::RequestMipLevel(int handle, uint32_t mipLevel,
                                               MipPriority priority)
{
    auto it = std::find_if(m_textures.begin(), m_textures.end(),
        [handle](const TextureStreamingEntry& e) { return e.handle == handle; });

    if (it != m_textures.end())
    {
        // mip レベルを有効範囲にクランプ
        if (mipLevel >= it->mipCount)
            mipLevel = it->mipCount - 1;

        it->requestedMipLevel = mipLevel;
        it->priority = priority;
        it->lastAccessFrame = m_currentFrame;

        // 現在のレベルと異なる場合はペンディングリクエストとしてカウント
        if (it->requestedMipLevel != it->currentMipLevel)
            m_pendingRequests++;
    }
}

// ============================================================================
// カメラ情報に基づく自動 mip 選択
// ============================================================================

void TextureStreamingManager::UpdateFromCamera(const Vector3& cameraPos, float fovDegrees)
{
    if (!m_initialized)
        return;

    // FOV から基準距離を算出（FOV が狭いほど遠くのテクスチャも高解像度で必要）
    // baseDist: この距離以下では mip0（最高解像度）を使う
    float fovRad = fovDegrees * 3.14159265f / 180.0f;
    float baseDist = 10.0f / (std::tan(fovRad * 0.5f) + 0.001f);
    if (baseDist < 1.0f)
        baseDist = 1.0f;

    for (auto& tex : m_textures)
    {
        // 距離ベースの mip 選択:
        // テクスチャ位置はまだ追跡していないので、カメラから原点 (0,0,0) までの距離を
        // プロキシとして使用する。完全な実装では各テクスチャにワールド空間 AABB を紐付ける。
        float dx = cameraPos.x;
        float dy = cameraPos.y;
        float dz = cameraPos.z;
        float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

        // mipLevel = clamp( log2(distance / baseDist), 0, mipCount - 1 )
        float mipFloat = 0.0f;
        if (distance > baseDist)
        {
            mipFloat = std::log2(distance / baseDist);
        }

        uint32_t mipLevel = static_cast<uint32_t>(mipFloat);
        if (mipLevel >= tex.mipCount)
            mipLevel = tex.mipCount - 1;

        tex.requestedMipLevel = mipLevel;
        tex.lastAccessFrame = m_currentFrame;
    }
}

// ============================================================================
// フレーム更新（mip ロード処理）
// ============================================================================

void TextureStreamingManager::Update(ID3D12GraphicsCommandList* /*cmdList*/)
{
    if (!m_initialized)
        return;

    // 1フレームあたり最大4リクエストを処理（帯域制限）
    uint32_t processedThisFrame = 0;
    const uint32_t maxRequestsPerFrame = 4;

    // 優先度順にソート（Immediate > High > Normal > Low）して処理
    // まず Immediate 優先度のリクエストを探す
    for (auto& tex : m_textures)
    {
        if (processedThisFrame >= maxRequestsPerFrame)
            break;

        if (tex.requestedMipLevel == tex.currentMipLevel)
            continue;

        // 優先度でフィルタ: 最初のパスでは Immediate のみ
        if (tex.priority != MipPriority::Immediate)
            continue;

        // mip レベルの変更を適用
        uint64_t oldSize = tex.estimatedBytes;
        uint64_t newSize = EstimateMipSize(tex.width, tex.height, tex.requestedMipLevel);

        // 使用量を更新
        if (m_usedBytes >= oldSize)
            m_usedBytes -= oldSize;
        else
            m_usedBytes = 0;

        m_usedBytes += newSize;

        // GPU mip copy: staging -> DEFAULT heap via CopyTextureRegion
        // 実際の実装ではコピーコマンドリストを使用する
        if (m_device != nullptr && m_stagingBuffer)
        {
            GX_LOG_INFO("TextureStreaming: GPU mip copy handle=%d mip %u -> %u",
                        tex.handle, tex.currentMipLevel, tex.requestedMipLevel);
        }

        tex.currentMipLevel = tex.requestedMipLevel;
        tex.estimatedBytes = newSize;
        tex.lastAccessFrame = m_currentFrame;

        processedThisFrame++;
    }

    // 2パス目: High, Normal, Low の順で処理
    MipPriority priorities[] = { MipPriority::High, MipPriority::Normal, MipPriority::Low };
    for (auto prio : priorities)
    {
        if (processedThisFrame >= maxRequestsPerFrame)
            break;

        for (auto& tex : m_textures)
        {
            if (processedThisFrame >= maxRequestsPerFrame)
                break;

            if (tex.requestedMipLevel == tex.currentMipLevel)
                continue;

            if (tex.priority != prio)
                continue;

            uint64_t oldSize = tex.estimatedBytes;
            uint64_t newSize = EstimateMipSize(tex.width, tex.height, tex.requestedMipLevel);

            if (m_usedBytes >= oldSize)
                m_usedBytes -= oldSize;
            else
                m_usedBytes = 0;

            m_usedBytes += newSize;

            // GPU mip copy: staging -> DEFAULT heap via CopyTextureRegion
            // 実際の実装ではコピーコマンドリストを使用する
            if (m_device != nullptr && m_stagingBuffer)
            {
                GX_LOG_INFO("TextureStreaming: GPU mip copy handle=%d mip %u -> %u",
                            tex.handle, tex.currentMipLevel, tex.requestedMipLevel);
            }

            tex.currentMipLevel = tex.requestedMipLevel;
            tex.estimatedBytes = newSize;
            tex.lastAccessFrame = m_currentFrame;

            processedThisFrame++;
        }
    }

    // ペンディングリクエスト数を再計算
    m_pendingRequests = 0;
    for (const auto& tex : m_textures)
    {
        if (tex.requestedMipLevel != tex.currentMipLevel)
            m_pendingRequests++;
    }

    // 予算超過の場合は LRU エビクション
    while (m_usedBytes > m_budgetBytes && !m_textures.empty())
    {
        EvictLRU();
    }

    m_currentFrame++;
}

// ============================================================================
// mip サイズ推定
// ============================================================================

uint64_t TextureStreamingManager::EstimateMipSize(uint32_t width, uint32_t height,
                                                    uint32_t mipLevel) const
{
    // 各 mip レベルで幅・高さを半分にしていく
    uint32_t w = width >> mipLevel;
    uint32_t h = height >> mipLevel;

    // 最低 1x1 を保証
    if (w == 0) w = 1;
    if (h == 0) h = 1;

    // RGBA8 (4 bytes per pixel) を仮定。最低 4 バイト。
    uint64_t size = static_cast<uint64_t>(w) * static_cast<uint64_t>(h) * 4;
    if (size < 4)
        size = 4;

    return size;
}

// ============================================================================
// LRU エビクション
// ============================================================================

void TextureStreamingManager::EvictLRU()
{
    if (m_textures.empty())
        return;

    // 最も古い lastAccessFrame を持つテクスチャを探す
    size_t oldestIdx = 0;
    uint64_t oldestFrame = m_textures[0].lastAccessFrame;

    for (size_t i = 1; i < m_textures.size(); ++i)
    {
        if (m_textures[i].lastAccessFrame < oldestFrame)
        {
            oldestFrame = m_textures[i].lastAccessFrame;
            oldestIdx = i;
        }
    }

    auto& tex = m_textures[oldestIdx];

    // 既に最低解像度の場合はこれ以上エビクトできない
    if (tex.currentMipLevel >= tex.mipCount - 1)
    {
        // これ以上下げられないので、予算超過が解消できない。
        // 無限ループを防ぐため、このテクスチャを完全にアンロードする。
        if (m_usedBytes >= tex.estimatedBytes)
            m_usedBytes -= tex.estimatedBytes;
        else
            m_usedBytes = 0;

        tex.estimatedBytes = 0;
        tex.currentMipLevel = tex.mipCount - 1;
        m_evictedCount++;
        return;
    }

    // mip レベルを 1 段階下げる（低解像度に）
    uint64_t oldSize = tex.estimatedBytes;
    tex.currentMipLevel++;
    tex.requestedMipLevel = tex.currentMipLevel;  // リクエストも合わせる
    uint64_t newSize = EstimateMipSize(tex.width, tex.height, tex.currentMipLevel);

    if (m_usedBytes >= oldSize)
        m_usedBytes -= oldSize;
    else
        m_usedBytes = 0;

    m_usedBytes += newSize;
    tex.estimatedBytes = newSize;
    m_evictedCount++;
}

// ============================================================================
// 統計情報取得
// ============================================================================

StreamingStats TextureStreamingManager::GetStats() const
{
    StreamingStats stats;
    stats.totalBudgetBytes = m_budgetBytes;
    stats.usedBytes = m_usedBytes;
    stats.registeredTextures = static_cast<uint32_t>(m_textures.size());
    stats.pendingRequests = m_pendingRequests;
    stats.evictedMips = m_evictedCount;

    // ロード済み mip 数: 最高解像度（mip0）に近いほど多くの mip がロードされている
    uint32_t loadedMips = 0;
    for (const auto& tex : m_textures)
    {
        // currentMipLevel が 0 なら全 mip がロード済み（mipCount 個）
        // currentMipLevel が mipCount-1 なら最低解像度の 1 mip のみ
        if (tex.mipCount > tex.currentMipLevel)
            loadedMips += (tex.mipCount - tex.currentMipLevel);
    }
    stats.loadedMips = loadedMips;

    return stats;
}

// ============================================================================
// GPU Sampler Feedback — フィードバックマップ初期化
// ============================================================================

bool TextureStreamingManager::InitializeFeedback(ID3D12Device* device,
                                                  uint32_t screenWidth,
                                                  uint32_t screenHeight)
{
    if (m_feedbackInitialized)
        return false;

    // フィードバックマップサイズ: 画面解像度の 1/16
    uint32_t fbWidth  = (screenWidth  + 15) / 16;
    uint32_t fbHeight = (screenHeight + 15) / 16;

    if (fbWidth == 0) fbWidth = 1;
    if (fbHeight == 0) fbHeight = 1;

    m_feedbackConfig.feedbackWidth  = fbWidth;
    m_feedbackConfig.feedbackHeight = fbHeight;

    // device が nullptr の場合は CPU-only モード（テスト用途）
    if (!device)
    {
        m_feedbackConfig.enabled = true;
        m_feedbackInitialized = true;
        m_feedbackData.resize(static_cast<size_t>(fbWidth) * fbHeight, 0xFF);
        GX_LOG_INFO("TextureStreaming: feedback initialized (CPU-only, %ux%u)", fbWidth, fbHeight);
        return true;
    }

    // R8_UINT テクスチャ (UAV) — シェーダが最小アクセスmipレベルを書き込む
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width              = fbWidth;
    texDesc.Height             = fbHeight;
    texDesc.DepthOrArraySize   = 1;
    texDesc.MipLevels          = 1;
    texDesc.Format             = DXGI_FORMAT_R8_UINT;
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&m_feedbackMap));

    if (FAILED(hr))
    {
        GX_LOG_WARN("TextureStreaming: Failed to create feedback map (0x%08X)", hr);
        return false;
    }
    m_feedbackMap->SetName(L"TextureStreaming_FeedbackMap");

    // Readback バッファ — フィードバックマップと同サイズのバッファ
    // R8_UINT: 1 byte per texel, ただし D3D12 テクスチャのコピーには
    // 行ピッチのアライメントが必要（D3D12_TEXTURE_DATA_PITCH_ALIGNMENT = 256）
    uint64_t rowPitch = ((static_cast<uint64_t>(fbWidth) + 255) / 256) * 256;
    uint64_t readbackSize = rowPitch * fbHeight;

    D3D12_HEAP_PROPERTIES readbackHeap = {};
    readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width              = readbackSize;
    bufDesc.Height             = 1;
    bufDesc.DepthOrArraySize   = 1;
    bufDesc.MipLevels          = 1;
    bufDesc.Format             = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count   = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = device->CreateCommittedResource(
        &readbackHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&m_feedbackReadback));

    if (FAILED(hr))
    {
        GX_LOG_WARN("TextureStreaming: Failed to create readback buffer (0x%08X)", hr);
        m_feedbackMap.Reset();
        return false;
    }
    m_feedbackReadback->SetName(L"TextureStreaming_FeedbackReadback");

    m_feedbackConfig.enabled = true;
    m_feedbackInitialized = true;
    m_feedbackData.resize(static_cast<size_t>(fbWidth) * fbHeight, 0xFF);

    GX_LOG_INFO("TextureStreaming: feedback initialized (%ux%u, readback %llu bytes)",
                fbWidth, fbHeight, readbackSize);
    return true;
}

// ============================================================================
// GPU Sampler Feedback — UAV → Readback コピー & マップ
// ============================================================================

void TextureStreamingManager::ReadbackFeedback(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_feedbackInitialized || !m_feedbackConfig.enabled)
        return;

    // CPU-only モード（device == nullptr）: readbackは不要、テストデータをそのまま使う
    if (!m_feedbackMap || !m_feedbackReadback || !cmdList)
    {
        m_feedbackReadbackReady = true;
        return;
    }

    uint32_t fbWidth  = m_feedbackConfig.feedbackWidth;
    uint32_t fbHeight = m_feedbackConfig.feedbackHeight;

    // UAV → COPY_SOURCE バリア
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_feedbackMap.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // テクスチャ → バッファ コピー
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource        = m_feedbackMap.Get();
    src.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0;

    uint64_t rowPitch = ((static_cast<uint64_t>(fbWidth) + 255) / 256) * 256;

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource                         = m_feedbackReadback.Get();
    dst.Type                              = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Offset            = 0;
    dst.PlacedFootprint.Footprint.Format  = DXGI_FORMAT_R8_UINT;
    dst.PlacedFootprint.Footprint.Width   = fbWidth;
    dst.PlacedFootprint.Footprint.Height  = fbHeight;
    dst.PlacedFootprint.Footprint.Depth   = 1;
    dst.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(rowPitch);

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // COPY_SOURCE → UAV バリア（次フレームで再利用）
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    cmdList->ResourceBarrier(1, &barrier);

    // Map して CPU 側データにコピー
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(rowPitch * fbHeight) };
    HRESULT hr = m_feedbackReadback->Map(0, &readRange, &mappedData);
    if (SUCCEEDED(hr) && mappedData)
    {
        const uint8_t* src8 = static_cast<const uint8_t*>(mappedData);
        for (uint32_t y = 0; y < fbHeight; ++y)
        {
            std::memcpy(&m_feedbackData[static_cast<size_t>(y) * fbWidth],
                        src8 + y * rowPitch,
                        fbWidth);
        }

        D3D12_RANGE writeRange = { 0, 0 };
        m_feedbackReadback->Unmap(0, &writeRange);
        m_feedbackReadbackReady = true;
    }
    else
    {
        GX_LOG_WARN("TextureStreaming: Failed to map readback buffer (0x%08X)", hr);
        m_feedbackReadbackReady = false;
    }
}

// ============================================================================
// GPU Sampler Feedback — readback結果を解析してmipリクエスト生成
// ============================================================================

void TextureStreamingManager::ProcessFeedback()
{
    if (!m_feedbackInitialized || !m_feedbackConfig.enabled)
        return;

    if (!m_feedbackReadbackReady)
        return;

    uint32_t fbWidth  = m_feedbackConfig.feedbackWidth;
    uint32_t fbHeight = m_feedbackConfig.feedbackHeight;

    if (m_feedbackData.empty() || m_textures.empty())
        return;

    // フィードバックマップの各テクセルには、そのスクリーン領域で
    // シェーダがアクセスした最小ミップレベル（最高解像度）が格納されている。
    // 0xFF は「アクセスなし」を示す。
    //
    // テクスチャとフィードバックテクセルの対応付け:
    // フィードバックマップを登録テクスチャ数で均等に分割し、
    // 各テクスチャの担当領域内の最小値をそのテクスチャの要求mipとする。
    // (完全な実装ではテクスチャIDをエンコードしたフィードバックマップを使う)
    uint32_t texCount = static_cast<uint32_t>(m_textures.size());
    uint32_t totalTexels = fbWidth * fbHeight;

    // 各テクスチャの最小mipレベルを算出
    // 均等分割: テクスチャ i はテクセル [i * texelsPerTex, (i+1) * texelsPerTex) を担当
    uint32_t texelsPerTex = (totalTexels + texCount - 1) / texCount;

    for (uint32_t ti = 0; ti < texCount; ++ti)
    {
        auto& tex = m_textures[ti];
        uint8_t minMip = 0xFF;

        uint32_t start = ti * texelsPerTex;
        uint32_t end   = (std::min)(start + texelsPerTex, totalTexels);

        for (uint32_t idx = start; idx < end; ++idx)
        {
            uint8_t val = m_feedbackData[idx];
            if (val < minMip)
                minMip = val;
        }

        if (minMip == 0xFF)
        {
            // アクセスなし — 最低解像度で十分
            continue;
        }

        // フィードバックから得たmipレベルをテクスチャの有効範囲にクランプ
        uint32_t requestedMip = static_cast<uint32_t>(minMip);
        if (requestedMip >= tex.mipCount)
            requestedMip = tex.mipCount - 1;

        // 現在より高解像度が必要な場合のみリクエスト生成
        if (requestedMip < tex.currentMipLevel)
        {
            tex.requestedMipLevel = requestedMip;
            tex.priority = MipPriority::High;
            tex.lastAccessFrame = m_currentFrame;
        }
    }

    m_feedbackReadbackReady = false;
}

} // namespace gx
