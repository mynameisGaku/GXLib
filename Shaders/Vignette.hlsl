/// @file Vignette.hlsl
/// @brief ビネット＋色収差エフェクト
///
/// ビネット: 画面端に向かって暗くなる効果（カメラレンズの特性を模倣）
/// 色収差: R/G/Bチャンネルを異なるUVオフセットでサンプリング（レンズの分散を模倣）

#include "Fullscreen.hlsli"

cbuffer VignetteConstants : register(b0)
{
    float gVignetteIntensity;   // ビネットの強さ (0-1, default: 0.5)
    float gVignetteRadius;      // ビネット半径 (default: 0.8)
    float gChromaticStrength;   // 色収差の強さ (default: 0.003)
    float gPadding;
};

Texture2D    tScene  : register(t0);
SamplerState sLinear : register(s0);

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;

    // 色収差: 画面中心からの距離に応じてR/G/Bを異なるUVでサンプル
    float2 center = float2(0.5f, 0.5f);
    float2 dir = uv - center;
    float dist = length(dir);

    float2 uvR = uv + dir * gChromaticStrength;
    float2 uvB = uv - dir * gChromaticStrength;

    float3 color;
    color.r = tScene.Sample(sLinear, uvR).r;
    color.g = tScene.Sample(sLinear, uv).g;
    color.b = tScene.Sample(sLinear, uvB).b;

    // ビネット: 距離ベースの周辺減光
    float vignette = 1.0f - smoothstep(gVignetteRadius, gVignetteRadius + 0.4f, dist * 1.414f);
    vignette = lerp(1.0f, vignette, gVignetteIntensity);
    color *= vignette;

    return float4(color, 1.0f);
}
