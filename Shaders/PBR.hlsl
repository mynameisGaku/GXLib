/// @file PBR.hlsl
/// @brief 3D PBRシェーダー（Cook-Torrance BRDF + ライティング + CSMシャドウ）

#include "LightingUtils.hlsli"
#include "ShadowUtils.hlsli"

// ============================================================================
// 定数バッファ
// ============================================================================

cbuffer ObjectConstants : register(b0)
{
    float4x4 world;
    float4x4 worldInverseTranspose;
};

#define NUM_SHADOW_CASCADES 4

cbuffer FrameConstants : register(b1)
{
    float4x4 gView;
    float4x4 gProjection;
    float4x4 gViewProjection;
    float3   gCameraPosition;
    float    gTime;
    // CSMシャドウ関連
    float4x4 gLightVP[NUM_SHADOW_CASCADES];
    float4   gCascadeSplitsVec;   // x,y,z,w = splits[0..3]
    float    gShadowMapSize;
    uint     gShadowEnabled;
    float2   _fogPad;
    // フォグ関連
    float3   gFogColor;
    float    gFogStart;
    float    gFogEnd;
    float    gFogDensity;
    uint     gFogMode;
    uint     gShadowDebugMode;
    // スポットシャドウ関連
    float4x4 gSpotLightVP;
    float    gSpotShadowMapSize;
    int      gSpotShadowLightIndex;
    float2   _spotPad;
    // ポイントシャドウ関連
    float4x4 gPointLightVP[6];
    float    gPointShadowMapSize;
    int      gPointShadowLightIndex;
    float2   _pointPad;
};

#define MAX_LIGHTS 16

cbuffer LightConstants : register(b2)
{
    LightData gLights[MAX_LIGHTS];
    float3    gAmbientColor;
    uint      gNumLights;
};

cbuffer MaterialConstants : register(b3)
{
    float4 gAlbedoFactor;
    float  gMetallicFactor;
    float  gRoughnessFactor;
    float  gAOStrength;
    float  gEmissiveStrength;
    float3 gEmissiveFactor;
    uint   gMaterialFlags;
};

cbuffer BoneConstants : register(b4)
{
    float4x4 gBones[128];
};

// ============================================================================
// テクスチャ
// ============================================================================

Texture2D    tAlbedo   : register(t0);
Texture2D    tNormal   : register(t1);
Texture2D    tMetRough : register(t2);
Texture2D    tAO       : register(t3);
Texture2D    tEmissive : register(t4);
SamplerState sLinearWrap : register(s0);

// t8-t11, s2 はShadowUtils.hlsliで宣言済み

// マテリアルフラグ
#define HAS_ALBEDO_MAP    (1 << 0)
#define HAS_NORMAL_MAP    (1 << 1)
#define HAS_METROUGH_MAP  (1 << 2)
#define HAS_AO_MAP        (1 << 3)
#define HAS_EMISSIVE_MAP  (1 << 4)

// ============================================================================
// 頂点フォーマット
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 tangent  : TANGENT;
#if defined(SKINNED)
    uint4  joints   : JOINTS;
    float4 weights  : WEIGHTS;
#endif
};

struct PSInput
{
    float4 posH     : SV_Position;
    float3 posW     : POSITION;
    float3 normalW  : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 tangentW : TANGENT;
    float  bitangentSign : BTSIGN;
    float  viewZ    : VIEWZ;  // ビュー空間Z（カスケード選択用）
};

// ============================================================================
// 頂点シェーダー
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 posL = float4(input.position, 1.0f);
    float3 normalL = input.normal;
    float3 tangentL = input.tangent.xyz;

#if defined(SKINNED)
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);
    float3 skinnedTangent = float3(0, 0, 0);

    float4 w = input.weights;
    float wSum = w.x + w.y + w.z + w.w;
    if (wSum > 0.0001f) w /= wSum;
    else w = float4(1, 0, 0, 0);

    uint4 j = input.joints;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float4x4 bone = gBones[j[i]];
        skinnedPos += mul(posL, bone) * w[i];
        skinnedNormal += mul(float4(normalL, 0.0f), bone).xyz * w[i];
        skinnedTangent += mul(float4(tangentL, 0.0f), bone).xyz * w[i];
    }
    posL = skinnedPos;
    normalL = skinnedNormal;
    tangentL = skinnedTangent;
