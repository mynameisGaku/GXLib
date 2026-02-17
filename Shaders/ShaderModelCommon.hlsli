/// @file ShaderModelCommon.hlsli
/// @brief ShaderModel共通定義（cbuffer/テクスチャ/頂点フォーマット/VSMain）
///
/// 全シェーダーモデル（Unlit/Toon/Phong/Subsurface/ClearCoat）で共有。
/// PBR.hlslと同一のルートシグネチャ・頂点フォーマットを使用する。

#ifndef SHADER_MODEL_COMMON_HLSLI
#define SHADER_MODEL_COMMON_HLSLI

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

// ============================================================================
// ShaderModelConstants (b3) - 256B 統一cbuffer
// ============================================================================

cbuffer ShaderModelConstants : register(b3)
{
    // Common (0..63)
    float4   gBaseColor;          //  0
    float3   gEmissiveFactor;     // 16
    float    gEmissiveStrength;   // 28
    float    gAlphaCutoff;        // 32
    uint     gShaderModelId;      // 36
    uint     gMaterialFlags;      // 40
    float    gNormalScale;        // 44
    float    gMetallic;           // 48
    float    gRoughness;          // 52
    float    gAOStrength;         // 56
    float    gReflectance;        // 60

    // Toon (64..143)
    float4   gShadeColor;         // 64
    float    gOutlineWidth;       // 80
    float3   gOutlineColor;       // 84
    float    gRampThreshold;      // 96
    float    gRampSmoothing;      // 100
    uint     gRampBands;          // 104
    float    _toonPad;            // 108
    float4   gRimColor;           // 112
    float    gRimPower;           // 128
    float    gRimIntensity;       // 132
    float2   _toonPad2;           // 136

    // Phong (144..159)
    float3   gSpecularColor;      // 144
    float    gShininess;          // 156

    // Subsurface (160..191)
    float3   gSubsurfaceColor;    // 160
    float    gSubsurfaceRadius;   // 172
    float    gSubsurfaceStrength; // 176
    float    gThickness;          // 180
    float2   _ssPad;              // 184

    // ClearCoat (192..207)
    float    gClearCoatStrength;  // 192
    float    gClearCoatRoughness; // 196
    float2   _ccPad;              // 200

    // Reserved (208..255)
    float    _reserved[12];       // 208
};

cbuffer BoneConstants : register(b4)
{
    float4x4 gBones[128];
};

// ============================================================================
// マテリアルフラグ（Material.h MaterialFlags と一致）
// ============================================================================

#define HAS_ALBEDO_MAP        (1 << 0)
#define HAS_NORMAL_MAP        (1 << 1)
#define HAS_METROUGH_MAP      (1 << 2)
#define HAS_AO_MAP            (1 << 3)
#define HAS_EMISSIVE_MAP      (1 << 4)
#define HAS_TOON_RAMP_MAP     (1 << 5)
#define HAS_SUBSURFACE_MAP    (1 << 6)
#define HAS_CLEARCOAT_MASK_MAP (1 << 7)

// ============================================================================
// テクスチャ
// ============================================================================

Texture2D    tAlbedo       : register(t0);
Texture2D    tNormal       : register(t1);
Texture2D    tMetRough     : register(t2);
Texture2D    tAO           : register(t3);
Texture2D    tEmissive     : register(t4);
Texture2D    tToonRamp     : register(t5);
Texture2D    tSubsurface   : register(t6);
Texture2D    tClearCoatMask : register(t7);
SamplerState sLinearWrap   : register(s0);

// t8-t13, s2 は ShadowUtils.hlsli で宣言済み

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
// PSOutput（3 MRT: color, normal, albedo）
// ============================================================================

