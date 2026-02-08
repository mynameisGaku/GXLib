/// @file ColorGrading.hlsl
/// @brief カラーグレーディング（色調補正）
///
/// 露出・コントラスト・彩度・色温度を調整する。
/// トーンマッピングの前（HDR空間）または後（LDR空間）に適用可能。

#include "Fullscreen.hlsli"

cbuffer ColorGradingConstants : register(b0)
{
    float gCGExposure;     // 追加露出補正 (default: 0, range: -2 to 2)
    float gContrast;       // コントラスト (default: 1.0, range: 0.5 to 2.0)
    float gSaturation;     // 彩度 (default: 1.0, range: 0 to 2.0)
    float gTemperature;    // 色温度 (default: 0, range: -1 to 1, negative=cool, positive=warm)
};

Texture2D    tScene  : register(t0);
SamplerState sLinear : register(s0);

float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float3 color = tScene.Sample(sLinear, input.uv).rgb;

    // 露出補正
    color *= exp2(gCGExposure);

    // コントラスト (0.5を基準にスケーリング)
    color = (color - 0.5f) * gContrast + 0.5f;
    color = max(color, 0.0f);

    // 彩度
    float lum = Luminance(color);
    color = lerp(float3(lum, lum, lum), color, gSaturation);

    // 色温度 (簡易版: R/Bチャンネルのバランスを調整)
    color.r *= 1.0f + gTemperature * 0.1f;
    color.b *= 1.0f - gTemperature * 0.1f;

    return float4(max(color, 0.0f), 1.0f);
}
