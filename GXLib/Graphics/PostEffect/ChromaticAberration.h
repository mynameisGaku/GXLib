#pragma once
/// @file ChromaticAberration.h
/// @brief Chromatic Aberration (色収差) ポストエフェクト
///
/// レンズの色収差を再現するエフェクト。R/G/B チャンネルをそれぞれ異なるオフセットで
/// サンプリングすることで、画面周辺部に虹色のフリンジ（色ずれ）を発生させる。
/// 放射状の減衰 (falloff) により、画面中央では収差が小さく周辺で大きくなる。
///
/// intensity が大きいほど色ずれが目立ち、falloff が大きいほど中央→周辺の
/// 変化が急峻になる。
/// @addtogroup grp_gfx_postfx/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{

/// Chromatic Aberration 定数バッファ (分離量・中心座標・減衰)
struct ChromaticAberrationConstants
{
    float intensity;   ///< RGB分離量 (ピクセル比、0.005 = 微弱)
    float centerX;     ///< 放射中心 X [0, 1] (画面UV座標)
    float centerY;     ///< 放射中心 Y [0, 1]
    float falloff;     ///< 放射減衰指数 (大きいほど中央付近で減衰が強い)
};

/// @brief レンズ色収差を再現するポストエフェクト
///
/// R/G/B チャンネルを放射方向に異なるオフセットでサンプリングし、
/// 画面周辺部に色フリンジを生成する。
class ChromaticAberration
{
public:
    ChromaticAberration() = default;
    ~ChromaticAberration() = default;

    /// @brief 初期化。PSO・定数バッファを作成する
    /// @param device D3D12デバイス (nullptrの場合はCPUのみ初期化)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 色収差エフェクトを適用する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param sceneRT 入力シーン (SRV状態)
    /// @param destRT 出力先。色収差が適用されたシーンが書き込まれる
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& sceneRT, RenderTarget& destRT);

    /// @brief RGB分離量を設定する
    void SetIntensity(float v) { m_intensity = v; }
    float GetIntensity() const { return m_intensity; }

    /// @brief 放射中心を設定する (UV座標、画面中央は 0.5, 0.5)
    void SetCenter(float x, float y) { m_centerX = x; m_centerY = y; }
    float GetCenterX() const { return m_centerX; }
    float GetCenterY() const { return m_centerY; }

    /// @brief 放射減衰指数を設定する
    void SetFalloff(float v) { m_falloff = v; }
    float GetFalloff() const { return m_falloff; }

    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief 画面リサイズ時の再初期化
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

private:
    bool CreatePipelines(ID3D12Device* device);

    ID3D12Device* m_device = nullptr;

    float m_intensity = 0.005f;
    float m_centerX   = 0.5f;
    float m_centerY   = 0.5f;
    float m_falloff   = 2.0f;
    bool  m_enabled   = false;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader                      m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer               m_constantBuffer;
};

} // namespace gx
/// @}
