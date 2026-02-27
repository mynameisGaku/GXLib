// PBR BRDF関数群（Cook-Torrance Microfacet Model）
// GGX/Trowbridge-Reitz NDF、Smith幾何減衰、Schlick Fresnel、
// これらを組み合わせたCook-Torrance BRDF を提供する。

/// @file PBRCommon.hlsli
/// @brief PBR BRDF関数（Cook-Torrance）

#ifndef PBR_COMMON_HLSLI
#define PBR_COMMON_HLSLI

static const float PI = 3.14159265359f;

// ============================================================================
// Normal Distribution Function (NDF): GGX/Trowbridge-Reitz
// マイクロファセットの法線分布を表す。roughnessが大きいほどハイライトが広がる。
// ============================================================================

/// @brief GGX法線分布関数 — マイクロファセットがハーフベクトルHを向く確率密度
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
// 幾何減衰関数 (Geometry Function): Smith法 + Schlick-GGX近似
// マイクロファセットの自己遮蔽と自己マスキングを表現する。
// ============================================================================

/// @brief Schlick-GGX片側遮蔽 — 視線 or ライト方向の幾何減衰
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

/// @brief Smith幾何減衰 — 視線側とライト側の両方を合成
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// ============================================================================
// Fresnel: Schlick近似
// 視線が表面に浅い角度で当たるほど反射率が上がる現象をモデル化。
// ============================================================================

/// @brief Schlick Fresnel — 入射角cosθとF0(垂直入射時の反射率)から反射係数を算出
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// ============================================================================
// Cook-Torrance Specular BRDF
// D(NDF) * G(幾何減衰) * F(Fresnel) の積でスペキュラを計算し、
// エネルギー保存を満たすLambertian拡散と合算する。
// ============================================================================

/// @brief Cook-Torrance BRDF — 拡散+鏡面反射の合計を返す
float3 CookTorranceBRDF(float3 N, float3 V, float3 L, float3 albedo,
                          float metallic, float roughness)
{
    float3 H = normalize(V + L);

    // F0: 金属はアルベド色がそのまま反射色、非金属は一律0.04
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    // Cook-Torrance三要素
    float  D = DistributionGGX(N, H, roughness);   // 法線分布
    float  G = GeometrySmith(N, V, L, roughness);   // 幾何減衰
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0); // フレネル

    // スペキュラ項: DGF / (4 * NdotV * NdotL)
    float3 numerator  = D * G * F;
    float  denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;

    // エネルギー保存: スペキュラで反射した分を拡散から差し引く
    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic); // 金属は拡散なし

    // Lambertian拡散
    float3 diffuse = kD * albedo / PI;

    return diffuse + specular;
}

// ============================================================================
// Fresnel（roughness考慮版）
// IBLの環境反射でroughnessが高い場合にフレネル効果を抑える。
// ============================================================================

/// @brief Schlick Fresnel（roughness考慮版）— IBL用
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) *
           pow(saturate(1.0 - cosTheta), 5.0);
}

// ============================================================================
// IBLテクスチャ (t15-t17)
// 環境マップから事前計算された間接照明テクスチャ（Renderer3Dで自動バインド）
// ============================================================================

TextureCube<float4> tIBLIrradiance  : register(t15); // 拡散照射キューブマップ（コサイン畳み込み済み）
TextureCube<float4> tIBLPrefiltered : register(t16); // 鏡面プリフィルタキューブマップ（roughnessミップ）
Texture2D<float4>   tIBLBRDFLUT    : register(t17); // BRDF積分LUT (x=scale, y=bias)
SamplerState        sLinearClamp   : register(s1);   // リニアClampサンプラー（IBL/BRDF LUT用）

// ============================================================================
// IBL間接照明
// Split-sum近似で拡散+鏡面の環境反射を計算する。
// ============================================================================

/// @brief IBL間接照明 — 拡散(照射マップ) + 鏡面(プリフィルタマップ + BRDF LUT)
float3 EvaluateIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness, float ao)
{
    float NdotV = max(dot(N, V), 0.0);
    float3 R = reflect(-V, N);

    // F0: 非金属=0.04, 金属=アルベド色
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    // エネルギー保存
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    // 拡散IBL: 照射マップからコサイン畳み込み済みの放射輝度を取得
    float3 irradiance = tIBLIrradiance.Sample(sLinearClamp, N).rgb;
    float3 diffuseIBL = kD * albedo * irradiance;

    // 鏡面IBL: プリフィルタマップ（roughness→ミップ）+ BRDF LUT
    float maxMipLevel = 4.0; // k_PrefilteredMipLevels - 1
    float3 prefilteredColor = tIBLPrefiltered.SampleLevel(sLinearClamp, R, roughness * maxMipLevel).rgb;
    float2 envBRDF = tIBLBRDFLUT.Sample(sLinearClamp, float2(NdotV, roughness)).rg;
    float3 specularIBL = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    return (diffuseIBL + specularIBL) * ao;
}

#endif // PBR_COMMON_HLSLI
