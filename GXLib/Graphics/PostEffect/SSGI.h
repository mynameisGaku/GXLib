#pragma once
/// @file SSGI.h
/// @brief Screen-Space Global Illumination (SSGI)
///
/// スクリーン空間でグローバルイルミネーション（間接照明）を近似する。
/// GBufferの深度・法線・カラーからレイマーチングで周囲のバウンスライトを計算し、
/// 間接照明として加算する。SSAOの上位互換としてカラー情報を含むGIを実現。
/// @addtogroup grp_gfx_postfx/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"

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

/// @brief スクリーン空間グローバルイルミネーション
///
/// GBuffer（深度・法線・カラー）からレイマーチングを行い、
/// 周囲のサーフェスからのバウンスライト（間接照明）を計算する。
/// コンピュートシェーダーベースでフルスクリーンに適用し、
/// ポストエフェクトパイプラインでシーンに合成する。
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

    /// @brief SSGI を実行する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depthSRV 深度バッファ
    /// @param normalSRV 法線バッファ
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 RenderTarget& depthRT, RenderTarget& normalRT);

    /// @brief 有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief 有効/無効を取得する
    bool IsEnabled() const { return m_enabled; }

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

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_intensity = 0.5f;
    int m_sampleCount = 32;
    float m_maxDistance = 50.0f;
    float m_thickness = 0.5f;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 3テクスチャ(scene + depth + normal)用専用SRVヒープ (3スロット x 2フレーム = 6)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& srcHDR, RenderTarget& depthRT,
                       RenderTarget& normalRT, uint32_t frameIndex);
};

} // namespace gx
/// @}
