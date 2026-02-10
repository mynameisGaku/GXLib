/// @file GPUProfiler.h
/// @brief GPU タイムスタンプ プロファイラ
///
/// D3D12 Query Heap を使用したGPU時間計測。
/// ダブルバッファリングリードバックでGPUストールを回避。
#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace GX
{

/// GPU プロファイラ (シングルトン)
class GPUProfiler
{
public:
    static GPUProfiler& Instance();

    /// 初期化 (デバイス + コマンドキューからタイムスタンプ周波数取得)
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue);
    void Shutdown();

    /// フレーム開始 (前フレームの結果をリードバック)
    void BeginFrame(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// フレーム終了 (クエリ結果をリードバックバッファにコピー)
    void EndFrame(ID3D12GraphicsCommandList* cmdList);

    /// スコープ計測開始
    void BeginScope(ID3D12GraphicsCommandList* cmdList, const char* name);

    /// スコープ計測終了
    void EndScope(ID3D12GraphicsCommandList* cmdList);

    /// 計測結果
    struct ScopeResult
    {
        const char* name;
        float durationMs;
    };

    const std::vector<ScopeResult>& GetResults() const { return m_results; }
    float GetFrameGPUTimeMs() const { return m_frameGPUTimeMs; }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool e) { m_enabled = e; }
    void ToggleEnabled() { m_enabled = !m_enabled; }

private:
    GPUProfiler() = default;
    ~GPUProfiler() = default;
    GPUProfiler(const GPUProfiler&) = delete;
    GPUProfiler& operator=(const GPUProfiler&) = delete;

    static constexpr uint32_t k_BufferCount = 2;
    static constexpr uint32_t k_MaxTimestamps = 256; // 128 scopes max

    ID3D12Device* m_device = nullptr;
    uint64_t m_timestampFrequency = 0;

    Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_queryHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_readbackBuffer[k_BufferCount];

    struct ScopeEntry
    {
        const char* name;
        uint32_t beginIndex;
        uint32_t endIndex;
    };

    struct FrameData
    {
        uint32_t timestampCount = 0;
        std::vector<ScopeEntry> scopes;
    };

    FrameData m_frameData[k_BufferCount];
    std::vector<ScopeResult> m_results;
    float m_frameGPUTimeMs = 0.0f;

    uint32_t m_currentFrameIndex = 0;
    uint32_t m_frameCount = 0;
    bool m_enabled = false;

    void ReadbackResults(uint32_t frameIndex);
    uint32_t AllocTimestamp();
};

/// RAII スコープヘルパー
struct GPUProfileScope
{
    GPUProfileScope(ID3D12GraphicsCommandList* cmdList, const char* name)
        : m_cmdList(cmdList)
    {
        GPUProfiler::Instance().BeginScope(cmdList, name);
    }

    ~GPUProfileScope()
    {
        GPUProfiler::Instance().EndScope(m_cmdList);
    }

    GPUProfileScope(const GPUProfileScope&) = delete;
    GPUProfileScope& operator=(const GPUProfileScope&) = delete;

private:
    ID3D12GraphicsCommandList* m_cmdList;
};

/// プロファイルスコープマクロ
#define GX_GPU_PROFILE_SCOPE(cmdList, name) \
    GX::GPUProfileScope _gpuScope_##__LINE__(cmdList, name)

} // namespace GX
