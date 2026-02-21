#pragma once
/// @file CommandList.h
/// @brief 描画命令の記録バッファ
///
/// DxLibのDrawGraph()やDrawBox()は内部でコマンドリストに命令を積んでいる。
/// DX12ではこの記録と送信を自分で行う。
///
/// 流れ: Reset(記録開始) -> 描画命令を記録 -> Close(記録終了) -> CommandQueueに送信
///
/// コマンドアロケータ(命令メモリ)はGPU使用中にリセットできないため、
/// ダブルバッファリングに合わせて2つを交互に使う。

#include "pch.h"

namespace GX
{

/// @brief GPUへの命令を記録するコマンドリスト
///
/// DxLibのDraw系関数が裏側で使っている仕組みに相当する。
/// DXR対応GPUではID3D12GraphicsCommandList4も内部で保持する。
class CommandList
{
public:
    /// @brief ダブルバッファリング用のアロケータ数（SwapChainのバッファ数と同じ）
    static constexpr uint32_t k_AllocatorCount = 2;

    CommandList() = default;
    ~CommandList() = default;

    /// @brief コマンドリストとアロケータを作成する
    /// @param device D3D12デバイスへのポインタ
    /// @param type コマンドリストの種類（デフォルトはDirect = 描画用）
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// @brief 命令の記録を開始する
    /// @param frameIndex 現在のフレーム番号（0 or 1）。対応するアロケータが使われる
    /// @param initialPSO 最初に設定するPSO（省略可）
    /// @return 成功なら true
    bool Reset(uint32_t frameIndex, ID3D12PipelineState* initialPSO = nullptr);

    /// @brief 命令の記録を終了する（この後CommandQueueに送信できる）
    /// @return 成功なら true
    bool Close();

    /// @brief 内部のコマンドリストを取得する
    /// @return ID3D12GraphicsCommandListへのポインタ
    ID3D12GraphicsCommandList* Get() const { return m_commandList.Get(); }

    /// @brief アロー演算子で直接D3D12 APIを呼べる（cmdList->DrawInstanced() など）
    /// @return ID3D12GraphicsCommandListへのポインタ
    ID3D12GraphicsCommandList* operator->() const { return m_commandList.Get(); }

    /// @brief DXR用のCommandList4インターフェースを取得する
    /// @return ID3D12GraphicsCommandList4へのポインタ（DXR非対応GPUではnullptr）
    ID3D12GraphicsCommandList4* Get4() const { return m_commandList4.Get(); }

private:
    ComPtr<ID3D12GraphicsCommandList>  m_commandList;   ///< 標準コマンドリスト
    ComPtr<ID3D12GraphicsCommandList4> m_commandList4;  ///< DXR用拡張（DispatchRays等）
    std::array<ComPtr<ID3D12CommandAllocator>, k_AllocatorCount> m_allocators; ///< フレーム交互のメモリ
};

} // namespace GX
