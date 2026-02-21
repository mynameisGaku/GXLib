#pragma once
/// @file PointShadowMap.h
/// @brief ポイントライト用キューブシャドウマップ（Texture2DArray 6面）
/// ポイントライトから全方向に影を投射するためのキューブマップ方式シャドウ。
/// 6面(+X,-X,+Y,-Y,+Z,-Z)それぞれに正方形の深度マップを描画する。

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief ポイントライト用の全方向シャドウマップ（キューブマップ方式、1024x1024x6面）
class PointShadowMap
{
public:
    static constexpr uint32_t k_NumFaces = 6;     ///< キューブマップの面数
    static constexpr uint32_t k_Size = 1024;       ///< 各面の解像度

    PointShadowMap() = default;
    ~PointShadowMap() = default;

    /// @brief キューブシャドウマップを作成する（SRVは外部ヒープに配置）
    /// @param device D3D12デバイス
    /// @param srvHeap SRVを配置するヒープ
    /// @param srvIndex SRVのインデックス
    /// @return 成功ならtrue
    bool Create(ID3D12Device* device, DescriptorHeap* srvHeap, uint32_t srvIndex);

    /// @brief ライト位置と影の届く範囲から6面のVP行列を更新する
    /// @param lightPos ライトのワールド座標
    /// @param range 影が届く最大距離
    void Update(const XMFLOAT3& lightPos, float range);

    /// @brief 指定面のDSVハンドルを取得する
    /// @param face 面インデックス (0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z)
    /// @return DSVのCPUハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetFaceDSVHandle(uint32_t face) const;

    /// @brief SRVのGPUハンドルを取得する（Texture2DArrayとしてシェーダーからサンプル）
    /// @return SRVのGPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    /// @brief 基礎リソースを取得する（バリア発行用）
    /// @return リソースポインタ
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// @brief 指定面のVP行列を取得する（転置済み）
    /// @param face 面インデックス
    /// @return VP行列
    const XMFLOAT4X4& GetFaceVP(uint32_t face) const { return m_faceVP[face]; }

    /// @brief 現在のリソース状態を取得する
    /// @return リソース状態
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /// @brief リソース状態を設定する
    /// @param state 新しいリソース状態
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

private:
    ComPtr<ID3D12Resource>       m_resource;
    DescriptorHeap               m_dsvHeap;            ///< 6面分のDSVヒープ
    D3D12_GPU_DESCRIPTOR_HANDLE  m_srvGPUHandle = {};
    XMFLOAT4X4                   m_faceVP[k_NumFaces] = {};
    D3D12_RESOURCE_STATES        m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

} // namespace GX
