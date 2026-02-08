/// @file LightingUtils.hlsli
/// @brief ライト評価関数（Directional/Point/Spot + 減衰）

#ifndef LIGHTING_UTILS_HLSLI
#define LIGHTING_UTILS_HLSLI

#include "PBRCommon.hlsli"

// ライトタイプ定数
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

// ライトデータ構造体（C++側LightDataと対応）
struct LightData
{
    float3 position;
    float  range;
    float3 direction;
    float  spotAngle;
    float3 color;
    float  intensity;
    uint   type;
    float3 padding;
};

// ============================================================================
// 距離減衰
// ============================================================================
float AttenuatePoint(float distance, float range)
{
    // Smoothly fade to zero at range
    float attenuation = saturate(1.0f - (distance * distance) / (range * range));
    return attenuation * attenuation;
}

// ============================================================================
// スポットライト減衰
// ============================================================================
float AttenuateSpot(float3 lightDir, float3 spotDir, float spotAngleCos)
{
    float cosAngle = dot(-lightDir, spotDir);
    // Inner angle = spotAngle, outer = spotAngle * 0.8 for soft edge
    float outerCos = spotAngleCos;
    float innerCos = lerp(outerCos, 1.0f, 0.3f);
    return smoothstep(outerCos, innerCos, cosAngle);
}

// ============================================================================
// 単一ライトの寄与を計算
// ============================================================================
float3 EvaluateLight(LightData light, float3 posW, float3 N, float3 V,
                      float3 albedo, float metallic, float roughness)
{
    float3 L;
    float attenuation = 1.0f;

    if (light.type == LIGHT_DIRECTIONAL)
    {
        L = -light.direction;
    }
    else if (light.type == LIGHT_POINT)
    {
        float3 toLight = light.position - posW;
        float distance = length(toLight);
        L = toLight / max(distance, 0.0001f);
        attenuation = AttenuatePoint(distance, light.range);
    }
    else // LIGHT_SPOT
    {
        float3 toLight = light.position - posW;
        float distance = length(toLight);
        L = toLight / max(distance, 0.0001f);
        attenuation = AttenuatePoint(distance, light.range);
        attenuation *= AttenuateSpot(L, light.direction, light.spotAngle);
    }

    float NdotL = max(dot(N, L), 0.0f);
    if (NdotL <= 0.0f || attenuation <= 0.0f)
        return float3(0, 0, 0);

    float3 brdf = CookTorranceBRDF(N, V, L, albedo, metallic, roughness);
    float3 radiance = light.color * light.intensity * attenuation;

    return brdf * radiance * NdotL;
}

#endif // LIGHTING_UTILS_HLSLI
