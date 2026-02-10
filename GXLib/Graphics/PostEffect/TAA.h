#pragma once
/// @file TAA.h
/// @brief Temporal Anti-Aliasing (TAA) ポストエフェクト
///
/// 現フレームとリプロジェクションした前フレーム履歴を、
/// 近傍クランプ付きでブレンドしてサブピクセル情報を蓄積する。
/// Halton(2,3)ジッターでプロジェクション行列をサブピクセルシフトする。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// TAA 定数バッファ
struct TAAConstants
{
    XMFLOAT4X4 invViewProjection;       // 64B: 現フレーム逆VP (非ジッター)
    XMFLOAT4X4 previousViewProjection;  // 64B: 前フレームVP (非ジッター)
    XMFLOAT2   jitterOffset;            // 8B: 現フレームジッター (NDC)
    float      blendFactor;             // 4B: 履歴ウェイト (0.9)
    float      screenWidth;             // 4B
    float      screenHeight;            // 4B
    float      padding[3];             // 12B
};  // 160B → 256-align

/// @brief Temporal Anti-Aliasing エフェクト
class TAA
{
public:
    TAA() = default;
    ~TAA() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// TAA実行: srcHDR → destHDR、完了後 destHDR → historyRT にコピー
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetBlendFactor(float f) { m_blendFactor = f; }
    float GetBlendFactor() const { return m_blendFactor; }

    /// 現フレームのジッターオフセットを取得 (NDC空間)
    XMFLOAT2 GetCurrentJitter() const;

    /// 前フレームVP行列を更新（フレーム末に呼ぶ）
    void UpdatePreviousVP(const Camera3D& camera);

    /// フレームカウントを進める
    void AdvanceFrame() { m_frameCount++; }

    uint32_t GetFrameCount() const { return m_frameCount; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_blendFactor = 0.9f;

    RenderTarget m_historyRT;       // R16G16B16A16_FLOAT (前フレームTAA出力)
    bool m_hasHistory = false;

    XMFLOAT4X4 m_previousVP;
    bool m_hasPreviousVP = false;
    uint32_t m_frameCount = 0;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 3テクスチャ(scene + history + depth)用専用SRVヒープ (3スロット × 2フレーム = 6)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);

    /// Halton数列 (base=2,3) ジッター生成
    static float Halton(int index, int base);
};

} // namespace GX
