/// @file MotionBlur.hlsl
/// @brief カメラベース Motion Blur シェーダー
///
/// 深度バッファからワールド座標を再構成し、前フレームVP行列で
/// スクリーン座標に再投影して速度ベクトルを求める。
/// 速度方向に沿ってHDRシーンを線形サンプリングしてブラーする。

#include "Fullscreen.hlsli"

cbuffer MotionBlurCB : register(b0)
{
    float4x4 invViewProjection;
    float4x4 previousViewProjection;
    float intensity;
    int   sampleCount;
    float2 padding;
};

Texture2D<float4> gScene : register(t0);
Texture2D<float>  gDepth : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

float4 PSMotionBlur(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = gDepth.Sample(gPointSampler, uv).r;

    // スカイボックス（depth=1.0）はブラーしない
    if (depth >= 1.0)
        return gScene.Sample(gLinearSampler, uv);

    // UV → NDC (clip space)
    float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);

    // ワールド座標再構成
    float4 worldPos = mul(invViewProjection, clipPos);
    worldPos /= worldPos.w;

    // 前フレームのスクリーン座標に再投影
    float4 prevClip = mul(previousViewProjection, worldPos);
    float2 prevNDC = prevClip.xy / prevClip.w;
    float2 prevUV = prevNDC * float2(0.5, -0.5) + 0.5;

    // 速度ベクトル
    float2 velocity = (uv - prevUV) * intensity;

    // 速度が非常に小さい場合はブラーをスキップ
    float velocityLen = length(velocity);
    if (velocityLen < 0.0001)
        return gScene.Sample(gLinearSampler, uv);

    // 速度方向にブラー（中心を基準に前後にサンプル）
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < sampleCount; i++)
    {
        float t = (float)i / (float)(sampleCount - 1) - 0.5;
        float2 sampleUV = uv + velocity * t;
        // UV範囲クランプ
        sampleUV = saturate(sampleUV);
        color += gScene.Sample(gLinearSampler, sampleUV);
    }
    color /= (float)sampleCount;

    return color;
}
