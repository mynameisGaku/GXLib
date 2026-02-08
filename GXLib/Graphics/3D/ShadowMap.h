#pragma once
/// @file ShadowMap.h
/// @brief シャドウマップリソース管理

#include "pch.h"
#include "Graphics/Resource/DepthBuffer.h"

namespace GX
{

/// @brief 単一シャドウマップ（深度バッファ + SRV）
class ShadowMap
{
public:
    ShadowMap() = default;
    ~ShadowMap() = default;

    /// シャドウマップを作成
    bool Create(ID3D12Device* device, uint32_t size,
                DescriptorHeap* srvHeap, uint32_t srvIndex);

    /// DSVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_depthBuffer.GetDSVHandle(); }

    /// SRVのGPUハンドル取得（テクスチャとして読む用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_depthBuffer.GetSRVGPUHandle(); }

    /// リソース取得（バリア用）
    ID3D12Resource* GetResource() const { return m_depthBuffer.GetResource(); }

    uint32_t GetSize() const { return m_size; }

    /// 現在のリソース状態を取得/設定（バリア管理用）
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

private:
    DepthBuffer m_depthBuffer;
    uint32_t    m_size = 0;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

} // namespace GX
