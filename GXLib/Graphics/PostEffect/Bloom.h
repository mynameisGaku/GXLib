#pragma once
/// @file Bloom.h
/// @brief Bloomポストエフェクト (光の滲み)
///
/// DxLibには無い機能。明るい部分から光が溢れ出すような表現を追加する。
/// 閾値以上の明部を抽出し、段階的に縮小→ブラー→拡大合成することで
/// 自然な光の広がりを作る。MIPチェーンは5レベル (1/2〜1/32解像度)。
///
/// 処理の流れ:
/// 1. 閾値抽出 (HDR → mip[0]) → 2. ダウンサンプル (1/2→1/4→...→1/32)
/// 3. Gaussianブラー (各レベルで水平→垂直) → 4. アップサンプル加算合成 (小→大)
/// 5. 最終合成 (HDRシーン + Bloom結果をアディティブブレンド)

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// Bloom定数バッファ (閾値・強度・テクセルサイズ)
struct BloomConstants
{
    float    threshold;    ///< この輝度以上のピクセルだけ抽出
    float    intensity;    ///< 最終合成時の明るさ倍率
    float    texelSizeX;   ///< 1.0 / テクスチャ幅
    float    texelSizeY;   ///< 1.0 / テクスチャ高さ
};

/// @brief 明部の光の滲みを再現するBloomエフェクト
///
/// HDRシーンの明るい部分を抽出し、MIPチェーンを使ったダウンサンプル+ブラー+
/// アップサンプルで自然な光の広がりを作り、最終的にシーンにアディティブ合成する。
class Bloom
{
public:
    /// MIPレベル数。5段階で1/2〜1/32解像度まで縮小する
    static constexpr uint32_t k_MaxMipLevels = 5;

    Bloom() = default;
    ~Bloom() = default;

    /// @brief 初期化。MIP RT・PSO・定数バッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief Bloomの全パスを実行する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param hdrRT 入力HDRシーン (SRV状態)
    /// @param destRT 出力先。hdrRTの内容にBloom結果が加算されて書き込まれる
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& hdrRT, RenderTarget& destRT);

    /// @brief 閾値を設定。この輝度以上のピクセルだけBloomの対象になる
    void SetThreshold(float threshold) { m_threshold = threshold; }
    float GetThreshold() const { return m_threshold; }

    /// @brief Bloom合成時の強度。大きいほど光が強く滲む
    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief 画面リサイズ時にMIP RTを再生成する
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

private:
    bool CreatePipelines(ID3D12Device* device);
    void CreateMipRenderTargets(ID3D12Device* device, uint32_t width, uint32_t height);

    void UpdateCB(ID3D12GraphicsCommandList* cmdList, float texelX, float texelY);
    void DrawFullscreen(ID3D12GraphicsCommandList* cmdList,
                        ID3D12PipelineState* pso,
                        RenderTarget& dest, RenderTarget& src,
                        float texelX, float texelY);

    ID3D12Device* m_device = nullptr;

    float m_threshold = 1.0f;
    float m_intensity = 0.5f;
    bool  m_enabled   = true;

    // MIPレベルRT
    RenderTarget m_mipRT[k_MaxMipLevels];
    RenderTarget m_blurTempRT[k_MaxMipLevels];  // 水平ブラー中間
    uint32_t     m_mipWidths[k_MaxMipLevels]  = {};
    uint32_t     m_mipHeights[k_MaxMipLevels] = {};

    // パイプライン
    Shader                       m_shader;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_thresholdPSO;
    ComPtr<ID3D12PipelineState>  m_downsamplePSO;
    ComPtr<ID3D12PipelineState>  m_blurHPSO;
    ComPtr<ID3D12PipelineState>  m_blurVPSO;
    ComPtr<ID3D12PipelineState>  m_additivePSO;     // アディティブブレンド描画用
    ComPtr<ID3D12PipelineState>  m_copyPSO;         // 同サイズコピー用
    DynamicBuffer                m_constantBuffer;
};

} // namespace GX