#endif

    float4 posW = mul(posL, world);
    output.posW = posW.xyz;
    output.posH = mul(posW, gViewProjection);

    float3x3 wit = (float3x3)worldInverseTranspose;
    output.normalW = normalize(mul(normalL, wit));
    output.tangentW = normalize(mul(tangentL, (float3x3)world));
    output.bitangentSign = input.tangent.w;

    output.texcoord = input.texcoord;

    // ビュー空間Z計算（カスケード選択用）
    float4 posV = mul(posW, gView);
    output.viewZ = posV.z;

    return output;
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================

float4 PSMain(PSInput input) : SV_Target
{
    // --- アルベド ---
    float4 albedo = gAlbedoFactor;
    if (gMaterialFlags & HAS_ALBEDO_MAP)
    {
        albedo *= tAlbedo.Sample(sLinearWrap, input.texcoord);
    }

    // --- 法線 ---
    float3 N = normalize(input.normalW);
    if (gMaterialFlags & HAS_NORMAL_MAP)
    {
        float3 T = normalize(input.tangentW);
        float3 B = cross(N, T) * input.bitangentSign;
        float3x3 TBN = float3x3(T, B, N);
        float3 normalMap = tNormal.Sample(sLinearWrap, input.texcoord).rgb * 2.0f - 1.0f;
        N = normalize(mul(normalMap, TBN));
    }

    // --- メタリック・ラフネス ---
    float metallic  = gMetallicFactor;
    float roughness = gRoughnessFactor;
    if (gMaterialFlags & HAS_METROUGH_MAP)
    {
        float4 mr = tMetRough.Sample(sLinearWrap, input.texcoord);
        metallic  *= mr.b;  // glTF convention: B=metallic
        roughness *= mr.g;  // glTF convention: G=roughness
    }
    roughness = max(roughness, 0.04f);

    // --- AO ---
    float ao = 1.0f;
    if (gMaterialFlags & HAS_AO_MAP)
    {
        ao = tAO.Sample(sLinearWrap, input.texcoord).r;
        ao = lerp(1.0f, ao, gAOStrength);
    }

    // --- View方向 ---
    float3 V = normalize(gCameraPosition - input.posW);

    // --- シャドウ ---
    // ライト方向依存の法線オフセットバイアス
    // 光に正対する面（床）→ オフセット0、光に平行な面（壁）→ オフセット大
    float3 mainLightDir = float3(0, -1, 0);
    for (uint li = 0; li < gNumLights; ++li)
    {
        if (gLights[li].type == LIGHT_DIRECTIONAL)
        {
            mainLightDir = normalize(gLights[li].direction);
            break;
        }
    }
    float NdotL = dot(N, -mainLightDir);
    float normalOffsetScale = saturate(1.0f - NdotL);
    float3 shadowPosW = input.posW + N * normalOffsetScale * 0.05f;

    float shadowFactor = 1.0f;
    if (gShadowEnabled)
    {
        float cascadeSplits[NUM_SHADOW_CASCADES] = {
            gCascadeSplitsVec.x, gCascadeSplitsVec.y,
            gCascadeSplitsVec.z, gCascadeSplitsVec.w
        };
        shadowFactor = ComputeCascadedShadow(
            shadowPosW, input.viewZ,
            gLightVP, cascadeSplits, gShadowMapSize);
    }

    // --- ライティング ---
    float3 Lo = float3(0, 0, 0);
    for (uint i = 0; i < gNumLights; ++i)
    {
        float3 contribution = EvaluateLight(gLights[i], input.posW, N, V,
                            albedo.rgb, metallic, roughness);

        // per-light shadow
        float shadow = 1.0f;
        if (gLights[i].type == LIGHT_DIRECTIONAL)
        {
            shadow = shadowFactor; // CSM
        }
        else if ((int)i == gSpotShadowLightIndex)
        {
            shadow = ComputeSpotShadow(shadowPosW, gSpotLightVP, gSpotShadowMapSize);
        }
        else if ((int)i == gPointShadowLightIndex)
        {
            shadow = ComputePointShadow(shadowPosW, gLights[i].position,
                                         gPointLightVP, gPointShadowMapSize);
        }
        contribution *= shadow;

        Lo += contribution;
    }

    // アンビエント
    float3 ambient = gAmbientColor * albedo.rgb * ao;

    // エミッシブ
    float3 emissive = float3(0, 0, 0);
    if (gMaterialFlags & HAS_EMISSIVE_MAP)
    {
        emissive = tEmissive.Sample(sLinearWrap, input.texcoord).rgb * gEmissiveFactor * gEmissiveStrength;
    }
    else if (gEmissiveStrength > 0.0f)
    {
        emissive = gEmissiveFactor * gEmissiveStrength;
    }

    float3 finalColor = ambient + Lo + emissive;

    // --- フォグ ---
    if (gFogMode > 0)
    {
        float dist = length(input.posW - gCameraPosition);
        float fogFactor = 0.0f;

        if (gFogMode == 1) // Linear
        {
            fogFactor = saturate((gFogEnd - dist) / (gFogEnd - gFogStart));
        }
        else if (gFogMode == 2) // Exp
        {
            fogFactor = exp(-gFogDensity * dist);
        }
        else // Exp2
        {
            float d = gFogDensity * dist;
            fogFactor = exp(-d * d);
        }

        finalColor = lerp(gFogColor, finalColor, fogFactor);
    }

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

        if (gShadowDebugMode == 1)
        {
            // Mode 1: shadowFactor グレースケール表示
            return float4(shadowFactor, shadowFactor, shadowFactor, 1.0f);
        }
        else if (gShadowDebugMode == 2)
        {
            // Mode 2: カスケードインデックス色分け × shadowFactor
            float3 cascadeColors[4] = {
                float3(1, 0, 0),  // Red
                float3(0, 1, 0),  // Green
                float3(0, 0, 1),  // Blue
                float3(1, 1, 0)   // Yellow
            };
            return float4(cascadeColors[ci] * shadowFactor, 1.0f);
        }
        else if (gShadowDebugMode == 3)
        {
            // Mode 3: シャドウ座標可視化（R=u, G=v, B=z）
            // [0,1]範囲外ならマゼンタ表示
            bool outOfRange = shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
                              shadowCoord.y < 0.0f || shadowCoord.y > 1.0f ||
                              shadowCoord.z < 0.0f || shadowCoord.z > 1.0f;
            if (outOfRange)
                return float4(1.0f, 0.0f, 1.0f, 1.0f); // マゼンタ = 範囲外
            return float4(shadowCoord.x, shadowCoord.y, shadowCoord.z, 1.0f);
        }
        else if (gShadowDebugMode == 4)
        {
            // Mode 4: シャドウマップ生深度（比較サンプラーバイパス）
            // 全白(1.0) = マップ空、パターンあり = マップ有効
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
            return float4(rawDepth, rawDepth, rawDepth, 1.0f);
        }
        else if (gShadowDebugMode == 5)
        {
            // Mode 5: ワールド法線可視化（N * 0.5 + 0.5）
            return float4(N * 0.5f + 0.5f, 1.0f);
        }
        else if (gShadowDebugMode == 6)
        {
            // Mode 6: ビュー深度可視化（0-100範囲をリニアマッピング）
            float normalizedZ = saturate(input.viewZ / 100.0f);
            return float4(normalizedZ, normalizedZ, normalizedZ, 1.0f);
        }
        else if (gShadowDebugMode == 7)
        {
            // Mode 7: アルベド可視化（ライティング無し）
            return float4(albedo.rgb, 1.0f);
        }
        else if (gShadowDebugMode == 8)
        {
            // Mode 8: ライティング可視化（Lo のみ、アンビエント/フォグ無し）
            return float4(Lo, 1.0f);
        }
        else if (gShadowDebugMode == 9)
        {
            // Mode 9: ライト[0]の生カラー表示（GPUが受け取った値の確認）
            return float4(gLights[0].color, 1.0f);
        }
    }

    // HDRリニア値をそのまま出力（トーンマッピングはポストエフェクトで実行）
    return float4(finalColor, albedo.a);
}