struct PSOutput
{
    float4 color  : SV_Target0;
    float4 normal : SV_Target1;
    float4 albedo : SV_Target2;  // GI用: ライティング前のアルベド
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
// ヘルパー関数
// ============================================================================

/// @brief 法線マップを適用して最終法線を計算
float3 ApplyNormalMap(PSInput input)
{
    float3 N = normalize(input.normalW);
    if (gMaterialFlags & HAS_NORMAL_MAP)
    {
        float3 T = normalize(input.tangentW);
        float3 B = cross(N, T) * input.bitangentSign;
        float3x3 TBN = float3x3(T, B, N);
        float3 normalMap = tNormal.Sample(sLinearWrap, input.texcoord).rgb * 2.0f - 1.0f;
        normalMap.xy *= gNormalScale;
        N = normalize(mul(normalMap, TBN));
    }
    return N;
}

/// @brief アルベドを取得（baseColor * テクスチャ）
float4 SampleAlbedo(float2 uv)
{
    float4 albedo = gBaseColor;
    if (gMaterialFlags & HAS_ALBEDO_MAP)
    {
        albedo *= tAlbedo.Sample(sLinearWrap, uv);
    }
    return albedo;
}

/// @brief AOを取得
float SampleAO(float2 uv)
{
    float ao = 1.0f;
    if (gMaterialFlags & HAS_AO_MAP)
    {
        ao = tAO.Sample(sLinearWrap, uv).r;
        ao = lerp(1.0f, ao, gAOStrength);
    }
    return ao;
}

/// @brief エミッシブを取得
float3 SampleEmissive(float2 uv)
{
    float3 emissive = float3(0, 0, 0);
    if (gMaterialFlags & HAS_EMISSIVE_MAP)
    {
        emissive = tEmissive.Sample(sLinearWrap, uv).rgb * gEmissiveFactor * gEmissiveStrength;
    }
    else if (gEmissiveStrength > 0.0f)
    {
        emissive = gEmissiveFactor * gEmissiveStrength;
    }
    return emissive;
}

/// @brief フォグを適用
float3 ApplyFog(float3 color, float3 posW)
{
    if (gFogMode > 0)
    {
        float dist = length(posW - gCameraPosition);
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

        color = lerp(gFogColor, color, fogFactor);
    }
    return color;
}

/// @brief 法線エンコード（MRT出力用、[0,1]にマップ + メタリック/ラフネスパック）
float4 EncodeNormal(float3 N, float metallic, float roughness)
{
    float packedMR = clamp(roughness, 0.01, 0.99) + (metallic > 0.5 ? 1.0 : 0.0);
    return float4(N * 0.5f + 0.5f, packedMR);
}

/// @brief シャドウファクターを全ライト分計算するヘルパー構造体
struct ShadowInfo
{
    float cascadeShadow;
};

/// @brief メインディレクショナルライトのシャドウファクターを計算
ShadowInfo ComputeShadowAll(float3 posW, float3 N, float viewZ)
{
    ShadowInfo info;
    info.cascadeShadow = 1.0f;

    // メインライト方向取得
    float3 mainLightDir = float3(0, -1, 0);
    for (uint li = 0; li < gNumLights; ++li)
    {
        if (gLights[li].type == LIGHT_DIRECTIONAL)
        {
            mainLightDir = normalize(gLights[li].direction);
            break;
        }
    }

    // 法線オフセットバイアス
    float NdotL = dot(N, -mainLightDir);
    float normalOffsetScale = saturate(1.0f - NdotL);
    static const float k_ShadowNormalBias = 0.05f;
    float3 shadowPosW = posW + N * normalOffsetScale * k_ShadowNormalBias;

    if (gShadowEnabled)
    {
        float cascadeSplits[NUM_SHADOW_CASCADES] = {
            gCascadeSplitsVec.x, gCascadeSplitsVec.y,
            gCascadeSplitsVec.z, gCascadeSplitsVec.w
        };
        info.cascadeShadow = ComputeCascadedShadow(
            shadowPosW, viewZ,
            gLightVP, cascadeSplits, gShadowMapSize);
    }

    return info;
}

/// @brief 個別ライトのシャドウファクターを取得
float GetLightShadow(uint lightIndex, float cascadeShadow, float3 posW, float3 N)
{
    // 法線オフセットバイアス
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
    static const float k_ShadowNormalBias = 0.05f;
    float3 shadowPosW = posW + N * normalOffsetScale * k_ShadowNormalBias;

    if (gLights[lightIndex].type == LIGHT_DIRECTIONAL)
    {
        return cascadeShadow;
    }
    else if ((int)lightIndex == gSpotShadowLightIndex)
    {
        return ComputeSpotShadow(shadowPosW, gSpotLightVP, gSpotShadowMapSize);
    }
    else if ((int)lightIndex == gPointShadowLightIndex)
    {
        return ComputePointShadow(shadowPosW, gLights[lightIndex].position,
                                   gPointLightVP, gPointShadowMapSize);
    }
    return 1.0f;
}

#endif // SHADER_MODEL_COMMON_HLSLI
