/// @file Toon.hlsl
/// @brief トゥーン（セル）シェーダー — ステップ/ランプ照明 + リムライト
///
/// NdotLをランプ関数で量子化し、アニメ調の明暗を生成する。
/// gShadeColor で暗部の色味を制御、gRampBands でバンド数を設定。
/// リムライトは Fresnel的な NdotV を使い縁を光らせる。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// ランプ関数: NdotLをバンド数で量子化
// ============================================================================

float ToonRamp(float NdotL, float threshold, float smoothing, uint bands)
{
    // テクスチャランプが使用可能な場合はそちらを使用
    if (gMaterialFlags & HAS_TOON_RAMP_MAP)
    {
        // ランプテクスチャの横方向をNdotLで参照（V=0.5固定）
        float u = saturate(NdotL * 0.5f + 0.5f);
        return tToonRamp.Sample(sLinearWrap, float2(u, 0.5f)).r;
    }

    // バンド数による量子化ランプ
    if (bands <= 1)
    {
        // 2値（影/光）: smoothstepでソフトエッジ
        return smoothstep(threshold - smoothing, threshold + smoothing, NdotL);
    }

    // 複数バンド: NdotLを[0,1]に正規化→バンド数で量子化
    float halfLambert = NdotL * 0.5f + 0.5f;
    float stepped = floor(halfLambert * (float)bands) / (float)bands;
    // バンド境界をスムージング
    float frac_ = frac(halfLambert * (float)bands);
    float blend = smoothstep(0.5f - smoothing, 0.5f + smoothing, frac_);
    float nextStep = min(stepped + 1.0f / (float)bands, 1.0f);
    return lerp(stepped, nextStep, blend);
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

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- トゥーンライティング ---
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

        float NdotL = dot(N, L);

        // ランプ照明
        float ramp = ToonRamp(NdotL, gRampThreshold, gRampSmoothing, gRampBands);

        // lit色とshade色をランプでブレンド
        float3 litColor = albedo.rgb;
        float3 shadedColor = albedo.rgb * gShadeColor.rgb;
        float3 toonColor = lerp(shadedColor, litColor, ramp);

        // ライトの色と強度
        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);

        Lo += toonColor * radiance * shadow;
    }

    // --- リムライト ---
    float NdotV = max(dot(N, V), 0.0f);
    float rimFactor = pow(1.0f - NdotV, gRimPower) * gRimIntensity;
    float3 rimContribution = gRimColor.rgb * gRimColor.a * rimFactor;

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
