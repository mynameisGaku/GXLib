#pragma once
/// @file PointShadowMap.h
/// @brief ポイントライト用キューブシャドウマップ（Texture2DArray 6面）

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief ポイントライト用6面シャドウマップ
class PointShadowMap
{
public:
    static constexpr uint32_t k_NumFaces = 6;
    static constexpr uint32_t k_Size = 1024;

    PointShadowMap() = default;
    ~PointShadowMap() = default;

    /// 作成（SRVは外部ヒープに配置）
    bool Create(ID3D12Device* device, DescriptorHeap* srvHeap, uint32_t srvIndex);

    /// ライト位置とレンジから6面のVP行列を更新
    void Update(const XMFLOAT3& lightPos, float range);

    /// 指定面のDSVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetFaceDSVHandle(uint32_t face) const;

    /// SRVのGPUハンドル取得（Texture2DArray）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    /// リソース取得（バリア用）
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// 指定面のVP行列取得
    const XMFLOAT4X4& GetFaceVP(uint32_t face) const { return m_faceVP[face]; }

    /// リソースステート管理
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

private:
    ComPtr<ID3D12Resource>       m_resource;
    DescriptorHeap               m_dsvHeap;  // 6 DSVs
    D3D12_GPU_DESCRIPTOR_HANDLE  m_srvGPUHandle = {};
    XMFLOAT4X4                   m_faceVP[k_NumFaces] = {};
    D3D12_RESOURCE_STATES        m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

} // namespace GX
