/// @file Skin.hlsl
/// @brief スキンシェーダー (Pre-Integrated SSS + マイクロ法線 + キャビティ)
///
/// 人間の皮膚を高品質にレンダリングするための専用シェーダー。
/// Pre-Integrated Skin Shading (Penner 2011) のルックアップテーブル方式で
/// 高速にSSSを近似し、マイクロ法線による毛穴ディテール、
/// キャビティマップによる陰影強調を実装する。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// Skin用定数 (ShaderModelGPUParams aliased region offset 208-255)
// ============================================================================
// gSubsurfaceColorSkin = gCustom0.xyz (offset 208-216)
// gSubsurfaceRadius    = gCustom0.w   (offset 220)
// gSubsurfaceStrSkin   = gCustom1.x   (offset 224)
// gMicroNormalScale    = gCustom1.y   (offset 228)
// gCavityStrength      = gCustom1.z   (offset 232)
// gFresnelOffset       = gCustom1.w   (offset 236)
// gSpecTint            = gCustom2.xyz (offset 240-248)
// (padding)            = gCustom2.w   (offset 252)

// ============================================================================
// Skin ヘルパー関数
// ============================================================================

/// @brief Pre-Integrated SSS ルックアップ近似
/// @param NdotL Lambert dot product
/// @param curvature サーフェス曲率 (fwidth(N) から推定)
float3 PreIntegratedSkin(float NdotL, float curvature, float3 sssColor, float sssStrength)
{
    // Pre-Integrated Skinの近似: NdotLを柔らかくラップ
    float wrappedNdotL = saturate(NdotL * 0.5f + 0.5f);

    // 曲率に応じた散乱幅
    float scatter = saturate(curvature * 2.0f);

    // 散乱カラー (赤チャンネルが最も遠くまで散乱)
    float3 sssLookup;
    sssLookup.r = smoothstep(0.0f, 1.0f, wrappedNdotL + scatter * 0.3f);
    sssLookup.g = smoothstep(0.0f, 1.0f, wrappedNdotL + scatter * 0.1f);
    sssLookup.b = smoothstep(0.0f, 1.0f, wrappedNdotL);

    return lerp(float3(wrappedNdotL, wrappedNdotL, wrappedNdotL),
                sssLookup * sssColor, sssStrength);
}

/// @brief マイクロ法線によるディテール法線合成
/// @param macroNormal メッシュ法線 (ワールド空間)
/// @param microNormal マイクロ法線マップ値
/// @param scale マイクロ法線の影響スケール
float3 BlendMicroNormal(float3 macroNormal, float3 microNormal, float scale)
{
    // Reoriented Normal Mapping (RNM) 簡易版
    float3 detail = lerp(float3(0, 0, 1), microNormal, scale);
    return normalize(float3(macroNormal.xy + detail.xy, macroNormal.z * detail.z));
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Skinピクセルシェーダー - Pre-Integrated SSS + マイクロ法線 + キャビティ
PSOutput PSMain(PSInput input)
{
    // --- パラメータ取得 ---
    float3 sssColor        = gCustom0.xyz;
    float  sssRadius       = gCustom0.w;
    float  sssStrength     = gCustom1.x;
    float  microNormalScale = gCustom1.y;
    float  cavityStrength  = gCustom1.z;
    float  fresnelOffset   = gCustom1.w;
    float3 specTint        = gCustom2.xyz;

    // --- アルベド ---
    float4 albedo = SampleAlbedo(input.texcoord);

    if (albedo.a < gAlphaCutoff)
        discard;

    // --- 法線 ---
    float3 N = ApplyNormalMap(input);

    // --- マイクロ法線 (AOマップスロットの代替使用、または法線マップの2回目サンプル) ---
    float3 microN = N; // フォールバック
    if (microNormalScale > 0.001f)
    {
        // 法線マップの高周波成分をマイクロ法線として利用
        float3 detailNormal = tNormal.Sample(sLinearWrap, input.texcoord * 4.0f).xyz * 2.0f - 1.0f;
        N = BlendMicroNormal(N, detailNormal, microNormalScale);
    }

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

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- キャビティ (AOマップから推定) ---
    float cavity = lerp(1.0f, ao, cavityStrength);

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);
    float NdotV = max(dot(N, V), 0.0f);

    // --- サーフェス曲率推定 (スクリーン空間法線微分) ---
    float curvature = length(fwidth(N)) * sssRadius;

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- Skin ライティング ---
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

        float NdotL = dot(N, L);
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);
        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // Pre-Integrated SSS ディフューズ
        float3 sssDiffuse = PreIntegratedSkin(NdotL, curvature, sssColor, sssStrength);
        float3 diffuse = albedo.rgb * sssDiffuse * cavity;

        // スペキュラ (Cook-Torrance with specular tint)
        float3 specColor = float3(0, 0, 0);
        if (max(NdotL, 0.0f) > 0.0f)
        {
            float3 H = normalize(V + L);
            float NdotH = max(dot(N, H), 0.0f);
            float D = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);

            // Skin Fresnel (オフセット付き Schlick)
            float VdotH = max(dot(V, H), 0.0f);
            float F = fresnelOffset + (1.0f - fresnelOffset) * pow(1.0f - VdotH, 5.0f);

            float denom = 4.0f * NdotV * max(NdotL, 0.0f) + 0.001f;
            specColor = specTint * D * G * F / denom;
        }

        Lo += (diffuse + specColor) * radiance * shadow;
    }

    // --- アンビエント + IBL ---
    float3 iblContrib = EvaluateIBL(N, V, albedo.rgb, metallic, roughness, ao);
    float3 ambient = iblContrib + gAmbientColor * albedo.rgb * ao * cavity;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color    = float4(finalColor, albedo.a);
    output.normal   = EncodeNormal(N, metallic, roughness);
    output.albedo   = float4(albedo.rgb, 1.0);
    output.velocity = EncodeVelocity(input.currClipPos, input.prevClipPos);
    return output;
}
