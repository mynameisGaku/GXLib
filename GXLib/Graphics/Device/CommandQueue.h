#pragma once
/// @file CommandQueue.h
/// @brief コマンドキュー管理クラス
///
/// 【初学者向け解説】
/// コマンドキューは、GPUに対する「命令の受付窓口」です。
/// CPUが作成した描画コマンド（コマンドリスト）をこのキューに投入すると、
/// GPUが順番に処理していきます。
///
/// DX12には3種類のキューがあります：
/// - Direct: 描画コマンド用（最も一般的）
/// - Compute: 計算シェーダー用
/// - Copy: リソースコピー用
///
/// Phase 0ではDirectキューのみ使用します。

#include "pch.h"
#include "Graphics/Device/Fence.h"

namespace GX
{

/// @brief コマンドキューを管理するクラス
class CommandQueue
{
public:
    CommandQueue() = default;
    ~CommandQueue() = default;

    /// @brief コマンドキューを作成する
    /// @param device D3D12デバイスへのポインタ
    /// @param type コマンドリストの種類（Direct/Compute/Copy）
    /// @return 作成に成功した場合true
    bool Initialize(ID3D12Device* device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// @brief 複数のコマンドリストを実行する
    /// @param lists コマンドリストの配列
    /// @param count コマンドリストの数
    void ExecuteCommandLists(ID3D12CommandList* const* lists, uint32_t count);

    /// @brief 単一のコマンドリストを実行する（便利関数）
    /// @param list 実行するコマンドリスト
    void ExecuteCommandList(ID3D12CommandList* list);

    /// @brief GPUの処理完了をCPU側で待機する
    void Flush();

    /// @brief D3D12コマンドキューを取得する
    /// @return ID3D12CommandQueueへのポインタ
    ID3D12CommandQueue* GetQueue() const { return m_queue.Get(); }

    /// @brief フェンスオブジェクトを取得する
    /// @return Fenceへの参照
    Fence& GetFence() { return m_fence; }

private:
    ComPtr<ID3D12CommandQueue> m_queue;
    Fence m_fence;
};

} // namespace GX
