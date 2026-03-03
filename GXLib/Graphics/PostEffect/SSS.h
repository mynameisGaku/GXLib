#pragma once
/// @file SSS.h
/// @brief Screen-Space Subsurface Scattering (SSSSS)
///
/// Jimenez et al. "Separable Subsurface Scattering" の実装。
/// スキン、大理石、蝋、牛乳などの半透明マテリアルのサブサーフェスライトトランスポートを
/// スクリーン空間で近似する。セパラブルカーネルによる2パス（水平+垂直）ブラー。
///
/// 処理の流れ:
/// 1. CPU側でRGBチャンネル別のガウシアンカーネルを事前計算 (falloff/strength3パラメータに基づく)
/// 2. 水平パス: srcHDR → m_tempRT (direction=(1,0))
/// 3. 垂直パス: m_tempRT → destHDR (direction=(0,1))
/// 両パスとも深度バッファを参照し、深度差に応じてブラー幅をピクセル毎に変調する。
/// SSSマスク（sssStencilRT の R チャンネル）で SSS 対象ピクセルのみ処理する。

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Vector3.h"

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

class GraphicsDevice;

/// @brief SSS設定パラメータ
struct SSSSettings
{
    bool  enabled       = false;
    float strength      = 1.0f;                        ///< SSS効果の全体強度
    float width         = 0.012f;                      ///< カーネル幅（スクリーン空間）
    Vector3 falloff     = {1.0f, 0.37f, 0.3f};        ///< RGB別減衰（スキン用デフォルト）
    Vector3 strength3   = {0.48f, 0.41f, 0.28f};      ///< RGB別強度
    int   numSamples    = 25;                           ///< カーネルサンプル数（最大25）
    float translucency  = 0.0f;                        ///< 半透明度（バックライト透過）
};

/// @brief スクリーン空間サブサーフェススキャッタリング
///
/// セパラブルカーネルによる2パスブラーでSSSを近似する。
/// HDRシーン + 深度バッファ + SSSマスクRTの3テクスチャを入力とし、
/// RGB別に異なるガウシアンブラーを適用して半透明マテリアルの透過光を再現する。
class SSS
{
public:
    SSS() = default;
    ~SSS() = default;

    /// @brief 初期化。RS, PSO, CB, SRVヒープ, 一時RTを作成する
    /// @param device グラフィクスデバイス（nullptr時はCPUのみモード）
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(GraphicsDevice* device, uint32_t width, uint32_t height);

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief セパラブルSSS実行（水平+垂直の2パス）
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depth 深度バッファ（ブラー幅変調に使用）
    /// @param sssStencilRT SSSマスクRT（Rチャンネルが0.5以上でSSS対象）
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, RenderTarget& sssStencilRT);

    void SetSettings(const SSSSettings& s) { m_settings = s; ComputeKernel(); }
    const SSSSettings& GetSettings() const { return m_settings; }
    void SetEnabled(bool e) { m_settings.enabled = e; }
    bool IsEnabled() const { return m_settings.enabled; }
    void SetStrength(float s) { m_settings.strength = s; }
    float GetStrength() const { return m_settings.strength; }

    /// @brief PSO及びリソースが使用可能かどうか
    bool IsReady() const { return m_settings.enabled && m_pso != nullptr; }

    /// @brief カーネル幅を設定する
    void SetWidth(float w) { m_settings.width = w; }
    float GetWidth() const { return m_settings.width; }

    /// @brief RGB別減衰を設定する
    void SetFalloff(const Vector3& f) { m_settings.falloff = f; ComputeKernel(); }
    const Vector3& GetFalloff() const { return m_settings.falloff; }

    /// @brief RGB別強度を設定する
    void SetStrength3(const Vector3& s) { m_settings.strength3 = s; ComputeKernel(); }
    const Vector3& GetStrength3() const { return m_settings.strength3; }

    /// @brief サンプル数を設定する（1〜25）
    void SetNumSamples(int n) { m_settings.numSamples = n; ComputeKernel(); }
    int GetNumSamples() const { return m_settings.numSamples; }

    /// @brief カーネルサンプル数の最大値
    static constexpr int k_MaxSamples = 25;

private:
    bool CreatePipelines(ID3D12Device* device);
    void ComputeKernel();

    /// @brief ガウシアンプロファイル（RGB別σでサンプリング）
    static Vector3 GaussianProfile(float variance, const Vector3& falloff);

    SSSSettings m_settings;
    uint32_t m_width = 0, m_height = 0;

    // Separable blur kernel (precomputed on CPU)
    struct KernelSample
    {
        float offset;     ///< サンプルオフセット（-1〜1の正規化範囲）
        Vector3 weight;   ///< RGB別の重み
    };
    KernelSample m_kernel[k_MaxSamples] = {};
    int m_kernelSize = 0;

    // GPU resources
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;
    DescriptorHeap m_srvHeap;   ///< 3テクスチャ × 2フレーム × 2パス = 12スロット
    RenderTarget m_tempRT;       ///< 水平パスの中間RT
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& scene, DepthBuffer& depth,
                       RenderTarget& stencilRT, uint32_t base);
};

/// @}
} // namespace gx
