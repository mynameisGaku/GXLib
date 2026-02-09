#pragma once
/// @file LayerCompositor.h
/// @brief レイヤーコンポジター
///
/// Z-order順にレイヤーをフルスクリーン描画で合成し、バックバッファに出力する。
/// ブレンドモード別のPSO、マスク対応のPSOを保持する。

#include "pch.h"
#include "Graphics/Layer/LayerStack.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief 合成用定数バッファ
struct CompositeConstants
{
    float opacity;
    float hasMask;
    float padding[2];
};

/// @brief レイヤーコンポジター
class LayerCompositor
{
public:
    LayerCompositor() = default;
    ~LayerCompositor() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device, uint32_t w, uint32_t h);

    /// 全レイヤーをZ-order順に合成してバックバッファに描画
    void Composite(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                   D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                   LayerStack& layerStack);

    /// リサイズ
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

private:
    bool CreatePipelines(ID3D12Device* device);
    void DrawLayer(RenderLayer& layer);
    void DrawLayerMasked(RenderLayer& layer, RenderLayer& mask);

    ID3D12Device* m_device = nullptr;
    ID3D12GraphicsCommandList* m_cmdList = nullptr;
    uint32_t m_frameIndex = 0;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    Shader m_shader;

    // マスクなし RS: b0 + t0 + s0
    ComPtr<ID3D12RootSignature> m_rsNoMask;
    // マスクあり RS: b0 + DescTable(t0,t1) + s0
    ComPtr<ID3D12RootSignature> m_rsMask;

    // ブレンドモード別 PSO (マスクなし)
    ComPtr<ID3D12PipelineState> m_psoAlpha;
    ComPtr<ID3D12PipelineState> m_psoAdd;
    ComPtr<ID3D12PipelineState> m_psoSub;
    ComPtr<ID3D12PipelineState> m_psoMul;
    ComPtr<ID3D12PipelineState> m_psoScreen;
    ComPtr<ID3D12PipelineState> m_psoNone;

    // マスクあり PSO (Alpha/Add)
    ComPtr<ID3D12PipelineState> m_psoAlphaMask;
    ComPtr<ID3D12PipelineState> m_psoAddMask;

    DynamicBuffer m_compositeCB;

    // マスク用 SRV ヒープ: 2スロット x 2フレーム = 4
    DescriptorHeap m_maskSrvHeap;
};

} // namespace GX
