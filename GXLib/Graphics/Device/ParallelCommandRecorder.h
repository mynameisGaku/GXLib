#pragma once
/// @file ParallelCommandRecorder.h
/// @brief マルチスレッドコマンド記録
///
/// D3D12 ではコマンドリストの記録はスレッドセーフだが、1つのリストに対して
/// 同時に書き込むことはできない。そのためワーカースレッドごとに独立した
/// CommandList + CommandAllocator を用意し、並列にコマンドを記録して
/// 最後に Direct キューにまとめて送信する。
///
/// 使い方:
///   1. Initialize() でワーカー数を指定して初期化
///   2. AddRecordJob() で記録ジョブを登録
///   3. RecordAndExecute() で全ジョブを並列実行し、結果をキューに送信

#include "pch.h"

namespace GX
{

class CommandQueue;

/// @brief マルチスレッドコマンド記録ヘルパー
class ParallelCommandRecorder
{
public:
    /// @brief 記録ジョブ関数型（引数: コマンドリスト）
    using RecordJob = std::function<void(ID3D12GraphicsCommandList*)>;

    ParallelCommandRecorder() = default;
    ~ParallelCommandRecorder();

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param workerCount ワーカースレッド数（0 = hardware_concurrency - 1）
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device, uint32_t workerCount = 0);

    /// @brief コマンド記録ジョブを追加する
    /// @param job 記録するコールバック
    void AddRecordJob(RecordJob job);

    /// @brief 蓄積した全ジョブを並列実行し、結果をキューに送信する
    /// @param queue 送信先のコマンドキュー
    /// @param frameIndex 現在のフレーム番号（0 or 1）
    /// @param initialPSO 初期PSO（省略可）
    void RecordAndExecute(CommandQueue& queue, uint32_t frameIndex,
                          ID3D12PipelineState* initialPSO = nullptr);

    /// @brief ワーカー数を取得する
    /// @return ワーカースレッド数
    uint32_t GetWorkerCount() const { return m_workerCount; }

    /// @brief 現在登録されているジョブ数を取得する
    /// @return ジョブ数
    uint32_t GetPendingJobCount() const { return static_cast<uint32_t>(m_jobs.size()); }

private:
    struct WorkerCommandList
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        std::array<ComPtr<ID3D12CommandAllocator>, 2> allocators; // ダブルバッファ
    };

    ID3D12Device* m_device = nullptr;
    uint32_t m_workerCount = 0;
    std::vector<WorkerCommandList> m_workerCmdLists;
    std::vector<RecordJob> m_jobs;
};

} // namespace GX
