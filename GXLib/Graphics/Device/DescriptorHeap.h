#pragma once
/// @file DescriptorHeap.h
/// @brief テクスチャやバッファのGPU参照テーブル
///
/// DxLibではテクスチャハンドル(int)を渡すだけだが、DX12ではGPUがリソースを
/// 見つけるために「ディスクリプタ」という参照情報が必要。
/// このクラスはディスクリプタの割り当て・解放をフリーリスト方式で管理する。

#include "pch.h"

namespace GX
{

/// @brief ディスクリプタヒープの管理クラス
///
/// RTV(描画先), DSV(深度), CBV_SRV_UAV(テクスチャ/定数/読み書きバッファ)の
/// 各種ヒープを作成し、インデックスベースで割り当て・解放する。
/// 解放されたスロットはフリーリストで再利用される。
class DescriptorHeap
{
public:
    /// @brief 割り当て失敗時に返される無効インデックス
    static constexpr uint32_t k_InvalidIndex = UINT32_MAX;

    DescriptorHeap() = default;
    ~DescriptorHeap() = default;

    /// @brief ディスクリプタヒープを作成する
    /// @param device D3D12デバイスへのポインタ
    /// @param type ヒープの種類（RTV / DSV / CBV_SRV_UAV / SAMPLER）
    /// @param numDescriptors ヒープの最大スロット数
    /// @param shaderVisible trueならシェーダーから参照可能（CBV_SRV_UAV/SAMPLERのみ有効）
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device,
                    D3D12_DESCRIPTOR_HEAP_TYPE type,
                    uint32_t numDescriptors,
                    bool shaderVisible = false);

    /// @brief 空きスロットを割り当ててインデックスを返す
    /// @return 割り当てられたインデックス（満杯ならk_InvalidIndex）
    uint32_t AllocateIndex();

    /// @brief 空きスロットを割り当ててCPUハンドルを返す
    /// @return 割り当てたスロットのCPUディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate();

    /// @brief 指定インデックスのCPUハンドルを取得する
    /// @param index スロット番号
    /// @return CPUディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

    /// @brief 指定インデックスのGPUハンドルを取得する
    /// @param index スロット番号
    /// @return GPUディスクリプタハンドル（shaderVisible=trueで作成した場合のみ有効）
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;

    /// @brief 内部のID3D12DescriptorHeapを取得する
    /// @return ID3D12DescriptorHeapへのポインタ
    ID3D12DescriptorHeap* GetHeap() const { return m_heap.Get(); }

    /// @brief 1ディスクリプタあたりのバイトサイズを取得する
    /// @return ディスクリプタサイズ（GPU依存値）
    uint32_t GetDescriptorSize() const { return m_descriptorSize; }

    /// @brief 割り当て済みスロットを解放する（フリーリストに返却）
    /// @param index 解放するスロット番号
    void Free(uint32_t index);

private:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE   m_type = {};
    uint32_t m_descriptorSize   = 0;    ///< GPU依存のディスクリプタ1つのサイズ
    uint32_t m_numDescriptors   = 0;    ///< ヒープの総容量
    uint32_t m_currentIndex     = 0;    ///< 次に割り当てる位置（線形割り当て用）
    std::vector<uint32_t> m_freeList;   ///< 解放されたインデックスの再利用リスト
};

} // namespace GX
