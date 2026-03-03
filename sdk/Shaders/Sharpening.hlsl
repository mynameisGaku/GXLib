/// @file Sharpening.hlsl
/// @brief CAS (Contrast Adaptive Sharpening) シャープニングフィルタ
///
/// TAA後のボケを回復するためのシャープニングパス。
/// AMD FidelityFX CAS に基づくコントラスト適応型シャープニング。
/// テクスチャのエッジを検出し、コントラストが低い箇所ほど強くシャープ化する。

#include "Fullscreen.hlsli"

cbuffer SharpeningConstants : register(b0)
{
    float gSharpness;     // シャープニング強度 (0=無効, 1=最大)
    float gScreenWidth;   // スクリーン幅
    float gScreenHeight;  // スクリーン高さ
    float gPadding;
};

Texture2D    tSource : register(t0);
SamplerState sPoint  : register(s0);

/// @brief CAS (Contrast Adaptive Sharpening) ピクセルシェーダー
/// 5タップ十字型フィルタでコントラスト適応型シャープニングを行う
float4 PSSharpening(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 texelSize = float2(1.0 / gScreenWidth, 1.0 / gScreenHeight);

    // 5タップ十字型サンプリング
    float3 center = tSource.Sample(sPoint, uv).rgb;
    float3 north  = tSource.Sample(sPoint, uv + float2(0, -texelSize.y)).rgb;
    float3 south  = tSource.Sample(sPoint, uv + float2(0,  texelSize.y)).rgb;
    float3 west   = tSource.Sample(sPoint, uv + float2(-texelSize.x, 0)).rgb;
    float3 east   = tSource.Sample(sPoint, uv + float2( texelSize.x, 0)).rgb;

    // チャンネルごとにmin/maxを計算
    float3 minColor = min(min(north, south), min(west, east));
    float3 maxColor = max(max(north, south), max(west, east));

    minColor = min(minColor, center);
    maxColor = max(maxColor, center);

    // コントラストに基づくシャープ量を計算
    // コントラストが低い箇所ほど強くシャープ化（高コントラスト箇所はリンギングを避ける）
    float3 rcpRange = 1.0 / (maxColor - minColor + 1e-4);
    float3 amp = saturate(min(minColor, 2.0 - maxColor) * rcpRange);
    amp = sqrt(amp);

    // シャープニングウェイト (-0.125 ~ -0.5 の範囲でgSharpnessに比例)
    float3 w = amp * lerp(-0.125, -0.5, gSharpness);

    // 加重平均でシャープ化
    float3 result = (center + (north + south + west + east) * w) / (1.0 + 4.0 * w);

    // クランプ（リンギング防止）
    result = clamp(result, minColor, maxColor);

    return float4(result, 1.0);
}
