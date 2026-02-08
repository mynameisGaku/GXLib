#pragma once
/// @file CommandList.h
/// @brief コマンドリスト管理クラス
///
/// 【初学者向け解説】
/// コマンドリストは、GPUに実行させたい命令を「録画」するためのオブジェクトです。
///
/// DX12の描画の流れ：
/// 1. コマンドアロケータをリセット（命令バッファをクリア）
/// 2. コマンドリストをリセット（録画開始）
/// 3. 描画コマンドを記録（三角形を描け、テクスチャを使え、など）
/// 4. コマンドリストをClose（録画終了）
/// 5. コマンドキューに送信して実行
///
/// コマンドアロケータは「命令を溜めるメモリ」で、GPUが使用中は
/// リセットできません。そのため、ダブルバッファリングに合わせて
/// 2つのアロケータを交互に使います。

#include "pch.h"

namespace GX
{

/// @brief コマンドリストとアロケータを管理するクラス
class CommandList
{
public:
    static constexpr uint32_t k_AllocatorCount = 2;

    CommandList() = default;
    ~CommandList() = default;

    /// コマンドリストとアロケータを作成
    bool Initialize(ID3D12Device* device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// 指定フレームインデックスのアロケータでリセット（コマンド記録を開始）
    bool Reset(uint32_t frameIndex, ID3D12PipelineState* initialPSO = nullptr);

    /// コマンド記録を終了
    bool Close();

    /// D3D12グラフィックスコマンドリストを取得
    ID3D12GraphicsCommandList* Get() const { return m_commandList.Get(); }

    ID3D12GraphicsCommandList* operator->() const { return m_commandList.Get(); }

private:
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    std::array<ComPtr<ID3D12CommandAllocator>, k_AllocatorCount> m_allocators;
};

} // namespace GX
