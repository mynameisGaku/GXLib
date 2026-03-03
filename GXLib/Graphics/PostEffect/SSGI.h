#pragma once
/// @file SSGI.h
/// @brief Screen-Space Global Illumination (SSGI)
///
/// スクリーン空間でグローバルイルミネーション（間接照明）を近似する。
/// GBufferの深度・法線・カラーからレイマーチングで周囲のバウンスライトを計算し、
/// 間接照明として加算する。SSAOの上位互換としてカラー情報を含むGIを実現。
/// テンポラルリプロジェクションによりフレーム間の蓄積でノイズを低減する。
/// @addtogroup grp_gfx_postfx/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Matrix4x4.h"

namespace gx
{

class GraphicsDevice;

/// SSGI 定数バッファ (GPU送信用)
struct SSGIConstants
{
    float intensity;        ///< GIの強度
    int   sampleCount;      ///< レイサンプル数
    float maxDistance;       ///< レイの最大到達距離
    float thickness;        ///< ヒット判定の深度厚み閾値
    float screenWidth;      ///< スクリーン幅
    float screenHeight;     ///< スクリーン高さ
    float padding[2];       ///< 32B アライメント用
};

/// SSGI テンポラルリゾルブ定数バッファ (GPU送信用)
struct SSGITemporalConstants
{
    Matrix4x4 prevViewProj;  ///< 前フレームのVP行列 (row-major transposed)
    Matrix4x4 invViewProj;   ///< 現フレームの逆VP行列 (row-major transposed)
    float alpha;             ///< 新フレーム混合比率 (0.01~1.0)
    float screenWidth;       ///< スクリーン幅
    float screenHeight;      ///< スクリーン高さ
    float hasHistory;        ///< ヒストリが存在するか (0.0 or 1.0)
};  // 160B -> 256-align

/// @brief スクリーン空間グローバルイルミネーション
///
/// GBuffer（深度・法線・カラー）からレイマーチングを行い、
/// 周囲のサーフェスからのバウンスライト（間接照明）を計算する。
/// コンピュートシェーダーベースでフルスクリーンに適用し、
/// ポストエフェクトパイプラインでシーンに合成する。
/// テンポラルリプロジェクションで前フレームの結果と混合し、
/// ノイズを大幅に低減する。
class SSGI
{
public:
    SSGI() = default;
    ~SSGI() = default;

    /// @brief 初期化。PSO・SRVヒープ・定数バッファを作成する
    /// @param device D3D12デバイス（nullptrの場合はfalseを返す）
    /// @param width スクリーン幅
    /// @param height スクリーン高さ
    /// @return 成功なら true
    bool Initialize(GraphicsDevice* device, uint32_t width, uint32_t height);

    /// @brief 画面リサイズ対応
    void OnResize(GraphicsDevice* device, uint32_t width, uint32_t height);

    /// @brief 画面リサイズ対応 (ID3D12Device* 版)
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief SSGI を実行する (RenderTarget 深度版)
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depthRT 深度バッファ (RenderTarget)
    /// @param normalRT 法線バッファ
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 RenderTarget& depthRT, RenderTarget& normalRT);

    /// @brief SSGI を実行する (DepthBuffer 版)
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depth 深度バッファ (DepthBuffer)
    /// @param normalRT 法線バッファ
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, RenderTarget& normalRT);

    /// @brief 有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief 有効/無効を取得する
    bool IsEnabled() const { return m_enabled; }

    /// @brief 初期化済みかつ実行可能か
    bool IsReady() const { return m_enabled && m_pso != nullptr; }

    /// @brief GI強度を設定する
    /// @param intensity 0.0~2.0 にクランプ
    void SetIntensity(float intensity);

    /// @brief GI強度を取得する
    float GetIntensity() const { return m_intensity; }

    /// @brief レイサンプル数を設定する
    /// @param count 4~128 にクランプ
    void SetSampleCount(int count);

    /// @brief レイサンプル数を取得する
    int GetSampleCount() const { return m_sampleCount; }

    /// @brief レイの最大到達距離を設定する
    /// @param dist 1.0~200.0 にクランプ
    void SetMaxDistance(float dist);

    /// @brief レイの最大到達距離を取得する
    float GetMaxDistance() const { return m_maxDistance; }

    /// @brief 深度厚み閾値を設定する
    void SetThickness(float t) { m_thickness = (t > 0.0f) ? t : 0.01f; }

    /// @brief 深度厚み閾値を取得する
    float GetThickness() const { return m_thickness; }

    /// @brief テンポラル蓄積の新フレーム混合比率を設定する
    /// @param a 0.01~1.0 にクランプ（小さいほど蓄積が強い）
    void SetTemporalAlpha(float a) { m_temporalAlpha = std::clamp(a, 0.01f, 1.0f); }

    /// @brief テンポラル蓄積の新フレーム混合比率を取得する
    float GetTemporalAlpha() const { return m_temporalAlpha; }

    /// @brief 前フレームのビュープロジェクション行列を設定する
    /// @param vp 前フレームVP行列
    void SetPrevViewProj(const Matrix4x4& vp) { m_prevViewProj = vp; }

    /// @brief 現フレームの逆VP行列を設定する
    /// @param invVP 現フレーム逆VP行列
    void SetInvViewProj(const Matrix4x4& invVP) { m_invViewProj = invVP; }

private:
    bool CreatePipelines(ID3D12Device* device);
    bool CreateTemporalPipeline(ID3D12Device* device);

    bool m_enabled = false;
    float m_intensity = 0.5f;
    int m_sampleCount = 32;
    float m_maxDistance = 50.0f;
    float m_thickness = 0.5f;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // SSGIメインパイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 3テクスチャ(scene + depth + normal)用専用SRVヒープ (3スロット x 2フレーム = 6)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& srcHDR, RenderTarget& depthRT,
                       RenderTarget& normalRT, uint32_t frameIndex);
    void UpdateSRVHeapFromDepthBuffer(RenderTarget& srcHDR, DepthBuffer& depth,
                                       RenderTarget& normalRT, uint32_t frameIndex);

    // SSGI中間結果 (テンポラル合成前)
    RenderTarget m_intermediateRT;

    // テンポラル蓄積
    RenderTarget m_historyRT[2];              ///< テンポラル蓄積ヒストリ (ダブルバッファ)
    uint32_t m_historyWriteIdx = 0;           ///< ヒストリ書き込みインデックス

    // テンポラルリゾルブパイプライン
    ComPtr<ID3D12RootSignature> m_temporalRS;
    ComPtr<ID3D12PipelineState> m_temporalPSO;
    DynamicBuffer m_temporalCB;
    DescriptorHeap m_temporalHeap;            ///< 4 slots x 2 frames = 8 slots

    float m_temporalAlpha = 0.1f;             ///< 新フレーム混合比率
    Matrix4x4 m_prevViewProj = {};            ///< 前フレームVP行列
    Matrix4x4 m_invViewProj = {};             ///< 現フレーム逆VP行列
    bool m_hasHistory = false;                ///< ヒストリが存在するか

    void UpdateTemporalSRVHeap(RenderTarget& currentGI, RenderTarget& depthRT,
                                uint32_t frameIndex);
    void UpdateTemporalSRVHeapFromDepthBuffer(RenderTarget& currentGI, DepthBuffer& depth,
                                               uint32_t frameIndex);
    void ExecuteTemporalResolve(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                 RenderTarget& destHDR, RenderTarget& depthRT);
    void ExecuteTemporalResolveDepthBuffer(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                            RenderTarget& destHDR, DepthBuffer& depth);
};

} // namespace gx
/// @}
