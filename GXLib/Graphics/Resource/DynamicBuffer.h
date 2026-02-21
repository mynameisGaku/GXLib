#pragma once
/// @file DynamicBuffer.h
/// @brief 毎フレーム書き換え可能なダイナミックバッファ
///
/// SpriteBatchやPrimitiveBatchの頂点データは毎フレーム変化するため、
/// 静的なBufferではなくUPLOADヒープ上の書き換え可能なバッファを使う。
///
/// ダブルバッファリング:
/// GPUが前のフレームのバッファを読んでいる最中に、CPUが次フレームの
/// バッファに書き込めるよう、内部で2面のバッファを交互に切り替える。
/// DxLibでは内部で同等の処理が行われているが、D3D12では自前で管理する。

#include "pch.h"

namespace GX
{

/// @brief 毎フレーム書き換え可能なUPLOADヒープバッファ
///
/// 定数バッファやスプライトの頂点バッファなど、フレームごとに内容が変わる
/// データの転送に使う。内部で2面のバッファを持ち、GPU競合を回避する。
class DynamicBuffer
{
public:
    /// ダブルバッファリングのバッファ面数
    static constexpr uint32_t k_BufferCount = 2;

    DynamicBuffer() = default;
    ~DynamicBuffer() = default;

    /// @brief バッファを作成する
    /// @param device D3D12デバイス
    /// @param maxSize バッファの最大サイズ（バイト）
    /// @param stride 1要素あたりのバイトサイズ（頂点バッファビューで使用）
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, uint32_t maxSize, uint32_t stride);

    /// @brief CPUからの書き込み用にバッファをマップする
    /// @param frameIndex 現在のフレームインデックス（0 or 1、SwapChainと同期）
    /// @return マップされたメモリのポインタ。失敗時nullptr
    void* Map(uint32_t frameIndex);

    /// @brief マップを解除する
    /// @param frameIndex Map時と同じフレームインデックス
    void Unmap(uint32_t frameIndex);

    /// @brief 頂点バッファビューを取得する
    /// @param frameIndex 現在のフレームインデックス
    /// @param usedSize 実際に使用しているバイトサイズ
    /// @return 頂点バッファビュー
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t frameIndex, uint32_t usedSize) const;

    /// @brief インデックスバッファビューを取得する
    /// @param frameIndex 現在のフレームインデックス
    /// @param usedSize 実際に使用しているバイトサイズ
    /// @param format インデックスのフォーマット
    /// @return インデックスバッファビュー
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(uint32_t frameIndex, uint32_t usedSize,
                                                DXGI_FORMAT format = DXGI_FORMAT_R16_UINT) const;

    /// @brief GPU仮想アドレスを取得する
    /// @param frameIndex 現在のフレームインデックス
    /// @return GPU仮想アドレス
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const;

    /// @brief 最大バッファサイズを取得する
    /// @return バイト単位のサイズ
    uint32_t GetMaxSize() const { return m_maxSize; }

    /// @brief 1要素あたりのストライドを取得する
    /// @return バイト単位のストライド
    uint32_t GetStride() const { return m_stride; }

private:
    std::array<ComPtr<ID3D12Resource>, k_BufferCount> m_buffers; ///< ダブルバッファ用リソース
    uint32_t m_maxSize = 0; ///< 1面あたりの最大サイズ
    uint32_t m_stride  = 0; ///< 1要素のストライド
};

} // namespace GX
