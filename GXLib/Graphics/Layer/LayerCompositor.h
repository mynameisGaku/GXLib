#pragma once
/// @file LayerCompositor.h
/// @brief レイヤーコンポジター (合成器)
///
/// DxLibには直接対応する機能が無い。LayerStackの全レイヤーをZ-order順に
/// フルスクリーン三角形で合成してバックバッファに出力する。
/// ブレンドモード別のPSO (Alpha/Add/Sub/Mul/Screen/None) とマスク対応PSOを保持。

#include "pch.h"
#include "Graphics/Layer/LayerStack.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief レイヤー合成用定数バッファ
struct CompositeConstants
{
    float opacity;    ///< レイヤーの不透明度 (0〜1)
    float hasMask;    ///< マスク使用フラグ (1.0でマスクあり)
    float padding[2];
};

/// @brief 全レイヤーをブレンドモード別にバックバッファへ合成するコンポジター
///
/// ブレンドモード毎に専用PSOを保持し、マスク付きレイヤーは2テクスチャ入力の
/// 専用PSOで描画する。フルスクリーン三角形 (SV_VertexID、VB不要) で合成。
class LayerCompositor
{
public:
    LayerCompositor() = default;
    ~LayerCompositor() = default;

    /// @brief 初期化。全ブレンドモードのPSO・マスク用SRVヒープを作成する
    /// @param device D3D12デバイス
    /// @param w 画面幅
    /// @param h 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t w, uint32_t h);

    /// @brief 全レイヤーをZ-order順に合成してバックバッファに描画する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param backBufferRTV バックバッファのRTVハンドル
    /// @param layerStack 合成対象のレイヤースタック
    void Composite(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                   D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                   LayerStack& layerStack);

    /// @brief 画面リサイズ対応
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
