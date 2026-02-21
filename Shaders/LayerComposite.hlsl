/// @file LayerComposite.hlsl
/// @brief レイヤー合成シェーダー
///
/// フルスクリーン三角形でレイヤーをバックバッファに合成する。
/// マスクあり/なしの2種類のピクセルシェーダーを提供。

#include "Fullscreen.hlsli"

cbuffer CompositeConstants : register(b0)
{
    float gOpacity;   // レイヤーの透明度
    float gHasMask;   // マスク有無フラグ (PSO分岐用、シェーダー内では未使用)
    float2 gPadding;
};

Texture2D tLayer : register(t0);
Texture2D tMask  : register(t1);
SamplerState sLinear : register(s0);

/// @brief マスクなしレイヤー合成 — アルファにOpacityを乗算して出力
float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float4 color = tLayer.Sample(sLinear, input.uv);
    color.a *= gOpacity;
    return color;
}

/// @brief マスクあり合成 — Opacity * マスク値でアルファを制御
float4 PSCompositeMasked(FullscreenVSOutput input) : SV_Target
{
    float4 color = tLayer.Sample(sLinear, input.uv);
    float  mask  = tMask.Sample(sLinear, input.uv).r;
    color.a *= gOpacity * mask;
    return color;
}
