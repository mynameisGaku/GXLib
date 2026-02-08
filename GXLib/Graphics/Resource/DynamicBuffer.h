#pragma once
/// @file DynamicBuffer.h
/// @brief フレームごとに書き換え可能なダイナミックバッファ
///
/// 【初学者向け解説】
/// DynamicBufferは、毎フレームCPUからデータを書き換えるためのバッファです。
/// SpriteBatchやPrimitiveBatchの頂点データは毎フレーム変わるため、
/// 通常のバッファではなくUPLOADヒープのバッファを使います。
///
/// ダブルバッファリング対応：
/// GPUが前のフレームのバッファを読んでいる間に、
/// CPUが次のフレームのバッファに書き込めるよう、2つのバッファを交互に使います。

#include "pch.h"

namespace GX
{

/// @brief フレームごとに書き換え可能なUPLOADヒープバッファ
class DynamicBuffer
{
public:
    static constexpr uint32_t k_BufferCount = 2;

    DynamicBuffer() = default;
    ~DynamicBuffer() = default;

    /// バッファを作成
    /// @param device D3D12デバイス
    /// @param maxSize バッファの最大サイズ（バイト）
    /// @param stride 1要素あたりのサイズ（バイト）
    bool Initialize(ID3D12Device* device, uint32_t maxSize, uint32_t stride);

    /// CPUからの書き込み用にマップを開始
    /// @param frameIndex 現在のフレームインデックス（0 or 1）
    /// @return マップされたメモリのポインタ（nullptrで失敗）
    void* Map(uint32_t frameIndex);

    /// マップを解除
    void Unmap(uint32_t frameIndex);

    /// 頂点バッファビューを取得
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t frameIndex, uint32_t usedSize) const;

    /// インデックスバッファビューを取得
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(uint32_t frameIndex, uint32_t usedSize,
                                                DXGI_FORMAT format = DXGI_FORMAT_R16_UINT) const;

    /// バッファのGPU仮想アドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const;

    uint32_t GetMaxSize() const { return m_maxSize; }
    uint32_t GetStride() const { return m_stride; }

private:
    std::array<ComPtr<ID3D12Resource>, k_BufferCount> m_buffers;
    uint32_t m_maxSize = 0;
    uint32_t m_stride  = 0;
};

} // namespace GX
