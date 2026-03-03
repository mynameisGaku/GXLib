/// @file Eye.hlsl
/// @brief 眼球シェーダー (角膜屈折 + 虹彩深度 + 強膜SSS)
///
/// 眼球の構造（角膜・虹彩・瞳孔・強膜・リンバス）を物理ベースで再現する。
/// 角膜のフレネル反射、虹彩のパララックスによる深度感、
/// 強膜のサブサーフェス散乱、リンバスダーク二ングを実装する。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// Eye用定数 (ShaderModelGPUParams aliased region offset 208-255)
// ============================================================================
// gIrisColor       = gCustom0.xyz (offset 208-216)
// gPupilSize       = gCustom0.w   (offset 220)
// gScleraColor     = gCustom1.xyz (offset 224-232)
// gIrisIOR         = gCustom1.w   (offset 236)
// gCorneaReflection= gCustom2.x   (offset 240)
// gIrisDepth       = gCustom2.y   (offset 244)
// gLimbusWidth     = gCustom2.z   (offset 248)
// (padding)        = gCustom2.w   (offset 252)

// ============================================================================
// Eye ヘルパー関数
// ============================================================================

/// @brief 虹彩のパララックスオフセット
/// @param viewDir ビュー方向 (接線空間)
/// @param irisDepth 虹彩の深さ
/// @param pupilSize 瞳孔サイズ
float2 IrisParallax(float3 viewDirTS, float irisDepth, float pupilSize)
{
    // 簡易パララックスマッピング: ビュー方向に基づく虹彩UV オフセット
    float2 offset = viewDirTS.xy / max(viewDirTS.z, 0.1f) * irisDepth;
    return offset * (1.0f - pupilSize);
}

/// @brief リンバスダークニング
/// @param dist テクスチャ中心からの距離 (0=中心, 1=エッジ)
/// @param limbusWidth リンバスの幅
float LimbusDarkening(float dist, float limbusWidth)
{
    float innerEdge = 1.0f - limbusWidth;
    return 1.0f - smoothstep(innerEdge, 1.0f, dist);
}

/// @brief 角膜フレネル反射 (Schlick近似)
/// @param cosTheta 入射角のコサイン
/// @param ior 屈折率
float CorneaFresnel(float cosTheta, float ior)
{
    float f0 = pow((ior - 1.0f) / (ior + 1.0f), 2.0f);
    return f0 + (1.0f - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Eyeピクセルシェーダー - 角膜屈折 + 虹彩パララックス + 強膜SSS
PSOutput PSMain(PSInput input)
{
    // --- パラメータ取得 ---
    float3 irisColor       = gCustom0.xyz;
    float  pupilSize       = gCustom0.w;
    float3 scleraColor     = gCustom1.xyz;
    float  irisIOR         = gCustom1.w;
    float  corneaReflection = gCustom2.x;
    float  irisDepth       = gCustom2.y;
    float  limbusWidth     = gCustom2.z;

    // --- アルベド (虹彩テクスチャ) ---
    float4 albedo = SampleAlbedo(input.texcoord);

    // アルファカットオフ
    if (albedo.a < gAlphaCutoff)
        discard;

    // --- 法線 ---
    float3 N = ApplyNormalMap(input);

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // 接線空間のビュー方向
    float3 T = normalize(input.tangent.xyz);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = float3x3(T, B, N);
    float3 viewDirTS = mul(V, TBN);

    // --- 虹彩パララックス ---
    float2 irisOffset = IrisParallax(viewDirTS, irisDepth, pupilSize);
    float2 irisUV = input.texcoord + irisOffset;

    // 虹彩カラー (テクスチャ * tint)
    float4 irisTexColor = tAlbedo.Sample(sLinearWrap, irisUV);
    float3 eyeColor = irisTexColor.rgb * irisColor;

    // --- テクスチャ中心からの距離 (虹彩/強膜判定) ---
    float2 centeredUV = input.texcoord * 2.0f - 1.0f;
    float distFromCenter = length(centeredUV);

    // リンバスダークニング
    float limbus = LimbusDarkening(distFromCenter, limbusWidth);

    // 虹彩/強膜ブレンド
    float irisRegion = smoothstep(0.5f, 0.45f, distFromCenter);
    float3 baseColor = lerp(scleraColor * albedo.rgb, eyeColor * limbus, irisRegion);

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- 角膜反射 ---
    float NdotV = max(dot(N, V), 0.0f);
    float fresnel = CorneaFresnel(NdotV, irisIOR) * corneaReflection;

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- ライティング ---
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
        else
        {
            float3 toLight = gLights[i].position - input.posW;
            float dist = length(toLight);
            L = toLight / max(dist, 0.0001f);
            attenuation = AttenuatePoint(dist, gLights[i].range);
            attenuation *= AttenuateSpot(L, gLights[i].direction, gLights[i].spotAngle);
        }

        float NdotL = max(dot(N, L), 0.0f);
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);
        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // ディフューズ
        float3 diffuse = baseColor * NdotL;

        // 角膜スペキュラ (GGX)
        float3 H = normalize(V + L);
        float NdotH = max(dot(N, H), 0.0f);
        float specD = DistributionGGX(N, H, 0.04f); // 非常に滑らかな角膜
        float specG = GeometrySmith(N, V, L, 0.04f);
        float specular = (specD * specG * fresnel) / max(4.0f * NdotV * NdotL, 0.001f);

        Lo += (diffuse + specular) * radiance * shadow;
    }

    // --- アンビエント + IBL ---
    float3 iblContrib = EvaluateIBL(N, V, baseColor, 0.0f, 0.3f, ao);
    float3 ambient = iblContrib + gAmbientColor * baseColor * ao;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color    = float4(finalColor, albedo.a);
    output.normal   = EncodeNormal(N, 0.0f, 0.04f);
    output.albedo   = float4(baseColor, 1.0);
    output.velocity = EncodeVelocity(input.currClipPos, input.prevClipPos);
    return output;
}
