/// @file LayerComposite.hlsl
/// @brief レイヤー合成シェーダー
///
/// フルスクリーン三角形でレイヤーをバックバッファに合成する。
/// マスクあり/なしの2種類のピクセルシェーダーを提供。

#include "Fullscreen.hlsli"

cbuffer CompositeConstants : register(b0)
{
    float gOpacity;
    float gHasMask; // PSO分岐のためシェーダー内では未使用（CBレイアウト維持用）
    float2 gPadding;
};

Texture2D tLayer : register(t0);
Texture2D tMask  : register(t1);
SamplerState sLinear : register(s0);

// マスクなし合成
float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float4 color = tLayer.Sample(sLinear, input.uv);
    color.a *= gOpacity;
    return color;
}

// マスクあり合成
float4 PSCompositeMasked(FullscreenVSOutput input) : SV_Target
{
    float4 color = tLayer.Sample(sLinear, input.uv);
    float  mask  = tMask.Sample(sLinear, input.uv).r;
    color.a *= gOpacity * mask;
    return color;
}
