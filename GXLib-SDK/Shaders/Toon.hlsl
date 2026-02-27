/// @file Toon.hlsl
/// @brief UTS2ベース トゥーンシェーダー — ダブルシェード方式
///
/// UnityChan Toon Shader 2 (UTS2) のアルゴリズムに基づく3ゾーンシェーディング。
/// Base Color → 1st Shade → 2nd Shade の3段階グラデーションで
/// アニメ調の自然な明暗を生成する。
/// CSMシャドウは gShadowReceiveLevel で halfLambert に統合。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Toonピクセルシェーダー — UTS2方式の3ゾーンシェーディング + リム + スペキュラ
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

    // --- UTS2 ダブルシェードライティング ---
    float3 Lo = float3(0, 0, 0);

    // メインライトの halfLambert（リムライト方向マスク用に保持）
    float mainHalfLambert = 0.5f;
    // メインライトの shadowMask1（スペキュラ影マスク用に保持）
    float mainShadowMask1 = 0.0f;

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

        float NdotL = dot(N, L);

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);

        // UTS2: Half Lambert + シャドウ統合
        float halfLambert = NdotL * 0.5f + 0.5f;
        halfLambert *= lerp(1.0f, shadow, gShadowReceiveLevel);

        // メインライトの halfLambert 保持（リム方向マスク用）
        if (i == 0)
            mainHalfLambert = halfLambert;

        // UTS2準拠: 片側ランプ（step以下にfeather幅で遷移）
        // shadowMask=0→base(lit), shadowMask=1→shade(dark)
        float feather1 = max(gBaseShadeFeather, 0.001f);
        float shadowMask1 = saturate((gBaseColorStep - halfLambert) / feather1);

        float feather2 = max(gShade1st2ndFeather, 0.001f);
        float shadowMask2 = saturate((gShadeColorStep - halfLambert) / feather2);

        // メインライトの shadowMask1 保持（スペキュラ影マスク用）
        if (i == 0)
            mainShadowMask1 = shadowMask1;

        // 3ゾーン合成: base → 1st shade → 2nd shade (UTS2方式)
        float3 shade1st = albedo.rgb * gShadeColor.rgb;
        float3 shade2nd = albedo.rgb * gShade2ndColor.rgb;
        float3 shadeBlend = lerp(shade1st, shade2nd, shadowMask2);
        float3 toonColor = lerp(albedo.rgb, shadeBlend, shadowMask1);

        // ライトの色と強度
        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        Lo += toonColor * radiance;

        // UTS2準拠 スペキュラハイライト
        if (gHighColorIntensity > 0.0f)
        {
            float3 H = normalize(V + L);
            float NdotH = max(dot(N, H), 0.0f);

            // UTS2: power→可変閾値マッピング
            float normPow = saturate(gHighColorPower / 128.0f);
            float specThreshold = 1.0f - pow(normPow, 5.0f);
            float specMask = step(specThreshold, NdotH) * gHighColorIntensity;

            // 影マスク: gHighColorOnShadow (0=影でスペキュラ消す, 1=維持)
            specMask *= lerp(1.0f - shadowMask1, 1.0f, gHighColorOnShadow);

            // ブレンドモード: gHighColorBlendAdd (0=乗算, 1=加算)
            float3 specAdd = gHighColor * specMask * radiance;
            float3 specMul = toonColor * (1.0f + specMask * (gHighColor - 1.0f));
            Lo += lerp(specMul - toonColor, specAdd, gHighColorBlendAdd) * shadow;
        }
    }

    // --- UTS2準拠 リムライト ---
    float NdotV = max(dot(N, V), 0.0f);
    float rimRaw = pow(1.0f - NdotV, gRimPower) * gRimIntensity;

    // Inside mask: フェザーON/OFF
    float rimFinal;
    if (gRimFeatherOff > 0.5f)
        rimFinal = step(gRimInsideMask, rimRaw);
    else
        rimFinal = saturate((rimRaw - gRimInsideMask) / max(1.0f - gRimInsideMask, 0.001f));

    // ライト方向マスク: mainHalfLambert でlit側に制限
    rimFinal *= lerp(1.0f, mainHalfLambert, gRimLightDirMask);

    float3 rimContribution = gRimColor.rgb * gRimColor.a * rimFinal;

    // --- アンビエント ---
    float3 ambient = gAmbientColor * albedo.rgb * ao;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + rimContribution + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color  = float4(finalColor, albedo.a);
    output.normal = EncodeNormal(N, 0.0f, 1.0f); // Toon: non-metallic
    output.albedo = float4(albedo.rgb, 1.0);
    return output;
}
