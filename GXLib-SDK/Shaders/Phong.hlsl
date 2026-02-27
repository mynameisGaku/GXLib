/// @file Phong.hlsl
/// @brief Blinn-Phongライティングモデル
///
/// Ambient + Diffuse(NdotL) + Specular(NdotH^shininess) のクラシックな照明モデル。
/// PBRではなく古典的な見た目が必要な場合やパフォーマンス重視の場面向け。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Blinn-PhongPS — Lambert拡散 + Blinn-Phongスペキュラ + CSMシャドウ
PSOutput PSMain(PSInput input)
{
    // --- アルベド ---
    float4 albedo = SampleAlbedo(input.texcoord);

    // アルファカットオフ
    if (albedo.a < gAlphaCutoff)
        discard;

    // --- 法線 ---
    float3 N = ApplyNormalMap(input);

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- Blinn-Phong ライティング ---
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

        // Diffuse (Lambert)
        float3 diffuse = albedo.rgb * NdotL;

        // Specular (Blinn-Phong)
        float3 H = normalize(V + L);
        float NdotH = max(dot(N, H), 0.0f);
        // 正規化係数: エネルギー保存のためパワーに応じてスケール
        float specNorm = (gShininess + 8.0f) / (8.0f * PI);
        float3 specular = gSpecularColor * specNorm * pow(NdotH, gShininess);

        // ライトの色と強度
        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);

        Lo += (diffuse + specular) * radiance * shadow;
    }

    // --- アンビエント + IBL ---
    // Phongモデルではmetallic=0, roughness=gRoughnessとして近似的にIBLを適用
    float3 iblContrib = EvaluateIBL(N, V, albedo.rgb, 0.0, gRoughness, ao);
    float3 ambient = iblContrib + gAmbientColor * albedo.rgb * ao;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color  = float4(finalColor, albedo.a);
    output.normal = EncodeNormal(N, 0.0f, gRoughness); // Phong: non-metallic
    output.albedo = float4(albedo.rgb, 1.0);
    return output;
}
