/// @file RTReflectionComposite.hlsl
/// @brief DXR反射結果をシーンにFresnel LERP合成するフルスクリーンパス
///
/// 入力: scene(t0) + depth(t1) + reflection(t2) + normal(t3)
/// Fresnel (Schlick, F0=intensity) + 深度エッジ保持ブラー + LERP合成
/// depth≥0.999 (skybox) はスキップ

#include "Fullscreen.hlsli"

cbuffer CompositeConstants : register(b0)
{
    float    intensity;
    float    debugMode;
    float    screenWidth;
    float    screenHeight;
    float3   cameraPosition;
    float    _pad0;
    float4x4 invViewProjection;
};

Texture2D<float4> g_Scene      : register(t0);
Texture2D<float>  g_Depth      : register(t1);
Texture2D<float4> g_Reflection : register(t2);
Texture2D<float4> g_Normal     : register(t3);

SamplerState g_LinearClamp : register(s0);
SamplerState g_PointClamp  : register(s1);

/// 深度値からワールド座標を復元
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 worldPos = mul(clipPos, invViewProjection);
    return worldPos.xyz / worldPos.w;
}

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;

    float4 sceneColor = g_Scene.SampleLevel(g_LinearClamp, uv, 0);
    float  depth      = g_Depth.SampleLevel(g_PointClamp, uv, 0);

    // スカイボックスは反射スキップ
    if (depth >= 0.999)
        return sceneColor;

    // 反射テクスチャ 深度エッジ保持ブラー (25-tap cross+diag, Gaussian重み)
    uint rw, rh;
    g_Reflection.GetDimensions(rw, rh);
    float2 texel = 1.0 / float2(rw, rh);

    float depthThr = max(depth * 0.02, 0.001);

    static const int2 offs[25] = {
        int2( 0, 0),
        int2(-1, 0), int2(1, 0), int2(0,-1), int2(0, 1),
        int2(-2, 0), int2(2, 0), int2(0,-2), int2(0, 2),
        int2(-1,-1), int2(1,-1), int2(-1, 1), int2(1, 1),
        int2(-3, 0), int2(3, 0), int2(0,-3), int2(0, 3),
        int2(-2,-2), int2(2,-2), int2(-2, 2), int2(2, 2),
        int2(-4, 0), int2(4, 0), int2(0,-4), int2(0, 4)
    };
    static const float gw[25] = {
        0.12,
        0.08, 0.08, 0.08, 0.08,
        0.05, 0.05, 0.05, 0.05,
        0.04, 0.04, 0.04, 0.04,
        0.02, 0.02, 0.02, 0.02,
        0.02, 0.02, 0.02, 0.02,
        0.01, 0.01, 0.01, 0.01
    };

    float4 reflection = float4(0, 0, 0, 0);
    float wSum = 0.0;

    [unroll] for (int i = 0; i < 25; i++)
    {
        float2 sUV = uv + float2(offs[i]) * texel;
        float sd = g_Depth.SampleLevel(g_PointClamp, sUV, 0);
        if (abs(sd - depth) < depthThr)
        {
            float4 s = g_Reflection.SampleLevel(g_PointClamp, sUV, 0);
            reflection += s * gw[i];
            wSum += gw[i];
        }
    }
    reflection = (wSum > 0.001) ? reflection / wSum : float4(0, 0, 0, 0);

    // alpha=0 は無効ピクセル → スキップ
    if (reflection.a <= 0.0)
        return sceneColor;

    // デバッグモード 3: ClosestHit診断 — ブラー・加算なしで生データ表示
    if (debugMode >= 2.5 && debugMode < 3.5)
    {
        float4 raw = g_Reflection.SampleLevel(g_PointClamp, uv, 0);
        if (raw.a <= 0.0) return sceneColor;
        return float4(raw.rgb, 1.0);
    }

    // マテリアルベース Fresnel LERP 合成
    // normal.a にパックされたメタリック+ラフネスをデコード
    float4 normalSample = g_Normal.SampleLevel(g_LinearClamp, uv, 0);
    float rawAlpha = normalSample.a;
    float matMetallic  = step(1.0, rawAlpha);
    float matRoughness = rawAlpha - matMetallic;

    // F0: 非金属=0.04, 金属=1.0
    float F0 = lerp(0.04, 1.0, matMetallic);
    float3 N = normalize(normalSample.rgb * 2.0 - 1.0);
    float3 worldPos = ReconstructWorldPosition(uv, depth);
    float3 V = normalize(cameraPosition - worldPos);
    float NdotV = max(dot(N, V), 0.0);
    float F = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    // ラフネス減衰: 粗い面は反射弱化
    float roughAtten = saturate(1.0 - matRoughness * matRoughness);
    float blendFactor = saturate(reflection.a * F * roughAtten * intensity);
    float3 result = lerp(sceneColor.rgb, reflection.rgb, blendFactor);

    return float4(result, sceneColor.a);
}
