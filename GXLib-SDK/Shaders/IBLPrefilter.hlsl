/// @file IBLPrefilter.hlsl
/// @brief 鏡面プリフィルタマップ生成シェーダー
///
/// GGX importance samplingで環境マップをroughness別にプリフィルタリングし、
/// ミップレベルごとに異なるroughnessのぼけ具合を格納する。
/// Split-sum近似の第1項に相当する。

#include "Fullscreen.hlsli"

static const float PI = 3.14159265359f;

TextureCube<float4> gEnvMap : register(t0);
SamplerState gSampler : register(s0);

cbuffer IBLGenConstants : register(b0)
{
    uint gFaceIndex;        // キューブマップフェイス（0-5）
    float gRoughness;       // このミップレベルのroughness
    float2 _genPad;
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

/// @brief Van der Corput基数反転
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

/// @brief GGX importance samplingでハーフベクトルを生成
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

/// @brief 鏡面プリフィルタ — roughnessに応じたGGXサンプリングで環境マップをぼかす
float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float3 N = UVToDirection(input.uv, gFaceIndex);
    float3 R = N;
    float3 V = R;

    static const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    float3 prefilteredColor = float3(0, 0, 0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, gRoughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            prefilteredColor += gEnvMap.SampleLevel(gSampler, L, 0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor /= max(totalWeight, 0.001);

    return float4(prefilteredColor, 1.0);
}
