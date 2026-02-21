/// @file RTGIComposite.hlsl
/// @brief GI 加算コンポジット
///
/// デノイズ済みGI × アルベド × 強度 をシーンに加算合成

#include "Fullscreen.hlsli"

cbuffer RTGICompositeCB : register(b0)
{
    float intensity;   // GI強度
    float debugMode;   // デバッグ表示 (0=off, 1=GIのみ表示)
    float fullWidth;   // スクリーン幅
    float fullHeight;  // スクリーン高さ
};

Texture2D<float4> g_Scene     : register(t0);
Texture2D<float4> g_GI        : register(t1);
Texture2D<float>  g_Depth     : register(t2);
Texture2D<float4> g_Albedo    : register(t3);

SamplerState g_LinearClamp : register(s0);
SamplerState g_PointClamp  : register(s1);

/// @brief GI合成PS — デノイズ済みGI × アルベド × 強度をシーンに加算合成
float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 scene = g_Scene.SampleLevel(g_LinearClamp, uv, 0);
    float  depth = g_Depth.SampleLevel(g_PointClamp, uv, 0);

    if (depth >= 0.999)
        return scene;

    float3 gi = g_GI.SampleLevel(g_LinearClamp, uv, 0).rgb;
    float3 albedo = g_Albedo.SampleLevel(g_LinearClamp, uv, 0).rgb;

    // デバッグモード 1: GIのみ表示
    if (debugMode >= 0.5 && debugMode < 1.5)
        return float4(gi * albedo * intensity, 1.0);

    // 加算合成: GI入射光 × アルベド × 強度
    float3 result = scene.rgb + gi * albedo * intensity;
    return float4(result, scene.a);
}
