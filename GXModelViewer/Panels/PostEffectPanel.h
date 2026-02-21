#pragma once
/// @file PostEffectPanel.h
/// @brief ポストエフェクトパラメータ制御パネル
///
/// Bloom, SSAO, SSR, TAA, DoF, MotionBlur, Outline, VolumetricLight,
/// ColorGrading, AutoExposure, Tonemapping, FXAA, Vignetteの各エフェクトの
/// ON/OFFとパラメータをスライダーで調整できる。

namespace GX { class PostEffectPipeline; }

/// @brief PostEffectPipelineの全パラメータを編集するパネル
class PostEffectPanel
{
public:
    /// @brief ポストエフェクトパネルを独立ウィンドウとして描画する
    /// @param pipeline パラメータの読み書き先
    void Draw(GX::PostEffectPipeline& pipeline);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    /// @param pipeline パラメータの読み書き先
    void DrawContent(GX::PostEffectPipeline& pipeline);
};
