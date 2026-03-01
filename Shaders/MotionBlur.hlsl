/// @file MotionBlur.hlsl
/// @brief Per-object Motion Blur シェーダー (速度バッファ方式)
///
/// 各オブジェクトが書き込んだ速度バッファ (R16G16_FLOAT) から
/// スクリーン空間速度を読み取り、その方向にHDRシーンをブラーする。

#include "Fullscreen.hlsli"

cbuffer MotionBlurCB : register(b0)
{
    float intensity;                  // ブラーの強度
    int   sampleCount;                // ブラーのサンプル数
    float2 padding;
};

Texture2D<float4> gScene    : register(t0);
Texture2D<float2> gVelocity : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

/// @brief モーションブラーPS — 速度バッファから速度を読み取り、方向にブラー
float4 PSMotionBlur(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 velocity = gVelocity.Sample(gPointSampler, uv) * intensity;

    // 速度の大きさをチェック (動いていなければブラー不要)
    float velocityLen = length(velocity);
    if (velocityLen < 0.001)
        return gScene.Sample(gLinearSampler, uv);

    // 最大速度をクランプ（画面の10%以上のブラーを防止）
    float maxVelocity = 0.1;
    if (velocityLen > maxVelocity)
        velocity = velocity / velocityLen * maxVelocity;

    // 現在位置から速度方向にブラー
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    int count = max(sampleCount, 2);
    for (int i = 0; i < count; i++)
    {
        float t = (float)i / (float)(count - 1);
        float2 sampleUV = saturate(uv - velocity * t);
        color += gScene.Sample(gLinearSampler, sampleUV);
    }
    color /= (float)count;

    return color;
}
