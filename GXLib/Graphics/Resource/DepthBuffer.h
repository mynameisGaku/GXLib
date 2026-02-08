#pragma once
/// @file DepthBuffer.h
/// @brief 深度バッファ管理クラス
///
/// 【初学者向け解説】
/// 深度バッファ（Depth Buffer / Z-Buffer）は、各ピクセルの奥行き情報を
/// 記録するためのバッファです。3D描画で手前のオブジェクトが奥のオブジェクトを
/// 正しく隠すために使います。
///
/// Phase 1では2D描画が主体なので深度テストは無効ですが、
/// 将来の3D対応や描画順序制御のために基盤として用意しておきます。

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief 深度バッファ管理クラス
class DepthBuffer
{
public:
    DepthBuffer() = default;
    ~DepthBuffer() = default;

    /// 深度バッファを作成
    bool Create(ID3D12Device* device, uint32_t width, uint32_t height,
                DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT);

    /// 深度バッファをDSV+SRV付きで作成（シャドウマップ用）
    bool CreateWithSRV(ID3D12Device* device, uint32_t width, uint32_t height,
                        DescriptorHeap* srvHeap, uint32_t srvIndex);

    /// DSVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;

    /// 深度バッファをDSV+自前shader-visible SRV付きで作成（SSAO用）
    bool CreateWithOwnSRV(ID3D12Device* device, uint32_t width, uint32_t height);

    /// SRVのGPUハンドルを取得（シャドウマップ用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    /// リソースを取得
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    DXGI_FORMAT GetFormat() const { return m_format; }

    /// リソースバリアを発行してステート遷移
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /// 自前SRVヒープを取得（SSAO用）
    DescriptorHeap& GetOwnSRVHeap() { return m_ownSrvHeap; }
    bool HasOwnSRV() const { return m_hasOwnSRV; }

private:
    ComPtr<ID3D12Resource> m_resource;
    DescriptorHeap         m_dsvHeap;
    DescriptorHeap         m_ownSrvHeap;    // 自前shader-visible SRVヒープ (SSAO用)
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGPUHandle = {};
    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_D32_FLOAT;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    bool m_hasOwnSRV = false;
};

} // namespace GX
