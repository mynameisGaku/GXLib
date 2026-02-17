/// @file PBR.hlsl
/// @brief 3D PBRシェーダー（Cook-Torrance BRDF + ライティング + CSMシャドウ）

#include "ShaderModelCommon.hlsli"

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
        metallic  *= mr.b;  // glTF convention: B=metallic
        roughness *= mr.g;  // glTF convention: G=roughness
    }
    roughness = max(roughness, 0.04f);

    // スペキュラAA: ポリゴンエッジで法線のスクリーン空間微分が大きい → roughness増加
    // (Tokuyoshi & Kaplanyan 2019 簡易版)
    {
        float3 dNdx = ddx(N);
        float3 dNdy = ddy(N);
        float normalVariance = dot(dNdx, dNdx) + dot(dNdy, dNdy);
        float kernelRoughness = min(2.0 * normalVariance, 0.25);
        roughness = sqrt(roughness * roughness + kernelRoughness);
    }

    // --- AO ---
    float ao = SampleAO(input.texcoord);

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);

    // --- ライティング ---
    float3 Lo = float3(0, 0, 0);
    for (uint i = 0; i < gNumLights; ++i)
    {
        float3 contribution = EvaluateLight(gLights[i], input.posW, N, V,
                            albedo.rgb, metallic, roughness);

        // per-light shadow
        float shadow = GetLightShadow(i, shadowInfo.cascadeShadow, input.posW, N);
        contribution *= shadow;

        Lo += contribution;
    }

    // アンビエント
    float3 ambient = gAmbientColor * albedo.rgb * ao;

    // エミッシブ
    float3 emissive = SampleEmissive(input.texcoord);

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    float4 encodedNormal = EncodeNormal(N, metallic, roughness);

    // シャドウデバッグ可視化
    if (gShadowDebugMode >= 1)
    {
        // カスケード選択（複数モードで共通使用）
        float cascadeSplits[NUM_SHADOW_CASCADES] = {
            gCascadeSplitsVec.x, gCascadeSplitsVec.y,
            gCascadeSplitsVec.z, gCascadeSplitsVec.w
        };
        uint ci = 0;
        for (uint j = 0; j < NUM_SHADOW_CASCADES - 1; ++j)
        {
            if (input.viewZ > cascadeSplits[j])
                ci = j + 1;
        }

        // ライト空間座標（Mode 3, 4で使用）
        float4 shadowPos = mul(float4(input.posW, 1.0f), gLightVP[ci]);
        float3 shadowCoord;
        shadowCoord.xy = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
        shadowCoord.y  = 1.0f - shadowCoord.y;
        shadowCoord.z  = shadowPos.z / shadowPos.w;

        PSOutput dbg;
        dbg.normal = encodedNormal;
        dbg.albedo = float4(albedo.rgb, 1.0);

        if (gShadowDebugMode == 1)
        {
            dbg.color = float4(shadowInfo.cascadeShadow, shadowInfo.cascadeShadow, shadowInfo.cascadeShadow, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 2)
        {
            float3 cascadeColors[4] = {
                float3(1, 0, 0),
                float3(0, 1, 0),
                float3(0, 0, 1),
                float3(1, 1, 0)
            };
            dbg.color = float4(cascadeColors[ci] * shadowInfo.cascadeShadow, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 3)
        {
            bool outOfRange = shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
                              shadowCoord.y < 0.0f || shadowCoord.y > 1.0f ||
                              shadowCoord.z < 0.0f || shadowCoord.z > 1.0f;
            if (outOfRange)
                dbg.color = float4(1.0f, 0.0f, 1.0f, 1.0f);
            else
                dbg.color = float4(shadowCoord.x, shadowCoord.y, shadowCoord.z, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 4)
        {
            float rawDepth = 1.0f;
            int3 texCoord = int3(
                (int)(shadowCoord.x * gShadowMapSize),
                (int)(shadowCoord.y * gShadowMapSize),
                0);
            if (ci == 0)
                rawDepth = tShadowMap0.Load(texCoord).r;
            else if (ci == 1)
                rawDepth = tShadowMap1.Load(texCoord).r;
            else if (ci == 2)
                rawDepth = tShadowMap2.Load(texCoord).r;
            else
                rawDepth = tShadowMap3.Load(texCoord).r;
            dbg.color = float4(rawDepth, rawDepth, rawDepth, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 5)
        {
            dbg.color = float4(N * 0.5f + 0.5f, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 6)
        {
            float normalizedZ = saturate(input.viewZ / 100.0f);
            dbg.color = float4(normalizedZ, normalizedZ, normalizedZ, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 7)
        {
            dbg.color = float4(albedo.rgb, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 8)
        {
            dbg.color = float4(Lo, 1.0f);
            return dbg;
        }
        else if (gShadowDebugMode == 9)
        {
            dbg.color = float4(gLights[0].color, 1.0f);
            return dbg;
        }
    }

    // HDRリニア値をそのまま出力（トーンマッピングはポストエフェクトで実行）
    PSOutput output;
    output.color  = float4(finalColor, albedo.a);
    output.normal = encodedNormal;
    output.albedo = float4(albedo.rgb, 1.0);
    return output;
}
