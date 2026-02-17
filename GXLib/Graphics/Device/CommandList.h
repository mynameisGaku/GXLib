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
    /// @brief ダブルバッファリング用のアロケータ数
    static constexpr uint32_t k_AllocatorCount = 2;

    CommandList() = default;
    ~CommandList() = default;

    /// @brief コマンドリストとアロケータを作成する
    /// @param device D3D12デバイスへのポインタ
    /// @param type コマンドリストの種類（Direct/Compute/Copy）
    /// @return 作成に成功した場合true
    bool Initialize(ID3D12Device* device,
                    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// @brief 指定フレームインデックスのアロケータでリセットしコマンド記録を開始する
    /// @param frameIndex 現在のフレームインデックス（0または1）
    /// @param initialPSO 初期パイプラインステート（省略可）
    /// @return リセットに成功した場合true
    bool Reset(uint32_t frameIndex, ID3D12PipelineState* initialPSO = nullptr);

    /// @brief コマンド記録を終了する
    /// @return クローズに成功した場合true
    bool Close();

    /// @brief D3D12グラフィックスコマンドリストを取得する
    /// @return ID3D12GraphicsCommandListへのポインタ
    ID3D12GraphicsCommandList* Get() const { return m_commandList.Get(); }

    /// @brief D3D12グラフィックスコマンドリストへのアクセス演算子
    /// @return ID3D12GraphicsCommandListへのポインタ
    ID3D12GraphicsCommandList* operator->() const { return m_commandList.Get(); }

    /// @brief DXR用コマンドリスト (ID3D12GraphicsCommandList4) を取得する
    /// @return ID3D12GraphicsCommandList4へのポインタ（DXR非対応時はnullptr）
    ID3D12GraphicsCommandList4* Get4() const { return m_commandList4.Get(); }

private:
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList4;
    std::array<ComPtr<ID3D12CommandAllocator>, k_AllocatorCount> m_allocators;
};

} // namespace GX
