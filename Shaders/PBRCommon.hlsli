/// @file PBRCommon.hlsli
/// @brief PBR BRDF関数（Cook-Torrance）

#ifndef PBR_COMMON_HLSLI
#define PBR_COMMON_HLSLI

static const float PI = 3.14159265359f;

// ============================================================================
// Normal Distribution Function (NDF): GGX/Trowbridge-Reitz
// ============================================================================
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

// ============================================================================
// Geometry Function: Smith's method with Schlick-GGX
// ============================================================================
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
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// ============================================================================
// Fresnel: Schlick Approximation
// ============================================================================
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// ============================================================================
// Cook-Torrance Specular BRDF
// ============================================================================
float3 CookTorranceBRDF(float3 N, float3 V, float3 L, float3 albedo,
                          float metallic, float roughness)
{
    float3 H = normalize(V + L);

    // F0: 金属は反射率がalbedo色、非金属は0.04
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    // Cook-Torrance
    float  D = DistributionGGX(N, H, roughness);
    float  G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator  = D * G * F;
    float  denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;

    // kS: 鏡面反射の割合、kD: 拡散反射の割合
    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    // Lambertian拡散
    float3 diffuse = kD * albedo / PI;

    return diffuse + specular;
}

#endif // PBR_COMMON_HLSLI
