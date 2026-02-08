#pragma once
/// @file Buffer.h
/// @brief 頂点バッファ/インデックスバッファ管理クラス
///
/// 【初学者向け解説】
/// GPUが三角形を描画するには、頂点（三角形の角の座標）のデータが必要です。
/// このデータをGPUがアクセスできるメモリ（GPUメモリ）に置く必要があります。
///
/// - 頂点バッファ: 頂点の位置、色、UV座標などを格納するバッファ
/// - インデックスバッファ: 頂点の共有方法を定義する番号の配列
///
/// Phase 0ではシンプルにUPLOADヒープ（CPUとGPU両方からアクセス可能）を使います。
/// 本格的な実装では、DEFAULTヒープ（GPU専用、高速）にコピーしますが、
/// 今は簡単のためにUPLOADヒープを直接使用します。

#include "pch.h"

namespace GX
{

/// @brief GPUバッファを管理するクラス
class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    /// 頂点バッファを作成
    bool CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride);

    /// インデックスバッファを作成
    bool CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size,
                           DXGI_FORMAT format = DXGI_FORMAT_R16_UINT);

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vbv; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_ibv; }
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

private:
    bool CreateUploadBuffer(ID3D12Device* device, const void* data, uint32_t size);

    ComPtr<ID3D12Resource>   m_resource;
    D3D12_VERTEX_BUFFER_VIEW m_vbv = {};
    D3D12_INDEX_BUFFER_VIEW  m_ibv = {};
};

} // namespace GX
