/// @file RTGIDenoise.hlsl
/// @brief GI テンポラル蓄積 + A-Trous 空間フィルタ
///
/// TemporalPS: 半解像度GIをフル解像度にアップスケール + テンポラル蓄積
/// SpatialPS:  A-Trous wavelet フィルタ (エッジ保持デノイズ)

#include "Fullscreen.hlsli"

// ============================================================================
// テンポラル蓄積パス
// ============================================================================
cbuffer TemporalCB : register(b0)
{
    float4x4 prevViewProjection;
    float4x4 invViewProjection;
    float    alpha;
    float    frameCount;
    float    fullWidth;
    float    fullHeight;
};

Texture2D<float4> g_CurrentGI  : register(t0);  // 半解像度 (バイリニアアップスケール)
Texture2D<float4> g_History    : register(t1);  // 前フレーム蓄積結果
Texture2D<float>  g_Depth      : register(t2);
Texture2D<float>  g_PrevDepth  : register(t3);

SamplerState g_LinearClamp : register(s0);
SamplerState g_PointClamp  : register(s1);

float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 worldPos = mul(clipPos, invViewProjection);
    return worldPos.xyz / worldPos.w;
}

float4 TemporalPS(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = g_Depth.SampleLevel(g_PointClamp, uv, 0);

    if (depth >= 0.999)
        return float4(0, 0, 0, 0);

    // 現フレームGI (半解像度バイリニアアップスケール)
    float3 currentGI = g_CurrentGI.SampleLevel(g_LinearClamp, uv, 0).rgb;

    // 前フレームへのリプロジェクション
    float3 worldPos = ReconstructWorldPosition(uv, depth);
    float4 prevClip = mul(float4(worldPos, 1.0), prevViewProjection);
    float2 prevUV = prevClip.xy / prevClip.w * float2(0.5, -0.5) + 0.5;

    bool valid = all(prevUV >= 0.0) && all(prevUV <= 1.0);

    if (valid)
    {
        float prevDepth = g_PrevDepth.SampleLevel(g_PointClamp, prevUV, 0);
        float reprojDepth = prevClip.z / prevClip.w;
        if (abs(prevDepth - reprojDepth) > 0.01)
            valid = false;
    }

    float3 history = valid ? g_History.SampleLevel(g_LinearClamp, prevUV, 0).rgb : float3(0, 0, 0);

    float blendAlpha = valid ? alpha : 1.0;

    // 初期収束: 最初の数フレームは新データの重みを増やす
    if (frameCount < 8.0)
        blendAlpha = max(blendAlpha, 1.0 / (frameCount + 1.0));

    return float4(lerp(history, currentGI, blendAlpha), 1.0);
}

// ============================================================================
// A-Trous 空間フィルタパス
// ============================================================================
cbuffer SpatialCB : register(b0)
{
    float spatialFullWidth;
    float spatialFullHeight;
    float stepWidth;
    float sigmaDepth;
    float sigmaNormal;
    float sigmaColor;
    float2 _spatialPad;
};

Texture2D<float4> g_Input   : register(t0);
Texture2D<float>  g_SDepth  : register(t1);
Texture2D<float4> g_SNormal : register(t2);

float4 SpatialPS(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 texel = 1.0 / float2(spatialFullWidth, spatialFullHeight);

    float3 cColor = g_Input.SampleLevel(g_PointClamp, uv, 0).rgb;
    float  cDepth = g_SDepth.SampleLevel(g_PointClamp, uv, 0);
    float3 cNormal = normalize(g_SNormal.SampleLevel(g_LinearClamp, uv, 0).rgb * 2.0 - 1.0);

    if (cDepth >= 0.999)
        return float4(cColor, 1.0);

    static const float kernel[3] = { 1.0, 2.0/3.0, 1.0/6.0 };

    float3 weightedSum = float3(0, 0, 0);
    float wSum = 0.0;

    [unroll] for (int dy = -2; dy <= 2; dy++)
    [unroll] for (int dx = -2; dx <= 2; dx++)
    {
        float2 sUV = uv + float2(dx, dy) * texel * stepWidth;
        float3 sColor = g_Input.SampleLevel(g_PointClamp, sUV, 0).rgb;
        float  sDepth = g_SDepth.SampleLevel(g_PointClamp, sUV, 0);
        float3 sNormal = normalize(g_SNormal.SampleLevel(g_LinearClamp, sUV, 0).rgb * 2.0 - 1.0);

        // 深度重み
        float wD = exp(-abs(sDepth - cDepth) / max(sigmaDepth, 0.001));

        // 法線重み
        float wN = pow(max(dot(sNormal, cNormal), 0.0), sigmaNormal);

        // 色差重み
        float3 diff = sColor - cColor;
        float wC = exp(-dot(diff, diff) / max(sigmaColor, 0.001));

        float w = kernel[abs(dx)] * kernel[abs(dy)] * wD * wN * wC;
        weightedSum += sColor * w;
        wSum += w;
    }

    return float4(weightedSum / max(wSum, 0.001), 1.0);
}
