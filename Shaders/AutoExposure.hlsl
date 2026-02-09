/// @file AutoExposure.hlsl
/// @brief 自動露出用シェーダー
///
/// PSLogLuminance: HDR scene → log(luminance) R16_FLOAT
/// PSDownsample: bilinear average downsample

#include "Fullscreen.hlsli"

cbuffer AutoExposureCB : register(b0)
{
    float4 params; // unused placeholder
};

Texture2D<float4> gInput : register(t0);
SamplerState gLinearSampler : register(s0);

/// HDR → log(luminance)
float PSLogLuminance(FullscreenVSOutput input) : SV_Target
{
    float4 color = gInput.Sample(gLinearSampler, input.uv);

    // 輝度計算 (Rec. 709 luminance weights)
    float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    // log(luminance + epsilon) で暗い領域のlog(0)を防止
    return log(max(luminance, 0.0001));
}

/// バイリニアダウンサンプル (4テクセル平均)
/// 入力RTの中心を4テクセル分サンプリングするだけで
/// バイリニアフィルタが自動的に4テクセル平均を返す
float PSDownsample(FullscreenVSOutput input) : SV_Target
{
    return gInput.Sample(gLinearSampler, input.uv).r;
}
