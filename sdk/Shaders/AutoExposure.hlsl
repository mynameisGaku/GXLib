/// @file AutoExposure.hlsl
/// @brief 自動露出用シェーダー
///
/// PSLogLuminance: HDR scene → log(luminance) R16_FLOAT
/// PSDownsample: bilinear average downsample

#include "Fullscreen.hlsli"

cbuffer AutoExposureCB : register(b0)
{
    float4 params; // Reserved: C++側CBレイアウト維持のため削除不可
};

Texture2D<float4> gInput : register(t0);
SamplerState gLinearSampler : register(s0);

/// @brief 対数輝度変換 — HDRシーンのピクセル輝度をlog空間に変換して出力
float PSLogLuminance(FullscreenVSOutput input) : SV_Target
{
    float4 color = gInput.Sample(gLinearSampler, input.uv);

    // 輝度計算 (Rec. 709 luminance weights)
    float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    // log(luminance + epsilon) で暗い領域のlog(0)を防止
    return log(max(luminance, 0.0001));
}

/// @brief バイリニアダウンサンプル — バイリニアフィルタで4テクセル平均を取得
/// RTサイズを段階的に縮小して最終的に1x1にし、シーン平均輝度を求める。
float PSDownsample(FullscreenVSOutput input) : SV_Target
{
    return gInput.Sample(gLinearSampler, input.uv).r;
}
