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

    /// SRVのGPUハンドルを取得（シャドウマップ用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    /// リソースを取得
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    DXGI_FORMAT GetFormat() const { return m_format; }

private:
    ComPtr<ID3D12Resource> m_resource;
    DescriptorHeap         m_dsvHeap;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGPUHandle = {};
    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_D32_FLOAT;
};

} // namespace GX
