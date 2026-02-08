#pragma once
/// @file RenderTarget.h
/// @brief オフスクリーンレンダーターゲット
///
/// 【初学者向け解説】
/// RenderTargetは、画面に直接描画するのではなく、一旦テクスチャに描画するための
/// 仕組みです。DXLibのMakeScreen()に相当します。
///
/// 用途：
/// - ポストエフェクト（ブラー、モザイクなど）
/// - ミニマップの描画
/// - シーンの合成
///
/// 描画先をRenderTargetに切り替え → 描画 → 元に戻す → テクスチャとして利用

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief オフスクリーンレンダーターゲット
class RenderTarget
{
public:
    RenderTarget() = default;
    ~RenderTarget() = default;

    /// レンダーターゲットを作成
    bool Create(ID3D12Device* device,
                uint32_t width, uint32_t height,
                DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

    /// レンダーターゲットのRTVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const;

    /// SRVのGPUハンドルを取得（テクスチャとして読む時用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;

    /// リソースを取得
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    DXGI_FORMAT GetFormat() const { return m_format; }

    /// SRVヒープを取得（外部からディスクリプタヒープをバインドするため）
    DescriptorHeap& GetSRVHeap() { return m_srvHeap; }

    /// 現在のリソースステートを取得
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /// リソースステートを設定（外部でバリアを発行した後に呼ぶ）
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

    /// リソースバリアを発行してステート遷移
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

private:
    ComPtr<ID3D12Resource> m_resource;
    DescriptorHeap         m_rtvHeap;
    DescriptorHeap         m_srvHeap;
    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};

} // namespace GX
