#pragma once
/// @file DescriptorHeap.h
/// @brief ディスクリプタヒープ管理クラス
///
/// 【初学者向け解説】
/// ディスクリプタヒープとは、GPUがリソース（テクスチャ、定数バッファなど）の
/// 場所を知るための「住所録」のようなものです。
/// CPUがこの住所録に「このテクスチャはメモリのここにあるよ」と書き込み、
/// GPUがそれを読み取ってリソースにアクセスします。
///
/// DX12では以下の4種類のヒープがあります：
/// - RTV（Render Target View）: 描画先のバッファ用
/// - DSV（Depth Stencil View）: 深度バッファ用
/// - CBV_SRV_UAV: 定数バッファ、テクスチャ、読み書きバッファ用
/// - SAMPLER: テクスチャのサンプリング方法を定義
///
/// このクラスは、これらのヒープの作成と管理を簡単にするラッパーです。

#include "pch.h"

namespace GX
{

/// @brief ディスクリプタヒープを管理するクラス
class DescriptorHeap
{
public:
    DescriptorHeap() = default;
    ~DescriptorHeap() = default;

    /// ディスクリプタヒープを作成
    bool Initialize(ID3D12Device* device,
                    D3D12_DESCRIPTOR_HEAP_TYPE type,
                    uint32_t numDescriptors,
                    bool shaderVisible = false);

    /// 次のディスクリプタを割り当て、そのインデックスを返す
    uint32_t AllocateIndex();

    /// 次のディスクリプタのCPUハンドルを割り当てて返す
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate();

    /// 指定インデックスのCPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

    /// 指定インデックスのGPUハンドルを取得（shader-visibleの場合のみ有効）
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;

    /// D3D12ディスクリプタヒープを取得
    ID3D12DescriptorHeap* GetHeap() const { return m_heap.Get(); }

    /// ディスクリプタのサイズを取得
    uint32_t GetDescriptorSize() const { return m_descriptorSize; }

    /// 割り当て済みディスクリプタを解放（フリーリストに返却）
    void Free(uint32_t index);

private:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE   m_type = {};
    uint32_t m_descriptorSize   = 0;
    uint32_t m_numDescriptors   = 0;
    uint32_t m_currentIndex     = 0;
    std::vector<uint32_t> m_freeList;  ///< 解放されたインデックスの再利用リスト
};

} // namespace GX
