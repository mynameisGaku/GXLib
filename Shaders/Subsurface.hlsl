/// @file Subsurface.hlsl
/// @brief PBR + Burley SSS近似シェーダー
///
/// Cook-Torrance BRDFベースのPBRライティングに、サブサーフェス散乱（SSS）を
/// 加算する。SSSはライトラッピング（前面拡散）と背面透過の2成分で構成。
/// 肌、蝋、葉、大理石などの半透明マテリアル向け。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// SSS ヘルパー関数
// ============================================================================

/// @brief ライトラッピング（前面拡散の拡張）
/// @param NdotL 通常のLambert dot product
/// @param wrap  ラッピング範囲 (0=通常Lambert, 1=半球全面)
float WrapDiffuse(float NdotL, float wrap)
{
    return saturate((NdotL + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
}

/// @brief 背面透過（Back-lit transmission）
/// @param V  視線方向
/// @param L  ライト方向
/// @param N  法線
/// @param thickness 厚み（0=薄い→高透過, 1=厚い→低透過）
/// @param radius 散乱半径（拡散の広がり）
float3 SubsurfaceTransmission(float3 V, float3 L, float3 N,
                                float thickness, float radius,
                                float3 sssColor, float strength)
{
    // 背面ライト方向: ライトが法線の反対側から入射
    float3 backLight = -L + N * radius;
    float VdotBack = saturate(dot(V, normalize(backLight)));

    // 厚みで減衰（薄い部分ほど光が透過）
    float thicknessAtten = saturate(1.0f - thickness);

    // 指数減衰でソフトな透過を表現
    float transmissionFactor = pow(VdotBack, 4.0f) * thicknessAtten * strength;

    return sssColor * transmissionFactor;
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Subsurfaceピクセルシェーダー — PBR BRDF + ライトラッピング + 背面透過SSS
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

    // --- SSS厚みマップ ---
    float sssThickness = gThickness;
    if (gMaterialFlags & HAS_SUBSURFACE_MAP)
    {
        sssThickness *= tSubsurface.Sample(sLinearWrap, input.texcoord).r;
    }

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- PBR + SSS ライティング ---
    float3 Lo = float3(0, 0, 0);
    float3 sssTotal = float3(0, 0, 0);

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

        // PBR BRDF
        float3 brdf = float3(0, 0, 0);
        if (NdotL > 0.0f && attenuation > 0.0f)
        {
            brdf = CookTorranceBRDF(N, V, L, albedo.rgb, metallic, roughness);
        }

        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);

        // PBR contribution
        Lo += brdf * radiance * NdotL * shadow;

        // SSS: ライトラッピング（影響はシャドウに関係なく拡散）
        float wrapDiffuse = WrapDiffuse(dot(N, L), gSubsurfaceRadius);
        float3 sssWrap = albedo.rgb * gSubsurfaceColor * wrapDiffuse * gSubsurfaceStrength;

        // SSS: 背面透過
        float3 sssTransmit = SubsurfaceTransmission(
            V, L, N, sssThickness, gSubsurfaceRadius,
            gSubsurfaceColor, gSubsurfaceStrength);

        sssTotal += (sssWrap + sssTransmit) * radiance * attenuation * shadow;
    }

    // --- アンビエント ---
    float3 ambient = gAmbientColor * albedo.rgb * ao;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + sssTotal + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color  = float4(finalColor, albedo.a);
    output.normal = EncodeNormal(N, metallic, roughness);
    output.albedo = float4(albedo.rgb, 1.0);
    return output;
}
