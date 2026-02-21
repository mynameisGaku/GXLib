/// @file IBLEnvCapture.hlsl
/// @brief プロシージャルSkyboxからキューブマップをキャプチャするシェーダー
///
/// Skybox.hlslと同じプロシージャル空の計算をキューブマップ6面にレンダリングする。
/// 天頂色/地平色のグラデーション＋太陽ハイライトで空を生成する。

#include "Fullscreen.hlsli"

static const float PI = 3.14159265359f;

cbuffer IBLEnvConstants : register(b0)
{
    uint   gFaceIndex;
    float  gSunIntensity;
    float2 _envPad;
    float3 gTopColor;
    float  _envPad2;
    float3 gBottomColor;
    float  _envPad3;
    float3 gSunDirection;
    float  _envPad4;
};

/// @brief UV座標とフェイスインデックスからキューブマップ方向ベクトルを算出
float3 UVToDirection(float2 uv, uint face)
{
    float u = uv.x * 2.0 - 1.0;
    float v = uv.y * 2.0 - 1.0;
    v = -v;

    float3 dir = float3(0, 0, 0);
    switch (face)
    {
    case 0: dir = float3( 1,  v, -u); break;
    case 1: dir = float3(-1,  v,  u); break;
    case 2: dir = float3( u,  1, -v); break;
    case 3: dir = float3( u, -1,  v); break;
    case 4: dir = float3( u,  v,  1); break;
    case 5: dir = float3(-u,  v, -1); break;
    }
    return normalize(dir);
}

/// @brief プロシージャルSky生成 — Skybox.hlslと同じ計算ロジック
float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float3 dir = UVToDirection(input.uv, gFaceIndex);

    // 天頂〜地平のグラデーション
    float t = dir.y * 0.5 + 0.5; // [-1,1] → [0,1]
    float3 skyColor = lerp(gBottomColor, gTopColor, saturate(t));

    // 太陽ハイライト
    float3 sunDir = normalize(-gSunDirection); // ライト方向を反転して太陽位置に
    float sunDot = max(dot(dir, sunDir), 0.0);
    float sunDisc = pow(sunDot, 256.0);     // 鋭い太陽円盤
    float sunGlow = pow(sunDot, 8.0) * 0.5; // 柔らかいグロー
    skyColor += (sunDisc + sunGlow) * gSunIntensity;

    return float4(skyColor, 1.0);
}
