#pragma once
/// @file OutlineEffect.h
/// @brief エッジ検出アウトラインポストエフェクト
///
/// 深度バッファから法線を再構成し、Sobel エッジ検出で
/// アウトラインを合成する。Forward+ (GBuffer無し) 環境向け。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// OutlineEffect 定数バッファ
struct OutlineConstants
{
    XMFLOAT4X4 invProjection;  // 逆射影行列 (row-major transposed) 64B
    float depthThreshold;       // ビュー空間Z差のエッジ閾値
    float normalThreshold;      // 法線ドット積エッジ閾値
    float intensity;            // アウトライン強度
    float screenWidth;
    float screenHeight;         // offset 80
    float nearZ;
    float padding[2];           // offset 88-95 → 96
    XMFLOAT4 lineColor;        // offset 96 (RGBA)
};  // 112B → 256-align

/// @brief エッジ検出アウトラインエフェクト
class OutlineEffect
{
public:
    OutlineEffect() = default;
    ~OutlineEffect() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetDepthThreshold(float t) { m_depthThreshold = t; }
    float GetDepthThreshold() const { return m_depthThreshold; }

    void SetNormalThreshold(float t) { m_normalThreshold = t; }
    float GetNormalThreshold() const { return m_normalThreshold; }

    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

    void SetLineColor(const XMFLOAT4& color) { m_lineColor = color; }
    const XMFLOAT4& GetLineColor() const { return m_lineColor; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_depthThreshold  = 0.5f;
    float m_normalThreshold = 0.3f;
    float m_intensity       = 1.0f;
    XMFLOAT4 m_lineColor   = { 0.0f, 0.0f, 0.0f, 1.0f };

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 2テクスチャ(scene + depth)用専用SRVヒープ (2スロット × 2フレーム = 4)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);
};

} // namespace GX
