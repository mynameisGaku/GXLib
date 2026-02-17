/// @file ClearCoat.hlsl
/// @brief PBR + クリアコートシェーダー（2層スペキュラ）
///
/// Cook-Torranceベースレイヤーの上にクリアコート層（第2 GGXスペキュラローブ）を
/// 追加する。自動車塗装、ニス塗り家具、ラッカー仕上げなど光沢コーティング素材向け。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// クリアコート用 BRDF
// ============================================================================

/// @brief クリアコート層のFresnel（Schlick, F0=0.04 = IOR 1.5のコーティング）
float FresnelSchlickClearCoat(float cosTheta, float f0)
{
    return f0 + (1.0f - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

/// @brief クリアコート層の寄与を計算（GGX NDF + Geometry + Fresnel）
float EvaluateClearCoat(float3 N, float3 V, float3 L,
                          float clearCoatRoughness, float clearCoatStrength)
{
    float3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float VdotH = max(dot(V, H), 0.0f);

    if (NdotL <= 0.0f)
        return 0.0f;

    // GGX NDF for clear coat
    float ccRoughness = max(clearCoatRoughness, 0.04f);
    float D = DistributionGGX(N, H, ccRoughness);

    // Geometry (Smith GGX)
    float G = GeometrySmith(N, V, L, ccRoughness);

    // Fresnel (IOR 1.5 -> F0 = 0.04)
    float F = FresnelSchlickClearCoat(VdotH, 0.04f);

    // Cook-Torrance specular
    float denom = 4.0f * NdotV * NdotL + 0.0001f;
    float specular = (D * G * F) / denom;

    return specular * clearCoatStrength;
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================

PSOutput PSMain(PSInput input)
{
    // --- アルベド ---
    float4 albedo = SampleAlbedo(input.texcoord);

    // アルファカットオフ
    if (albedo.a < gAlphaCutoff)
        discard;

    // --- 法線 ---
    float3 N = ApplyNormalMap(input);

    // --- メタリック・ラフネス ---
    float metallic  = gMetallic;
    float roughness = gRoughness;
    if (gMaterialFlags & HAS_METROUGH_MAP)
    {
        float4 mr = tMetRough.Sample(sLinearWrap, input.texcoord);
        metallic  *= mr.b;
        roughness *= mr.g;
    }
    roughness = max(roughness, 0.04f);

    // スペキュラAA
    {
        float3 dNdx = ddx(N);
        float3 dNdy = ddy(N);
        float normalVariance = dot(dNdx, dNdx) + dot(dNdy, dNdy);
        float kernelRoughness = min(2.0 * normalVariance, 0.25);
        roughness = sqrt(roughness * roughness + kernelRoughness);
    }

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- クリアコートマスク ---
    float clearCoatMask = 1.0f;
    if (gMaterialFlags & HAS_CLEARCOAT_MASK_MAP)
    {
        clearCoatMask = tClearCoatMask.Sample(sLinearWrap, input.texcoord).r;
    }
    float effectiveClearCoat = gClearCoatStrength * clearCoatMask;

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- PBR + ClearCoat ライティング ---
    float3 Lo = float3(0, 0, 0);

    for (uint i = 0; i < gNumLights; ++i)
    {
        float3 L;
        float attenuation = 1.0f;

        if (gLights[i].type == LIGHT_DIRECTIONAL)
        {
            L = -gLights[i].direction;
        }
        else if (gLights[i].type == LIGHT_POINT)
        {
            float3 toLight = gLights[i].position - input.posW;
            float dist = length(toLight);
            L = toLight / max(dist, 0.0001f);
            attenuation = AttenuatePoint(dist, gLights[i].range);
        }
        else // LIGHT_SPOT
        {
            float3 toLight = gLights[i].position - input.posW;
            float dist = length(toLight);
            L = toLight / max(dist, 0.0001f);
            attenuation = AttenuatePoint(dist, gLights[i].range);
            attenuation *= AttenuateSpot(L, gLights[i].direction, gLights[i].spotAngle);
        }

        float NdotL = max(dot(N, L), 0.0f);

        // ベースレイヤー PBR BRDF
        float3 baseBRDF = float3(0, 0, 0);
        if (NdotL > 0.0f && attenuation > 0.0f)
        {
            baseBRDF = CookTorranceBRDF(N, V, L, albedo.rgb, metallic, roughness);
        }

        // クリアコートレイヤー
        float ccSpecular = EvaluateClearCoat(N, V, L,
                                              gClearCoatRoughness, effectiveClearCoat);

        // クリアコートのFresnel吸収: コーティングが反射した分だけベース層が減衰
        float3 H = normalize(V + L);
        float VdotH = max(dot(V, H), 0.0f);
        float ccFresnel = FresnelSchlickClearCoat(VdotH, 0.04f);
        float baseAttenuation = 1.0f - effectiveClearCoat * ccFresnel;

        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);

        // 合算: ベースレイヤー(減衰) + クリアコート
        Lo += (baseBRDF * baseAttenuation * NdotL + ccSpecular * NdotL) * radiance * shadow;
    }

    // --- アンビエント ---
    float3 ambient = gAmbientColor * albedo.rgb * ao;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    // クリアコートはラフネスを下げた見た目を反映するため、
    // normalのパックにはクリアコートで修正されたラフネスを使用
    float effectiveRoughness = lerp(roughness, gClearCoatRoughness, effectiveClearCoat);

    PSOutput output;
    output.color  = float4(finalColor, albedo.a);
    output.normal = EncodeNormal(N, metallic, effectiveRoughness);
    output.albedo = float4(albedo.rgb, 1.0);
    return output;
}
