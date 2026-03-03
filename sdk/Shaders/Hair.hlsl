/// @file Hair.hlsl
/// @brief Marschner Hair/Fur シェーダー
///
/// Marschner(2003)ベースの異方性反射モデルで髪・毛皮を描画する。
/// R (一次反射)、TT (透過-透過)、TRT (透過-内部反射-透過) の3成分を
/// 接線方向ベースで計算し、リアルな髪のハイライトを実現する。
/// フローマップによるストランド方向制御、根元-先端のカラーブレンドに対応。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// Hair用定数
// ============================================================================

// ShaderModelGPUParamsのaliased region (offset 208-255) にHairパラメータを配置
// gSpecularShift1  = gCustom0.x (offset 208)
// gSpecularShift2  = gCustom0.y (offset 212)
// gSpecularExp1    = gCustom0.z (offset 216)
// gSpecularExp2    = gCustom0.w (offset 220)
// gRootColor       = gCustom1.xyz (offset 224-232, padding at 236)
// gRootToTipBlend  = gCustom1.w (offset 236)
// gTipColor        = gCustom2.xyz (offset 240-248, padding at 252)
// gAlphaCutoffHair = gCustom2.w (offset 252)

// ============================================================================
// Hair BRDF ヘルパー関数
// ============================================================================

/// @brief Kajiya-Kay異方性スペキュラハイライト
/// @param T 接線方向 (ストランド方向)
/// @param H ハーフベクトル
/// @param shift 接線シフト量
/// @param exponent スペキュラ指数
float StrandSpecular(float3 T, float3 H, float shift, float exponent)
{
    // 接線ベースのスペキュラ: sin(T, H)をコサインに変換してpow
    float TdotH = dot(T, H);
    float sinTH = sqrt(max(0.0f, 1.0f - TdotH * TdotH));
    return pow(saturate(sinTH), exponent);
}

/// @brief 接線方向にシフトを適用
/// @param T 元の接線
/// @param N 法線
/// @param shift シフト量（正=法線側、負=反法線側）
float3 ShiftTangent(float3 T, float3 N, float shift)
{
    return normalize(T + N * shift);
}

/// @brief 根元-先端のカラーブレンド
/// @param rootColor 根元カラー
/// @param tipColor 先端カラー
/// @param blend ブレンド係数 (0=根元, 1=先端)
/// @param texcoord テクスチャUV (V軸をストランド方向として使用)
float3 BlendRootToTip(float3 rootColor, float3 tipColor, float blend, float v)
{
    float t = saturate(v * blend + (1.0f - blend) * 0.5f);
    return lerp(rootColor, tipColor, t);
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================

/// @brief Hairピクセルシェーダー — Marschnerベース異方性反射 + 根元先端カラー
PSOutput PSMain(PSInput input)
{
    // --- アルベド ---
    float4 albedo = SampleAlbedo(input.texcoord);

    // アルファカットオフ (Hair用: Dithered alpha for strand AA)
    float alphaCutoff = gCustom2.w; // gAlphaCutoffHair
    if (alphaCutoff <= 0.0f) alphaCutoff = 0.5f;
    if (albedo.a < alphaCutoff)
        discard;

    // --- 法線 ---
    float3 N = ApplyNormalMap(input);

    // --- 接線方向 ---
    // input.tangent.xyzがワールド空間接線方向 (ストランドの伸びる方向)
    float3 T = normalize(input.tangent.xyz);

    // フローマップが有効な場合、接線方向を上書き
    if (gMaterialFlags & (1u << 9)) // HAS_HAIR_FLOW_MAP
    {
        // tSubsurface テクスチャスロットをフローマップとして流用
        float3 flowDir = tSubsurface.Sample(sLinearWrap, input.texcoord).rgb * 2.0f - 1.0f;
        // フローマップの方向を接線空間からワールド空間に変換
        float3 B = cross(N, T);
        T = normalize(flowDir.x * T + flowDir.y * B + flowDir.z * N);
    }

    // --- Hair パラメータ取得 ---
    float specShift1    = gCustom0.x;    // 一次ハイライトシフト
    float specShift2    = gCustom0.y;    // 二次ハイライトシフト
    float specExp1      = gCustom0.z;    // 一次スペキュラ指数
    float specExp2      = gCustom0.w;    // 二次スペキュラ指数
    float3 rootColor    = gCustom1.xyz;  // 根元カラー
    float rootToTipBlend = gCustom1.w;   // 根元-先端ブレンド
    float3 tipColor     = gCustom2.xyz;  // 先端カラー

    // 根元-先端のカラーブレンド
    float3 hairColor = BlendRootToTip(rootColor, tipColor, rootToTipBlend, input.texcoord.y);
    // アルベドで変調
    hairColor *= albedo.rgb;

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- Hair ライティング ---
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

        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0f);

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);

        float3 radiance = gLights[i].color * gLights[i].intensity * attenuation;

        // シフトされた接線によるデュアルスペキュラ
        float3 T1 = ShiftTangent(T, N, specShift1);
        float3 T2 = ShiftTangent(T, N, specShift2);

        // R 成分 (一次反射) - 鋭いハイライト
        float spec1 = StrandSpecular(T1, H, specShift1, specExp1);

        // TRT 成分 (透過-内部反射-透過) - 広い柔らかいハイライト
        float spec2 = StrandSpecular(T2, H, specShift2, specExp2);

        // ディフューズ (ラッピング付き Lambert)
        float wrapNdotL = saturate((NdotL + 0.5f) / 2.25f);
        float3 diffuse = hairColor * wrapNdotL;

        // 合成
        float3 specular = float3(1, 1, 1) * spec1 + hairColor * spec2 * 0.5f;
        Lo += (diffuse + specular) * radiance * shadow;
    }

    // --- アンビエント ---
    float3 ambient = gAmbientColor * hairColor * ao;

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color    = float4(finalColor, albedo.a);
    output.normal   = EncodeNormal(N, 0.0f, 1.0f);
    output.albedo   = float4(hairColor, 1.0);
    output.velocity = EncodeVelocity(input.currClipPos, input.prevClipPos);
    return output;
}
