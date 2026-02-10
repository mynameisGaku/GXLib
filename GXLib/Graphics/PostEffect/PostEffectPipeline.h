#pragma once
/// @file PostEffectPipeline.h
/// @brief ポストエフェクトパイプライン管理
///
/// エフェクトチェーン:
/// HDRシーン → [ブルーム] → [カラーグレーディング] → [トーンマッピング] → [FXAA] → [ビネット+色収差] → バックバッファ

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/PostEffect/Bloom.h"
#include "Graphics/PostEffect/SSAO.h"
#include "Graphics/PostEffect/DepthOfField.h"
#include "Graphics/PostEffect/MotionBlur.h"
#include "Graphics/PostEffect/SSR.h"
#include "Graphics/PostEffect/OutlineEffect.h"
#include "Graphics/PostEffect/VolumetricLight.h"
#include "Graphics/PostEffect/TAA.h"
#include "Graphics/PostEffect/AutoExposure.h"

namespace GX
{

enum class TonemapMode : uint32_t { Reinhard = 0, ACES = 1, Uncharted2 = 2 };

struct TonemapConstants { uint32_t mode; float exposure; float padding[2]; };
struct FXAAConstants    { float rcpFrameX; float rcpFrameY; float qualitySubpix; float edgeThreshold; };
struct VignetteConstants{ float intensity; float radius; float chromaticStrength; float padding; };
struct ColorGradingConstants { float exposure; float contrast; float saturation; float temperature; };

/// @brief ポストエフェクトパイプライン
class PostEffectPipeline
{
public:
    PostEffectPipeline() = default;
    ~PostEffectPipeline() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    void BeginScene(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                     D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, Camera3D& camera);
    void EndScene();
    void Resolve(D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                 DepthBuffer& depthBuffer, const Camera3D& camera,
                 float deltaTime = 0.0f);

    // --- トーンマッピング ---
    void SetTonemapMode(TonemapMode mode) { m_tonemapMode = mode; }
    TonemapMode GetTonemapMode() const { return m_tonemapMode; }
    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }

    // --- SSAO ---
    SSAO& GetSSAO() { return m_ssao; }
    const SSAO& GetSSAO() const { return m_ssao; }

    // --- ブルーム ---
    Bloom& GetBloom() { return m_bloom; }
    const Bloom& GetBloom() const { return m_bloom; }

    // --- DoF (被写界深度) ---
    DepthOfField& GetDoF() { return m_dof; }
    const DepthOfField& GetDoF() const { return m_dof; }

    // --- モーションブラー ---
    MotionBlur& GetMotionBlur() { return m_motionBlur; }
    const MotionBlur& GetMotionBlur() const { return m_motionBlur; }

    // --- SSR (スクリーン空間反射) ---
    SSR& GetSSR() { return m_ssr; }
    const SSR& GetSSR() const { return m_ssr; }

    // --- アウトライン ---
    OutlineEffect& GetOutline() { return m_outline; }
    const OutlineEffect& GetOutline() const { return m_outline; }

    // --- ボリューメトリックライト ---
    VolumetricLight& GetVolumetricLight() { return m_volumetricLight; }
    const VolumetricLight& GetVolumetricLight() const { return m_volumetricLight; }

    // --- TAA (時間的AA) ---
    TAA& GetTAA() { return m_taa; }
    const TAA& GetTAA() const { return m_taa; }

    // --- 自動露出 ---
    AutoExposure& GetAutoExposure() { return m_autoExposure; }
    const AutoExposure& GetAutoExposure() const { return m_autoExposure; }

    // --- FXAA ---
    void SetFXAAEnabled(bool enabled) { m_fxaaEnabled = enabled; }
    bool IsFXAAEnabled() const { return m_fxaaEnabled; }

    // --- ビネット ---
    void SetVignetteEnabled(bool enabled) { m_vignetteEnabled = enabled; }
    bool IsVignetteEnabled() const { return m_vignetteEnabled; }
    void SetVignetteIntensity(float v) { m_vignetteIntensity = v; }
    float GetVignetteIntensity() const { return m_vignetteIntensity; }
    void SetChromaticAberration(float v) { m_chromaticStrength = v; }
    float GetChromaticAberration() const { return m_chromaticStrength; }

