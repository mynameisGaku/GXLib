/// @file MotionBlur.hlsl
/// @brief カメラベース Motion Blur シェーダー
///
/// 深度バッファからワールド座標を再構成し、前フレームVP行列で
/// スクリーン座標に再投影して速度ベクトルを求める。
/// 現在位置から前フレーム位置に向かってHDRシーンをブラーする。

#include "Fullscreen.hlsli"

cbuffer MotionBlurCB : register(b0)
{
    float4x4 invViewProjection;       // 現フレームの逆VP行列（ワールド座標復元用）
    float4x4 previousViewProjection;  // 前フレームのVP行列（リプロジェクション用）
    float intensity;                  // ブラーの強度
    int   sampleCount;                // ブラーのサンプル数
    float2 padding;
};

Texture2D<float4> gScene : register(t0);
Texture2D<float>  gDepth : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

/// @brief モーションブラーPS — リプロジェクションで速度ベクトルを求め、方向にブラー
float4 PSMotionBlur(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = gDepth.Sample(gPointSampler, uv).r;

    // スカイボックス（depth>=1.0）はブラーしない
    if (depth >= 1.0)
        return gScene.Sample(gLinearSampler, uv);

    // UV → NDC (clip space)
    float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);

    // ワールド座標再構成 (行ベクトル × 行列: DirectXMath 規約)
    float4 worldPos = mul(clipPos, invViewProjection);
    worldPos /= worldPos.w;

    // 前フレームのスクリーン座標に再投影
    float4 prevClip = mul(worldPos, previousViewProjection);
    float2 prevNDC = prevClip.xy / prevClip.w;
    float2 prevUV = prevNDC * float2(0.5, -0.5) + 0.5;

    // 速度ベクトル（current → previous 方向）
    float2 velocity = (uv - prevUV) * intensity;

    // 速度の大きさをチェック
    float velocityLen = length(velocity);
    if (velocityLen < 0.001)
        return gScene.Sample(gLinearSampler, uv);

    // 最大速度をクランプ（画面の10%以上のブラーを防止）
    float maxVelocity = 0.1;
    if (velocityLen > maxVelocity)
        velocity = velocity / velocityLen * maxVelocity;

    // 現在位置から前フレーム位置に向かってブラー
    // t=0 で現在位置、t=1 で前フレーム位置
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
