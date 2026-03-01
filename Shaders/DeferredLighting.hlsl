/// @file DeferredLighting.hlsl
/// @brief Deferred Rendering ライティングパス
///
/// GBufferからマテリアル属性を読み取り、深度バッファからワールド座標を復元し、
/// PBR Cook-Torrance BRDF でライティングを計算してHDR出力する。
/// シャドウマップ・IBLは使用せず、直接光 + アンビエント + フォグのみ。
/// （シャドウ/IBLが必要な場合は Forward パスを使用すること）

#include "Fullscreen.hlsli"

// ============================================================================
// PBR BRDF 関数（PBRCommon.hlsli から必要部分を抜粋）
// 独立シェーダーのため IBL テクスチャ宣言を避けて自己完結させる
// ============================================================================

static const float PI = 3.14159265359f;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0000001f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 CookTorranceBRDF(float3 N, float3 V, float3 L, float3 albedo,
                          float metallic, float roughness)
{
    float3 H = normalize(V + L);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float  D = DistributionGGX(N, H, roughness);
    float  G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    float3 numerator  = D * G * F;
    float  denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;
    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;
    return diffuse + specular;
}

// ============================================================================
// ライト評価（LightingUtils.hlsli から抜粋）
// ============================================================================

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2
#define MAX_LIGHTS 16

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

float AttenuatePoint(float distance, float range)
{
    float attenuation = saturate(1.0f - (distance * distance) / (range * range));
    return attenuation * attenuation;
}

float AttenuateSpot(float3 lightDir, float3 spotDir, float spotAngleCos)
{
    float cosAngle = dot(-lightDir, spotDir);
    float outerCos = spotAngleCos;
    float innerCos = lerp(outerCos, 1.0f, 0.3f);
    return smoothstep(outerCos, innerCos, cosAngle);
}

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

// ============================================================================
// 定数バッファ
// ============================================================================

cbuffer DeferredFrameConstants : register(b0)
{
    float4x4 gView;
    float3   gCameraPosition;
    float    _pad0;
    float3   gFogColor;
    float    gFogStart;
    float    gFogEnd;
    float    gFogDensity;
    uint     gFogMode;
    float    _pad1;
};

cbuffer DeferredLightConstants : register(b1)
{
    LightData gLights[MAX_LIGHTS];
    float3    gAmbientColor;
    uint      gNumLights;
};

cbuffer DeferredExtraConstants : register(b2)
{
    float4x4 gInvViewProjection;
};

// ============================================================================
// GBuffer テクスチャ
// ============================================================================

Texture2D<float4> gGBuffer0 : register(t0); // Normal.xyz + Roughness
Texture2D<float4> gGBuffer1 : register(t1); // Albedo.rgb + Metallic
Texture2D<float4> gGBuffer2 : register(t2); // Emissive.rgb + AO
Texture2D<float>  gDepth    : register(t3); // 深度バッファ

SamplerState gPointSampler : register(s0);

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Deferred ライティングPS — GBufferからマテリアル読み取り → PBRライティング → HDR出力
float4 PSDeferredLighting(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;

    // --- GBuffer読み取り ---
    float4 gb0 = gGBuffer0.Sample(gPointSampler, uv);
    float4 gb1 = gGBuffer1.Sample(gPointSampler, uv);
    float4 gb2 = gGBuffer2.Sample(gPointSampler, uv);
    float  depth = gDepth.Sample(gPointSampler, uv).r;

    // スカイボックス（depth>=1.0）はライティングスキップ
    if (depth >= 1.0f)
        return float4(0, 0, 0, 0);

    // --- GBufferデコード ---
    float3 N         = normalize(gb0.xyz * 2.0f - 1.0f);
    float  roughness = max(gb0.w, 0.04f);
    float3 albedo    = gb1.rgb;
    float  metallic  = gb1.a;
    float3 emissive  = gb2.rgb;
    float  ao        = gb2.a;

    // --- ワールド座標復元 ---
    float2 ndc = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldPos4 = mul(clipPos, gInvViewProjection);
    float3 posW = worldPos4.xyz / worldPos4.w;

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - posW);

    // --- PBRライティング ---
    float3 Lo = float3(0, 0, 0);
    for (uint i = 0; i < gNumLights; ++i)
    {
        Lo += EvaluateLight(gLights[i], posW, N, V, albedo, metallic, roughness);
    }

    // --- アンビエント ---
    float3 ambient = gAmbientColor * albedo * ao;

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    if (gFogMode > 0)
    {
        float dist = length(posW - gCameraPosition);
        float fogFactor = 0.0f;
        if (gFogMode == 1)
            fogFactor = saturate((gFogEnd - dist) / (gFogEnd - gFogStart));
        else if (gFogMode == 2)
            fogFactor = exp(-gFogDensity * dist);
        else
        {
            float d = gFogDensity * dist;
            fogFactor = exp(-d * d);
        }
        finalColor = lerp(gFogColor, finalColor, fogFactor);
    }

    return float4(finalColor, 1.0f);
}
