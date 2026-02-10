#pragma once
/// @file MotionBlur.h
/// @brief カメラベース Motion Blur ポストエフェクト
///
/// 深度バッファからワールド座標を再構成し、前フレームVP行列で再投影して
/// 速度ベクトルを求め、HDRシーンを速度方向にブラーする。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// Motion Blur 定数バッファ
struct MotionBlurConstants
{
    XMFLOAT4X4 invViewProjection;       // 現フレーム逆VP (row-major transposed)
    XMFLOAT4X4 previousViewProjection;  // 前フレームVP (row-major transposed)
    float intensity;
    int   sampleCount;
    float padding[2];
};  // 144B → 256-align

/// @brief カメラベース Motion Blur エフェクト
class MotionBlur
{
public:
    MotionBlur() = default;
    ~MotionBlur() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

    void SetSampleCount(int n) { m_sampleCount = n; }
    int GetSampleCount() const { return m_sampleCount; }

    /// 前フレームVP行列を更新（フレーム末に呼ぶ）
    void UpdatePreviousVP(const Camera3D& camera);

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_intensity = 1.0f;
    int m_sampleCount = 16;

    XMFLOAT4X4 m_previousVP;       // 前フレームのViewProjection
    bool m_hasPreviousVP = false;   // 初回フレームはスキップ

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
