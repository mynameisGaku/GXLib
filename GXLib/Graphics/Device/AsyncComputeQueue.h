#pragma once
/// @file AsyncComputeQueue.h
/// @brief 非同期コンピュートキュー
///
/// D3D12 の COMPUTE キューを使用し、グラフィックス処理と並行して
/// Compute Shader を実行する。SSAO/Bloom 等のポストエフェクトや
/// パーティクル更新を非同期実行することで GPU 使用率を向上できる。
///
/// Fence ベースの同期で Graphics ⇔ Compute 間のリソース依存を管理する。

#include "pch.h"
#include "Graphics/Device/Fence.h"

namespace GX
{

/// @brief 非同期コンピュートキュー
///
/// Graphics キューとは独立した Compute キューでディスパッチを実行し、
/// Fence による同期で安全にリソースを共有する。
class AsyncComputeQueue
{
public:
    AsyncComputeQueue() = default;
    ~AsyncComputeQueue() = default;

    /// @brief コンピュートキューを初期化する
    /// @param device D3D12デバイス
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device);

    /// @brief コマンドリストの記録を開始する
    /// @param frameIndex 現在のフレーム番号（0 or 1）
    /// @param initialPSO 初期PSO（省略可）
    /// @return 成功なら true
    bool BeginRecord(uint32_t frameIndex, ID3D12PipelineState* initialPSO = nullptr);

    /// @brief 記録中のコマンドリストを取得する
    /// @return ID3D12GraphicsCommandListへのポインタ
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    /// @brief 記録を終了してキューに送信する
    void EndRecordAndExecute();

    /// @brief Graphics キューの完了を待ってから Compute を開始するシグナルを挿入する
    /// @param graphicsQueue Graphics 側のコマンドキュー
    void WaitForGraphics(ID3D12CommandQueue* graphicsQueue);

    /// @brief Compute の完了を Graphics キュー側で待つシグナルを挿入する
    /// @param graphicsQueue Graphics 側のコマンドキュー
    void SignalToGraphics(ID3D12CommandQueue* graphicsQueue);

    /// @brief Compute キューの全処理完了を CPU で待つ
    void Flush();

    /// @brief 内部のコンピュートキューを取得する
    /// @return ID3D12CommandQueueへのポインタ
    ID3D12CommandQueue* GetQueue() const { return m_queue.Get(); }

private:
    static constexpr uint32_t k_AllocatorCount = 2;

    ComPtr<ID3D12CommandQueue>         m_queue;
    ComPtr<ID3D12GraphicsCommandList>  m_commandList;
    std::array<ComPtr<ID3D12CommandAllocator>, k_AllocatorCount> m_allocators;

    Fence    m_fence;       ///< Compute キュー用フェンス
    Fence    m_syncFence;   ///< Graphics⇔Compute 同期用フェンス
};

} // namespace GX
