#pragma once
/// @file SSR.h
/// @brief Screen Space Reflections ポストエフェクト
///
/// ビュー空間でレイマーチングを行い、深度バッファとの交差から
/// 反射色を取得する。法線は深度勾配から再構成。
/// Forward+レンダリング (GBuffer無し) 環境向け。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// SSR 定数バッファ
struct SSRConstants
{
    XMFLOAT4X4 projection;       // 射影行列 (row-major transposed)
    XMFLOAT4X4 invProjection;    // 逆射影行列 (row-major transposed)
    XMFLOAT4X4 view;             // ビュー行列 (row-major transposed)
    float maxDistance;
    float stepSize;
    int   maxSteps;
    float thickness;
    float intensity;
    float screenWidth;
    float screenHeight;
    float nearZ;
};  // 224B → 256-align

/// @brief Screen Space Reflections エフェクト
class SSR
{
public:
    SSR() = default;
    ~SSR() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetMaxDistance(float d) { m_maxDistance = d; }
    float GetMaxDistance() const { return m_maxDistance; }

    void SetStepSize(float s) { m_stepSize = s; }
    float GetStepSize() const { return m_stepSize; }

    void SetMaxSteps(int n) { m_maxSteps = n; }
    int GetMaxSteps() const { return m_maxSteps; }

    void SetThickness(float t) { m_thickness = t; }
    float GetThickness() const { return m_thickness; }

    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_maxDistance = 30.0f;
    float m_stepSize = 0.15f;
    int m_maxSteps = 200;
    float m_thickness = 0.15f;    // ビュー空間: ≈1ステップ分
    float m_intensity = 1.0f;

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
