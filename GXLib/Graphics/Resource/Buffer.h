#pragma once
/// @file Buffer.h
/// @brief GPUバッファ（頂点/インデックス/汎用）の作成と管理
///
/// DxLibでは頂点バッファやインデックスバッファは内部で自動管理されるが、
/// DirectX 12ではアプリケーション側でGPUメモリの確保・転送を明示的に行う必要がある。
/// このクラスはその手続きをまとめたもの。
///
/// バッファには2種類のヒープがある:
/// - UPLOADヒープ: CPUから書き込み可能。頂点/インデックスデータの初期転送に使う
/// - DEFAULTヒープ: GPU専用で高速。BLAS/TLASなどレイトレ用途に使う

#include "pch.h"

namespace GX
{

/// @brief GPUバッファリソースを管理するクラス
///
/// 頂点バッファ、インデックスバッファ、レイトレ用スクラッチバッファなど、
/// 用途に応じたGPUバッファを作成できる。DxLibの内部バッファ管理に相当する。
class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    /// @brief 頂点バッファを作成する
    /// @param device D3D12デバイス
    /// @param data 頂点データの先頭ポインタ
    /// @param size データ全体のバイトサイズ
    /// @param stride 1頂点あたりのバイトサイズ
    /// @return 成功時true
    bool CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride);

    /// @brief インデックスバッファを作成する
    /// @param device D3D12デバイス
    /// @param data インデックスデータの先頭ポインタ
    /// @param size データ全体のバイトサイズ
    /// @param format インデックスのフォーマット（16bit or 32bit）
    /// @return 成功時true
    bool CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size,
                           DXGI_FORMAT format = DXGI_FORMAT_R16_UINT);

    /// @brief 頂点バッファビューを取得する
    /// @return D3D12が描画時に必要とするビュー構造体
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vbv; }

    /// @brief インデックスバッファビューを取得する
    /// @return D3D12が描画時に必要とするビュー構造体
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_ibv; }

    /// @brief 内部のID3D12Resourceを取得する
    /// @return リソースポインタ（未作成時はnullptr）
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// @brief GPU仮想アドレスを取得する（ルートSRV等に必要）
    /// @return GPU仮想アドレス。リソース未作成時は0
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_resource ? m_resource->GetGPUVirtualAddress() : 0; }

    /// @brief GPU専用（DEFAULTヒープ）バッファを作成する
    ///
    /// BLAS/TLASのスクラッチバッファや結果バッファなど、
    /// CPUからの直接アクセスが不要な用途向け。
    /// @param device D3D12デバイス
    /// @param size バッファサイズ（バイト）
    /// @param flags リソースフラグ（UAV許可など）
    /// @param initialState 初期リソースステート
    /// @return 成功時true
    bool CreateDefaultBuffer(ID3D12Device* device, uint64_t size,
                             D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                             D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);

    /// @brief 空のUPLOADバッファを作成する（Map/Unmapで後からデータを書き込む用）
    /// @param device D3D12デバイス
    /// @param size バッファサイズ（バイト）
    /// @return 成功時true
    bool CreateUploadBufferEmpty(ID3D12Device* device, uint64_t size);

    /// @brief バッファをCPUアドレス空間にマップする
    /// @return マップされたポインタ。失敗時はnullptr
    void* Map();

    /// @brief マップを解除する
    void Unmap();

private:
    /// UPLOADヒープにバッファを作成し、dataの内容をコピーする（内部実装）
    bool CreateUploadBuffer(ID3D12Device* device, const void* data, uint32_t size);

    ComPtr<ID3D12Resource>   m_resource;
    D3D12_VERTEX_BUFFER_VIEW m_vbv = {};
    D3D12_INDEX_BUFFER_VIEW  m_ibv = {};
};

} // namespace GX
