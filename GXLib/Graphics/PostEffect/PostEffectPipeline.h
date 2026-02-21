#pragma once
/// @file PostEffectPipeline.h
/// @brief ポストエフェクトパイプライン管理
///
/// DxLibには無いHDR後処理パイプライン。シーン描画後の画面加工を一括管理する。
/// エフェクトチェーン:
/// HDRシーン → [RTGI] → [SSAO] → [RT反射/SSR] → [ゴッドレイ] → [Bloom] → [DoF]
///           → [MotionBlur] → [Outline] → [TAA] → [ColorGrading] → [Tonemap(HDR→LDR)]
///           → [FXAA] → [Vignette+色収差] → バックバッファ
///
/// HDR空間ではping-pong RT (2枚のHDR RT) を交互に使い、入力と出力が重ならないようにしている。
/// LDR空間でも同様に2枚のLDR RTでping-pongする。

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

// Forward declarations
class RTReflections;
class RTGI;

/// トーンマッピング方式。HDRからLDRへの変換アルゴリズムを選ぶ
enum class TonemapMode : uint32_t { Reinhard = 0, ACES = 1, Uncharted2 = 2 };

/// トーンマッピング用定数バッファ
struct TonemapConstants { uint32_t mode; float exposure; float padding[2]; };
/// FXAA (Fast Approximate Anti-Aliasing) 用定数バッファ
struct FXAAConstants    { float rcpFrameX; float rcpFrameY; float qualitySubpix; float edgeThreshold; };
/// ビネット+色収差用定数バッファ
struct VignetteConstants{ float intensity; float radius; float chromaticStrength; float padding; };
/// カラーグレーディング用定数バッファ
struct ColorGradingConstants { float exposure; float contrast; float saturation; float temperature; };

/// @brief シーン描画後の画面加工を一括管理するポストエフェクトパイプライン
///
/// HDR→LDR変換を含む全エフェクトの初期化・実行・設定を担当する。
/// DxLibでは自前で画面加工するしかないが、GXLibではこのクラスに設定するだけで済む。
class PostEffectPipeline
{
public:
    PostEffectPipeline() = default;
    ~PostEffectPipeline() = default;

    /// @brief パイプラインの初期化。全エフェクトのリソースとPSOを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅 (ピクセル)
    /// @param height 画面高さ (ピクセル)
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief シーン描画の開始。HDR RTをクリアして描画先に設定する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス (0 or 1)
    /// @param dsvHandle 深度ステンシルビューのCPUハンドル
    /// @param camera カメラ (TAA有効時にジッターが適用される)
    void BeginScene(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                     D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, Camera3D& camera);

    /// @brief シーン描画の終了。HDR RTをSRV状態に遷移する
    void EndScene();

    /// @brief 全エフェクトを実行してバックバッファに最終画像を出力する
    /// @param backBufferRTV バックバッファのRTVハンドル
    /// @param depthBuffer 深度バッファ (SSAO/SSR/DoF等で使用)
    /// @param camera カメラ情報
    /// @param deltaTime フレーム間秒数 (自動露出の適応速度に使用)
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

    // --- DXR レイトレ反射 (外部所有) ---
    void SetRTReflections(RTReflections* rt) { m_rtReflections = rt; }
    RTReflections* GetRTReflections() const { return m_rtReflections; }

    // --- DXR GI (外部所有) ---
    void SetRTGI(RTGI* gi) { m_rtGI = gi; }
    RTGI* GetRTGI() const { return m_rtGI; }

    // --- Albedo RT (GI用) ---
    RenderTarget& GetAlbedoRT() { return m_albedoRT; }

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
    RenderTarget& GetNormalRT() { return m_normalRT; }
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// DXR用にコマンドリスト4を設定
    void SetCommandList4(ID3D12GraphicsCommandList4* cmdList4) { m_cmdList4 = cmdList4; }

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
    ID3D12GraphicsCommandList4* m_cmdList4 = nullptr;
    uint32_t                   m_frameIndex = 0;
    uint32_t                   m_width  = 0;
    uint32_t                   m_height = 0;

    // HDR用RT (高ダイナミックレンジ)
    RenderTarget m_hdrRT;
    RenderTarget m_hdrPingPongRT;

    // GBuffer法線RT (DXR反射用)
    RenderTarget m_normalRT;

    // GBufferアルベドRT (GI用)
    RenderTarget m_albedoRT;

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

    // DXRレイトレ反射 (外部所有、nullptrの場合SSRのみ)
    RTReflections* m_rtReflections = nullptr;

    // DXR GI (外部所有)
    RTGI* m_rtGI = nullptr;

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
