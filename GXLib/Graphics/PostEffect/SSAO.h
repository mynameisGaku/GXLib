#pragma once
/// @file SSAO.h
/// @brief Screen Space Ambient Occlusion
///
/// 深度バッファから遮蔽情報を計算し、角や凹部を暗くすることで
/// リアリティを向上させるポストエフェクト。
///
/// 処理の流れ:
/// 1. AO生成: 深度→ビュー空間位置復元→半球サンプリング→遮蔽計算
/// 2. バイラテラルブラー: 水平+垂直の2パスでノイズ除去（エッジ保持）
/// 3. 乗算合成: MultiplyBlend PSOでHDRシーンにAO値を乗算

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// SSAO生成定数バッファ
struct SSAOConstants
{
    XMFLOAT4X4 projection;       // 64B
    XMFLOAT4X4 invProjection;    // 64B
    XMFLOAT4   samples[64];     // 1024B
    float      radius;           // 4B
    float      bias;             // 4B
    float      power;            // 4B
    float      screenWidth;      // 4B
    float      screenHeight;     // 4B
    float      nearZ;            // 4B
    float      farZ;             // 4B
    float      padding;          // 4B
};  // Total: 1184B → 256-align → 1280B

/// ブラー定数バッファ
struct SSAOBlurConstants
{
    float blurDirX;
    float blurDirY;
    float padding[2];
};

/// @brief SSAOエフェクト
class SSAO
{
public:
    static constexpr uint32_t k_KernelSize = 64;

    SSAO() = default;
    ~SSAO() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& hdrRT, DepthBuffer& depthBuffer, const Camera3D& camera);

    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetRadius(float r) { m_radius = r; }
    float GetRadius() const { return m_radius; }

    void SetBias(float b) { m_bias = b; }
    float GetBias() const { return m_bias; }

    void SetPower(float p) { m_power = p; }
    float GetPower() const { return m_power; }

private:
    void GenerateKernel();
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = true;
    float m_radius = 0.5f;
    float m_bias   = 0.025f;
    float m_power  = 2.0f;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // AO出力RT (R8_UNORM)
    RenderTarget m_ssaoRT;
    // ブラー中間RT (R8_UNORM)
    RenderTarget m_blurTempRT;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_generateRS;
    ComPtr<ID3D12RootSignature> m_blurRS;
    ComPtr<ID3D12PipelineState> m_generatePSO;
    ComPtr<ID3D12PipelineState> m_blurHPSO;
    ComPtr<ID3D12PipelineState> m_blurVPSO;
    ComPtr<ID3D12PipelineState> m_compositePSO;

    // 定数バッファ
    DynamicBuffer m_generateCB;
    DynamicBuffer m_blurCB;

    // 半球カーネル
    XMFLOAT4 m_kernel[k_KernelSize];
};

} // namespace GX
