#pragma once
/// @file ParallelRenderQueue.h
/// @brief マルチスレッド描画準備（並列コマンドリスト記録）
///
/// ワーカースレッドプールで独立したコマンドリストに描画コマンドを並列記録し、
/// 最後にメインスレッドでExecuteCommandListsする。
/// @addtogroup grp_graphics/// @{

#include "pch_graphics.h"
#include <queue>

namespace gx
{

class GraphicsDevice;

/// @brief 並列描画キュー
///
/// 複数のワーカースレッドがそれぞれ独立したコマンドリストに
/// 描画コマンドを記録し、Execute()でまとめて実行する。
class ParallelRenderQueue
{
public:
    ParallelRenderQueue() = default;
    ~ParallelRenderQueue();

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param workerCount ワーカー数（0=ハードウェア並行数-1）
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t workerCount = 0);

    /// @brief 描画コマンドをサブミットする（ワーカースレッドで記録される）
    /// @param recordFunc コマンドリストへの記録関数
    void Submit(std::function<void(ID3D12GraphicsCommandList*)> recordFunc);

    /// @brief 全サブミットの完了を待ち、コマンドキューで実行する
    /// @param commandQueue 実行先のコマンドキュー
    void Execute(ID3D12CommandQueue* commandQueue);

    /// @brief シャットダウン
    void Shutdown();

    /// @brief ワーカー数を取得する
    uint32_t GetWorkerCount() const { return static_cast<uint32_t>(m_workers.size()); }

private:
    struct WorkerData
    {
        ComPtr<ID3D12CommandAllocator>    allocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        bool used = false;
    };

    void WorkerThread();

    ID3D12Device* m_device = nullptr;

    std::vector<std::thread> m_workers;
    std::vector<WorkerData>  m_workerData;

    std::queue<std::function<void(ID3D12GraphicsCommandList*)>> m_tasks;
    std::mutex              m_taskMutex;
    std::condition_variable m_taskCV;
    std::atomic<bool>       m_shutdown{ false };
    std::atomic<int>        m_pendingTasks{ 0 };
    std::mutex              m_completionMutex;
    std::condition_variable m_completionCV;
};

} // namespace gx
/// @}
