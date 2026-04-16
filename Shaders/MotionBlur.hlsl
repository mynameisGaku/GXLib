/// @file MotionBlur.hlsl
/// @brief カメラベース Motion Blur シェーダー (深度再投影方式)
///
/// 深度バッファからワールド座標を復元し、前フレームVP行列で再投影して
/// スクリーン空間速度を求め、その方向にHDRシーンをブラーする。

#include "Fullscreen.hlsli"

// C++ MotionBlurConstants と完全一致するレイアウト (144B)
cbuffer MotionBlurCB : register(b0)
{
    float4x4 invViewProjection;       // offset 0:   現フレーム逆VP行列（転置済み）
    float4x4 previousViewProjection;  // offset 64:  前フレームVP行列（転置済み）
    float  intensity;                 // offset 128: ブラー強度
    int    sampleCount;               // offset 132: サンプリング数
    float2 padding;                   // offset 136: パディング
};

Texture2D<float4> gScene : register(t0);
Texture2D<float>  gDepth : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

/// @brief モーションブラーPS — 深度再投影で速度を求め、方向にブラー
float4 PSMotionBlur(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;

    // 1. 深度読み取り
    float depth = gDepth.Sample(gPointSampler, uv);

    // スカイ (depth == 0 or 1) はブラーしない
    if (depth <= 0.0 || depth >= 1.0)
        return gScene.Sample(gLinearSampler, uv);

    // 2. UV + depth → NDC → ワールド座標復元
    //    UV [0,1] → NDC [-1,1] (Y反転)
    float2 ndc = float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 worldPos = mul(clipPos, invViewProjection);
    worldPos /= worldPos.w;

    // 3. ワールド座標 → 前フレームNDC
    float4 prevClip = mul(worldPos, previousViewProjection);
    float2 prevNDC = prevClip.xy / prevClip.w;

    // 4. 速度ベクトル (screen-space)
    float2 velocity = (ndc - prevNDC) * 0.5 * intensity;

    // 速度の大きさをチェック (動いていなければブラー不要)
    float velocityLen = length(velocity);
    if (velocityLen < 0.001)
        return gScene.Sample(gLinearSampler, uv);

    // 最大速度をクランプ（画面の10%以上のブラーを防止）
    float maxVelocity = 0.1;
    if (velocityLen > maxVelocity)
        velocity = velocity / velocityLen * maxVelocity;

    // 5. 速度方向にサンプリングしてブラー
    int count = clamp(sampleCount, 2, 64);
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < count; i++)
    {
        float t = (float)i / (float)(count - 1);
        float2 sampleUV = saturate(uv - velocity * t);
        color += gScene.Sample(gLinearSampler, sampleUV);
    }
    color /= (float)count;

    return color;
}
