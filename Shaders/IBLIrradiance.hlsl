/// @file IBLIrradiance.hlsl
/// @brief 拡散照射マップ生成シェーダー（コサイン畳み込み）
///
/// 環境キューブマップを全方向でコサイン重み付きサンプリングし、
/// 拡散反射用の低解像度キューブマップを生成する。
/// 各フェイスごとにフルスクリーン描画してRT書き込みする。

#include "Fullscreen.hlsli"

static const float PI = 3.14159265359f;

TextureCube<float4> gEnvMap : register(t0);
SamplerState gSampler : register(s0);

cbuffer IBLGenConstants : register(b0)
{
    uint gFaceIndex;        // キューブマップフェイス（0-5）
    float gRoughness;       // プリフィルタ時のroughness（照射マップでは未使用）
    float2 _genPad;
};

/// @brief UV座標とフェイスインデックスからキューブマップ方向ベクトルを算出
float3 UVToDirection(float2 uv, uint face)
{
    // UV [0,1] → [-1,+1]
    float u = uv.x * 2.0 - 1.0;
    float v = uv.y * 2.0 - 1.0;
    v = -v; // DirectX UV Y反転

    float3 dir = float3(0, 0, 0);
    switch (face)
    {
    case 0: dir = float3( 1,  v, -u); break; // +X
    case 1: dir = float3(-1,  v,  u); break; // -X
    case 2: dir = float3( u,  1, -v); break; // +Y
    case 3: dir = float3( u, -1,  v); break; // -Y
    case 4: dir = float3( u,  v,  1); break; // +Z
    case 5: dir = float3(-u,  v, -1); break; // -Z
    }
    return normalize(dir);
}

/// @brief 拡散照射マップ生成 — 半球上のコサイン重み付き積分
float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float3 N = UVToDirection(input.uv, gFaceIndex);

    // 半球サンプリング用のTBN構築
    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(0, 0, 1);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float3 irradiance = float3(0, 0, 0);

    // 一様サンプリング（球面座標）
    float sampleDelta = 0.025;
    uint nrSamples = 0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // 球面座標 → 直交座標（接空間）
            float3 tangentSample = float3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta));

            // 接空間 → ワールド空間
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += gEnvMap.Sample(gSampler, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance / float(nrSamples);

    return float4(irradiance, 1.0);
}