    // --- カラーグレーディング ---
    void SetColorGradingEnabled(bool enabled) { m_colorGradingEnabled = enabled; }
    bool IsColorGradingEnabled() const { return m_colorGradingEnabled; }
    void SetContrast(float v) { m_contrast = v; }
    float GetContrast() const { return m_contrast; }
    void SetSaturation(float v) { m_saturation = v; }
    float GetSaturation() const { return m_saturation; }
    void SetTemperature(float v) { m_temperature = v; }
    float GetTemperature() const { return m_temperature; }

    // --- 設定ファイル ---
    bool LoadSettings(const std::string& filePath);
    bool SaveSettings(const std::string& filePath) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetHDRRTVHandle() const;
    DXGI_FORMAT GetHDRFormat() const { return k_HDRFormat; }
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

private:
    bool CreatePipelines(ID3D12Device* device);

    /// フルスクリーン描画ヘルパー（RenderTarget → RenderTarget）
    void DrawFullscreen(ID3D12PipelineState* pso, ID3D12RootSignature* rs,
                        RenderTarget& dest, RenderTarget& src,
                        DynamicBuffer& cb, const void* cbData, uint32_t cbSize);

    /// フルスクリーン描画ヘルパー（RenderTarget → 生RTVハンドル, バックバッファ向け）
    void DrawFullscreenToRTV(ID3D12PipelineState* pso, ID3D12RootSignature* rs,
                             D3D12_CPU_DESCRIPTOR_HANDLE destRTV,
                             RenderTarget& src,
                             DynamicBuffer& cb, const void* cbData, uint32_t cbSize);

    static constexpr DXGI_FORMAT k_HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static constexpr DXGI_FORMAT k_LDRFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    ID3D12Device*              m_device    = nullptr;
    ID3D12GraphicsCommandList* m_cmdList   = nullptr;
    uint32_t                   m_frameIndex = 0;
    uint32_t                   m_width  = 0;
    uint32_t                   m_height = 0;

    // HDR用RT (高ダイナミックレンジ)
    RenderTarget m_hdrRT;
    RenderTarget m_hdrPingPongRT;

    // LDR用RT (FXAA/Vignette用)
    RenderTarget m_ldrRT[2];

    // SSAO
    SSAO m_ssao;

    // ブルーム
    Bloom m_bloom;

    // DoF (被写界深度)
    DepthOfField m_dof;

    // モーションブラー
    MotionBlur m_motionBlur;

    // SSR (スクリーン空間反射)
    SSR m_ssr;

    // アウトライン
    OutlineEffect m_outline;

    // ボリューメトリックライト
    VolumetricLight m_volumetricLight;

    // TAA (時間的AA)
    TAA m_taa;

    // 自動露出
    AutoExposure m_autoExposure;

    // シェーダー & パイプライン
    Shader m_shader;

    // トーンマッピング
    ComPtr<ID3D12RootSignature> m_commonRS;  // 全エフェクト共通: b0 + t0 + s0
    ComPtr<ID3D12PipelineState> m_tonemapPSO;
    DynamicBuffer               m_tonemapCB;

    // FXAA
    ComPtr<ID3D12PipelineState> m_fxaaPSO;
    DynamicBuffer               m_fxaaCB;

    // ビネット + 色収差
    ComPtr<ID3D12PipelineState> m_vignettePSO;
    DynamicBuffer               m_vignetteCB;

    // カラーグレーディング (HDR空間)
    ComPtr<ID3D12PipelineState> m_colorGradingHDRPSO;
    DynamicBuffer               m_colorGradingCB;

    // パラメータ
    TonemapMode m_tonemapMode = TonemapMode::ACES;
    float m_exposure = 1.0f;

    bool  m_fxaaEnabled = true;

    bool  m_vignetteEnabled = false;
    float m_vignetteIntensity = 0.5f;
    float m_chromaticStrength = 0.003f;

    bool  m_colorGradingEnabled = false;
    float m_contrast    = 1.0f;
    float m_saturation  = 1.0f;
    float m_temperature = 0.0f;
};

} // namespace GX
