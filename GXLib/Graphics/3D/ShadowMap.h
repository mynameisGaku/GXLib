#pragma once
/// @file ShadowMap.h
/// @brief シャドウマップリソース管理
/// 深度バッファとSRVをペアで管理する。影の描画パスでDSVに書き込み、
/// メインパスでSRVとしてシェーダーからサンプルする。

#include "pch.h"
#include "Graphics/Resource/DepthBuffer.h"

namespace GX
{

/// @brief 単一シャドウマップ（深度バッファ + SRV）
/// CascadedShadowMapの各カスケードや単独のシャドウマップとして使用する。
class ShadowMap
{
public:
    ShadowMap() = default;
    ~ShadowMap() = default;

    /// @brief シャドウマップを作成する
    /// @param device D3D12デバイス
    /// @param size マップの解像度（正方形）
    /// @param srvHeap SRVを配置するヒープ
    /// @param srvIndex SRVのインデックス
    /// @return 成功ならtrue
    bool Create(ID3D12Device* device, uint32_t size,
                DescriptorHeap* srvHeap, uint32_t srvIndex);

    /// @brief DSV（深度ステンシルビュー）ハンドルを取得する
    /// @return DSVのCPUハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_depthBuffer.GetDSVHandle(); }

    /// @brief SRVのGPUハンドルを取得する（テクスチャとしてシェーダーからサンプルする用）
    /// @return SRVのGPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_depthBuffer.GetSRVGPUHandle(); }

    /// @brief 基礎リソースを取得する（バリア発行用）
    /// @return リソースポインタ
    ID3D12Resource* GetResource() const { return m_depthBuffer.GetResource(); }

    /// @brief マップの解像度を取得する
    /// @return 解像度（ピクセル）
    uint32_t GetSize() const { return m_size; }

    /// @brief 現在のリソース状態を取得する（バリア管理用）
    /// @return リソース状態
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /// @brief リソース状態を設定する（バリア発行後に呼ぶ）
    /// @param state 新しいリソース状態
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

private:
    DepthBuffer m_depthBuffer;
    uint32_t    m_size = 0;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

} // namespace GX
