/// @file GPUProfiler.cpp
/// @brief GPU タイムスタンプ プロファイラの実装
///
/// D3D12 Query Heap (TIMESTAMP) + ダブルバッファリングリードバック。
/// BeginFrame で前フレーム結果を読み取り、EndFrame で ResolveQueryData。
#include "pch.h"
#include "Graphics/Device/GPUProfiler.h"
#include "Core/Logger.h"

namespace GX
{

GPUProfiler& GPUProfiler::Instance()
{
    static GPUProfiler s_instance;
    return s_instance;
}

bool GPUProfiler::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    m_device = device;

    // タイムスタンプ周波数取得 (ticks/sec)
    HRESULT hr = queue->GetTimestampFrequency(&m_timestampFrequency);
    if (FAILED(hr) || m_timestampFrequency == 0)
    {
        GX_LOG_ERROR("GPUProfiler: GetTimestampFrequency failed");
        return false;
    }

    // クエリヒープ作成
    D3D12_QUERY_HEAP_DESC heapDesc = {};
    heapDesc.Type  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    heapDesc.Count = k_MaxTimestamps;
    hr = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&m_queryHeap));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUProfiler: CreateQueryHeap failed");
        return false;
    }

    // リードバックバッファ (k_MaxTimestamps * sizeof(uint64_t) per frame)
    uint64_t bufferSize = k_MaxTimestamps * sizeof(uint64_t);
    // 256バイトアライメント
    bufferSize = (bufferSize + 255) & ~255ULL;

    for (uint32_t i = 0; i < k_BufferCount; ++i)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width            = bufferSize;
        resDesc.Height           = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels        = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_readbackBuffer[i]));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("GPUProfiler: CreateCommittedResource (readback %d) failed", i);
            return false;
        }
    }

    // フレームデータ初期化
    for (uint32_t i = 0; i < k_BufferCount; ++i)
    {
        m_frameData[i].timestampCount = 0;
        m_frameData[i].scopes.clear();
        m_frameData[i].scopes.reserve(64);
    }

    m_results.reserve(64);
    m_frameCount = 0;
    m_frameGPUTimeMs = 0.0f;

    GX_LOG_INFO("GPUProfiler initialized (freq=%llu Hz, maxQueries=%u)",
                m_timestampFrequency, k_MaxTimestamps);
    return true;
}

void GPUProfiler::Shutdown()
{
    m_queryHeap.Reset();
    for (auto& rb : m_readbackBuffer)
        rb.Reset();
    m_results.clear();
    m_device = nullptr;
}

void GPUProfiler::BeginFrame(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    m_currentFrameIndex = frameIndex;

    if (!m_enabled)
    {
        m_results.clear();
        m_frameGPUTimeMs = 0.0f;
        return;
    }

    // 前フレームの結果をリードバック (WaitForValue済みなので安全)
    if (m_frameCount >= k_BufferCount)
    {
        ReadbackResults(frameIndex);
    }

    // 今フレームのデータをリセット
    auto& fd = m_frameData[frameIndex];
    fd.timestampCount = 0;
    fd.scopes.clear();

    // フレーム全体の開始タイムスタンプ
    uint32_t idx = AllocTimestamp();
    cmdList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, idx);
}

void GPUProfiler::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_enabled)
    {
        m_frameCount++;
        return;
    }

    auto& fd = m_frameData[m_currentFrameIndex];

    // フレーム全体の終了タイムスタンプ
    uint32_t idx = AllocTimestamp();
    cmdList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, idx);

    // クエリ結果をリードバックバッファにコピー
    if (fd.timestampCount > 0)
    {
        cmdList->ResolveQueryData(
            m_queryHeap.Get(),
            D3D12_QUERY_TYPE_TIMESTAMP,
            0,
            fd.timestampCount,
            m_readbackBuffer[m_currentFrameIndex].Get(),
            0);
    }

    m_frameCount++;
}

void GPUProfiler::BeginScope(ID3D12GraphicsCommandList* cmdList, const char* name)
{
    if (!m_enabled) return;

    auto& fd = m_frameData[m_currentFrameIndex];
    uint32_t beginIdx = AllocTimestamp();

    cmdList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, beginIdx);

    ScopeEntry entry;
    entry.name       = name;
    entry.beginIndex = beginIdx;
    entry.endIndex   = 0xFFFFFFFF; // 未設定
    fd.scopes.push_back(entry);
}

void GPUProfiler::EndScope(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_enabled) return;

    auto& fd = m_frameData[m_currentFrameIndex];
    if (fd.scopes.empty()) return;

    // 最後の未終了スコープを探す (後方検索)
    for (int i = static_cast<int>(fd.scopes.size()) - 1; i >= 0; --i)
    {
        if (fd.scopes[i].endIndex == 0xFFFFFFFF)
        {
            uint32_t endIdx = AllocTimestamp();
            cmdList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, endIdx);
            fd.scopes[i].endIndex = endIdx;
            break;
        }
    }
}

uint32_t GPUProfiler::AllocTimestamp()
{
    auto& fd = m_frameData[m_currentFrameIndex];
    if (fd.timestampCount >= k_MaxTimestamps)
    {
        // オーバーフロー: 最後のスロットを再利用
        return k_MaxTimestamps - 1;
    }
    return fd.timestampCount++;
}

void GPUProfiler::ReadbackResults(uint32_t frameIndex)
{
    const auto& fd = m_frameData[frameIndex];
    m_results.clear();
    m_frameGPUTimeMs = 0.0f;

    if (fd.timestampCount < 2)
        return;

    // リードバックバッファをマップ
    D3D12_RANGE readRange = { 0, fd.timestampCount * sizeof(uint64_t) };
    void* mapped = nullptr;
    HRESULT hr = m_readbackBuffer[frameIndex]->Map(0, &readRange, &mapped);
    if (FAILED(hr) || !mapped)
        return;

    const uint64_t* timestamps = static_cast<const uint64_t*>(mapped);
    double ticksToMs = 1000.0 / static_cast<double>(m_timestampFrequency);

    // フレーム全体の時間 (最初と最後のタイムスタンプ)
    uint64_t frameBegin = timestamps[0];
    uint64_t frameEnd   = timestamps[fd.timestampCount - 1];
    m_frameGPUTimeMs = static_cast<float>(
        static_cast<double>(frameEnd - frameBegin) * ticksToMs);

    // 各スコープの結果
    for (const auto& scope : fd.scopes)
    {
        if (scope.endIndex == 0xFFFFFFFF) continue;
        if (scope.beginIndex >= fd.timestampCount) continue;
        if (scope.endIndex >= fd.timestampCount) continue;

        uint64_t begin = timestamps[scope.beginIndex];
        uint64_t end   = timestamps[scope.endIndex];
        float duration = static_cast<float>(
            static_cast<double>(end - begin) * ticksToMs);

        m_results.push_back({ scope.name, duration });
    }

    D3D12_RANGE writeRange = { 0, 0 };
    m_readbackBuffer[frameIndex]->Unmap(0, &writeRange);
}

} // namespace GX
