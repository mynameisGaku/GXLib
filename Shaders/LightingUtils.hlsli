// ライト評価ユーティリティ
// Directional / Point / Spot の3種類のライトに対して
// 距離減衰・スポット減衰を適用し、Cook-Torrance BRDFで寄与を計算する。

/// @file LightingUtils.hlsli
/// @brief ライト評価関数（Directional/Point/Spot + 減衰）

#ifndef LIGHTING_UTILS_HLSLI
#define LIGHTING_UTILS_HLSLI

#include "PBRCommon.hlsli"

// ライトタイプ定数
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

// ライトデータ構造体（C++側 LightData と同一レイアウト）
struct LightData
{
    float3 position;   // ワールド座標（Point/Spot用）
    float  range;      // 到達距離（Point/Spot用）
    float3 direction;  // ライト方向（Directional/Spot用）
    float  spotAngle;  // スポットライト外角のcos値
    float3 color;      // ライト色 (リニアRGB)
    float  intensity;  // 強度スカラー
    uint   type;       // LIGHT_DIRECTIONAL / LIGHT_POINT / LIGHT_SPOT
    float3 padding;
};

// ============================================================================
// 距離減衰
// ============================================================================

/// @brief ポイント/スポットライトの距離減衰 — rangeでゼロに到達する二次減衰
float AttenuatePoint(float distance, float range)
{
    // rangeに達すると滑らかにゼロへ減衰
    float attenuation = saturate(1.0f - (distance * distance) / (range * range));
    return attenuation * attenuation;
}

// ============================================================================
// スポットライト減衰
// ============================================================================

/// @brief スポットライトの角度減衰 — コーン端でsmoothstepフェードアウト
float AttenuateSpot(float3 lightDir, float3 spotDir, float spotAngleCos)
{
    float cosAngle = dot(-lightDir, spotDir);
    // 外角=spotAngle、内角は外角と1.0の間(30%位置)でソフトエッジ
    float outerCos = spotAngleCos;
    float innerCos = lerp(outerCos, 1.0f, 0.3f);
    return smoothstep(outerCos, innerCos, cosAngle);
}

// ============================================================================
// 単一ライトの寄与を計算
// ライト種別に応じた方向・減衰を求め、Cook-Torrance BRDFで最終寄与を返す。
// ============================================================================

/// @brief ライト種別に応じたBRDF評価 — 減衰込みの放射輝度寄与を返す
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
