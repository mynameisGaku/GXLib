#pragma once
/// @file Bloom.h
/// @brief Bloomポストエフェクト
///
/// 【初学者向け解説】
/// Bloomとは、明るい部分が光り輝いているように見せるエフェクトです。
/// 現実のカメラで強い光を撮影すると周囲に光が滲むのと同じ現象を再現します。
///
/// 処理の流れ：
/// 1. 閾値抽出: HDRシーンから明るい部分だけを取り出す（→ mip[0]）
/// 2. ダウンサンプル: 段階的に解像度を下げる（1/2→1/4→1/8→1/16→1/32）
/// 3. Gaussianブラー: 各レベルで水平→垂直ブラー
/// 4. アップサンプル合成: 小さいレベルから順に大きいレベルに加算
/// 5. 最終合成: HDRシーンにブルーム結果をアディティブブレンドで描画

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// Bloom定数バッファ
struct BloomConstants
{
    float    threshold;
    float    intensity;
    float    texelSizeX;
    float    texelSizeY;
};

/// @brief Bloomエフェクト
class Bloom
{
public:
    static constexpr uint32_t k_MaxMipLevels = 5;

    Bloom() = default;
    ~Bloom() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// Bloom実行
    /// hdrRT: HDRシーン（SRV状態で渡す）
    /// destRT: 出力先（RTV状態にしてhdrRTの内容+Bloomを書き込む）
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& hdrRT, RenderTarget& destRT);

    void SetThreshold(float threshold) { m_threshold = threshold; }
    float GetThreshold() const { return m_threshold; }

    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

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
