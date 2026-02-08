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

    /// コマンドキューを作成
    bool Initialize(ID3D12Device* device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// コマンドリストを実行
    void ExecuteCommandLists(ID3D12CommandList* const* lists, uint32_t count);

    /// 単一のコマンドリストを実行（便利関数）
    void ExecuteCommandList(ID3D12CommandList* list);

    /// GPUの処理完了を待機
    void Flush();

    /// D3D12コマンドキューを取得
    ID3D12CommandQueue* GetQueue() const { return m_queue.Get(); }

    /// フェンスを取得
    Fence& GetFence() { return m_fence; }

private:
    ComPtr<ID3D12CommandQueue> m_queue;
    Fence m_fence;
};

} // namespace GX
