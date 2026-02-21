/// @file GPUProfiler.h
/// @brief GPU処理時間の計測プロファイラ
///
/// D3D12のQuery Heap(TIMESTAMP)を使ってGPU側の処理時間を計測する。
/// ダブルバッファリングのリードバックで、GPUストールなしに前フレームの結果を取得する。
/// DxLibには相当する機能がない。パフォーマンス分析用。
#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace GX
{

/// @brief GPU処理時間を計測するシングルトン
///
/// BeginFrame/EndFrameでフレーム全体、BeginScope/EndScopeで個別区間を計測する。
/// 結果は1フレーム遅れで取得される（ダブルバッファリードバック）。
class GPUProfiler
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return GPUProfilerの参照
    static GPUProfiler& Instance();

    /// @brief プロファイラを初期化する
    /// @param device D3D12デバイスへのポインタ
    /// @param queue タイムスタンプ周波数取得用のコマンドキュー
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue);

    /// @brief リソースを解放する
    void Shutdown();

    /// @brief フレーム開始時に呼ぶ（前フレーム結果のリードバック + 開始タイムスタンプ発行）
    /// @param cmdList コマンドリスト
    /// @param frameIndex 現在のフレーム番号（0 or 1）
    void BeginFrame(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief フレーム終了時に呼ぶ（終了タイムスタンプ発行 + リードバックバッファにコピー）
    /// @param cmdList コマンドリスト
    void EndFrame(ID3D12GraphicsCommandList* cmdList);

    /// @brief 計測区間の開始
    /// @param cmdList コマンドリスト
    /// @param name 区間名（文字列リテラル推奨。ポインタを保持するのでスコープ寿命に注意）
    void BeginScope(ID3D12GraphicsCommandList* cmdList, const char* name);

    /// @brief 計測区間の終了（直近のBeginScopeと対応）
    /// @param cmdList コマンドリスト
    void EndScope(ID3D12GraphicsCommandList* cmdList);

    /// @brief 個別区間の計測結果
    struct ScopeResult
    {
        const char* name;       ///< 区間名
        float durationMs;       ///< 処理時間（ミリ秒）
    };

    /// @brief 前フレームの全区間の計測結果を取得する
    /// @return ScopeResultの配列
    const std::vector<ScopeResult>& GetResults() const { return m_results; }

    /// @brief 前フレームのGPU処理時間合計を取得する
    /// @return ミリ秒
    float GetFrameGPUTimeMs() const { return m_frameGPUTimeMs; }

    /// @brief プロファイラが有効かどうかを取得する
    /// @return 有効なら true
    bool IsEnabled() const { return m_enabled; }

    /// @brief プロファイラの有効/無効を設定する
    /// @param e trueで有効化
    void SetEnabled(bool e) { m_enabled = e; }

    /// @brief 有効/無効をトグルする
    void ToggleEnabled() { m_enabled = !m_enabled; }

private:
    GPUProfiler() = default;
    ~GPUProfiler() = default;
    GPUProfiler(const GPUProfiler&) = delete;
    GPUProfiler& operator=(const GPUProfiler&) = delete;

    static constexpr uint32_t k_BufferCount = 2;        ///< リードバックのダブルバッファ
    static constexpr uint32_t k_MaxTimestamps = 256;     ///< 最大タイムスタンプ数（128スコープ分）

    ID3D12Device* m_device = nullptr;
    uint64_t m_timestampFrequency = 0;   ///< GPUタイムスタンプの周波数（ticks/sec）

    Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_queryHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_readbackBuffer[k_BufferCount];

    /// 1つの計測区間の開始/終了タイムスタンプインデックス
    struct ScopeEntry
    {
        const char* name;
        uint32_t beginIndex;
        uint32_t endIndex;
    };

    /// 1フレーム分の計測データ
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

    /// リードバックバッファから前フレームの結果を読み取る
    void ReadbackResults(uint32_t frameIndex);

    /// タイムスタンプスロットを1つ確保して返す
    uint32_t AllocTimestamp();
};

/// @brief RAII方式の計測スコープ（コンストラクタでBegin、デストラクタでEnd）
///
/// 使い方: { GPUProfileScope scope(cmdList, "Shadow Pass"); ... }
struct GPUProfileScope
{
    /// @brief 計測開始
    /// @param cmdList コマンドリスト
    /// @param name 区間名
    GPUProfileScope(ID3D12GraphicsCommandList* cmdList, const char* name)
        : m_cmdList(cmdList)
    {
        GPUProfiler::Instance().BeginScope(cmdList, name);
    }

    /// @brief 計測終了（スコープ抜け時に自動呼び出し）
    ~GPUProfileScope()
    {
        GPUProfiler::Instance().EndScope(m_cmdList);
    }

    GPUProfileScope(const GPUProfileScope&) = delete;
    GPUProfileScope& operator=(const GPUProfileScope&) = delete;

private:
    ID3D12GraphicsCommandList* m_cmdList;
};

/// @brief 計測スコープを手軽に張るマクロ
///
/// 使い方: GX_GPU_PROFILE_SCOPE(cmdList, "MyPass");
#define GX_GPU_PROFILE_SCOPE(cmdList, name) \
    GX::GPUProfileScope _gpuScope_##__LINE__(cmdList, name)

} // namespace GX
