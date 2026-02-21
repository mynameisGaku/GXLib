#pragma once
/// @file CommandQueue.h
/// @brief GPUへの命令送信キュー
///
/// DxLibではDraw系関数を呼ぶだけでGPUに命令が飛ぶが、DX12では
/// コマンドリストに記録した命令をコマンドキュー経由で明示的に送信する。
/// 内部にFenceを持ち、GPU完了待ちもこのクラスから行える。

#include "pch.h"
#include "Graphics/Device/Fence.h"

namespace GX
{

/// @brief GPUに描画コマンドを送信するキュー
///
/// DxLibのDraw系関数は内部的にこれと同等の処理を行っている。
/// DX12では Direct / Compute / Copy の3種類があり、通常はDirectを使う。
class CommandQueue
{
public:
    CommandQueue() = default;
    ~CommandQueue() = default;

    /// @brief コマンドキューを作成する
    /// @param device D3D12デバイスへのポインタ
    /// @param type キューの種類（デフォルトはDirect = 描画命令用）
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// @brief 複数のコマンドリストをまとめてGPUに送信する
    /// @param lists コマンドリストの配列
    /// @param count 配列の要素数
    void ExecuteCommandLists(ID3D12CommandList* const* lists, uint32_t count);

    /// @brief コマンドリストを1つだけ送信する
    /// @param list 実行するコマンドリスト
    void ExecuteCommandList(ID3D12CommandList* list);

    /// @brief GPUの処理が全て完了するまでCPUを待機させる
    ///
    /// DxLibのScreenFlip()内部でも同様の同期が行われている。
    void Flush();

    /// @brief 内部のID3D12CommandQueueを取得する
    /// @return ID3D12CommandQueueへのポインタ
    ID3D12CommandQueue* GetQueue() const { return m_queue.Get(); }

    /// @brief 内蔵のFenceオブジェクトを取得する
    /// @return Fenceへの参照
    Fence& GetFence() { return m_fence; }

private:
    ComPtr<ID3D12CommandQueue> m_queue;
    Fence m_fence;  ///< GPU-CPU同期用フェンス（キュー作成時に自動初期化）
};

} // namespace GX
