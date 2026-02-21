// シェーダーモデル共通定義
// PBR/Unlit/Toon/Phong/Subsurface/ClearCoat の全シェーダーモデルが共有する
// cbuffer、テクスチャバインディング、頂点フォーマット、VSMain、ヘルパー関数を定義。

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
    float4x4 world;                 // ワールド変換行列
    float4x4 worldInverseTranspose; // 法線変換用（ワールド逆転置行列）
};

#define NUM_SHADOW_CASCADES 4

cbuffer FrameConstants : register(b1)
{
    float4x4 gView;                              // ビュー行列
    float4x4 gProjection;                        // プロジェクション行列
    float4x4 gViewProjection;                    // ビュー×プロジェクション行列
    float3   gCameraPosition;                    // カメラのワールド座標
    float    gTime;                              // アプリ開始からの経過秒
    // CSMシャドウ関連
    float4x4 gLightVP[NUM_SHADOW_CASCADES];      // 各カスケードのライトVP行列
    float4   gCascadeSplitsVec;                   // カスケード分割距離 (x,y,z,w)
    float    gShadowMapSize;                      // シャドウマップの解像度（px）
    uint     gShadowEnabled;                      // シャドウ有効フラグ
    float2   _fogPad;
    // フォグ関連
    float3   gFogColor;                           // フォグの色
    float    gFogStart;                           // 線形フォグの開始距離
    float    gFogEnd;                             // 線形フォグの終了距離
    float    gFogDensity;                         // 指数フォグの密度
    uint     gFogMode;                            // 0=無効, 1=Linear, 2=Exp, 3=Exp2
    uint     gShadowDebugMode;                    // シャドウデバッグ可視化モード (0=off)
    // スポットシャドウ関連
    float4x4 gSpotLightVP;                        // スポットライトのVP行列
    float    gSpotShadowMapSize;                  // スポットシャドウマップ解像度
    int      gSpotShadowLightIndex;               // スポットシャドウ対象のライトインデックス
    float2   _spotPad;
    // ポイントシャドウ関連
    float4x4 gPointLightVP[6];                    // ポイントライト6面のVP行列
    float    gPointShadowMapSize;                 // ポイントシャドウマップ解像度
    int      gPointShadowLightIndex;              // ポイントシャドウ対象のライトインデックス
    float2   _pointPad;
};

#define MAX_LIGHTS 16

cbuffer LightConstants : register(b2)
{
    LightData gLights[MAX_LIGHTS]; // ライト配列（最大16個）
    float3    gAmbientColor;       // アンビエントライト色
    uint      gNumLights;          // 有効ライト数
};

// ============================================================================
// ShaderModelConstants (b3) - 256B 統一cbuffer
// ============================================================================

cbuffer ShaderModelConstants : register(b3)
{
    // -- 共通パラメータ (0..63) -- 全シェーダーモデルで使用
    float4   gBaseColor;          //  0: 基本色 (RGBA)
    float3   gEmissiveFactor;     // 16: 自発光色
    float    gEmissiveStrength;   // 28: 自発光の強度
    float    gAlphaCutoff;        // 32: アルファテストの閾値
    uint     gShaderModelId;      // 36: シェーダーモデルID
    uint     gMaterialFlags;      // 40: テクスチャ有無フラグ群
    float    gNormalScale;        // 44: 法線マップの強度
    float    gMetallic;           // 48: メタリック度 (PBR)
    float    gRoughness;          // 52: ラフネス (PBR)
    float    gAOStrength;         // 56: AOマップの効き具合
    float    gReflectance;        // 60: 反射率

    // -- Toonパラメータ (64..143) -- UTS2ダブルシェード方式
    float4   gShadeColor;         // 64:  1stシェード色（半影）
    float4   gShade2ndColor;      // 80:  2ndシェード色（暗部）
    float    gBaseColorStep;      // 96:  Base→1stの閾値
    float    gBaseShadeFeather;   // 100: Base→1stの遷移幅
    float    gShadeColorStep;     // 104: 1st→2ndの閾値
    float    gShade1st2ndFeather; // 108: 1st→2ndの遷移幅
    float4   gRimColor;           // 112: リムライト色 (.a=強度)
    float    gRimPower;           // 128: リムライトの鋭さ
    float    gRimIntensity;       // 132: リムライト全体強度
    float    gHighColorPower;     // 136: スペキュラのパワー値
    float    gHighColorIntensity; // 140: スペキュラの強度

    // -- Phongパラメータ (144..159)
    float3   gSpecularColor;      // 144: スペキュラ反射色
    float    gShininess;          // 156: Phong指数（光沢度）

    // -- Subsurfaceパラメータ (160..191)
    float3   gSubsurfaceColor;    // 160: SSS散乱色
    float    gSubsurfaceRadius;   // 172: 散乱半径（広がり）
    float    gSubsurfaceStrength; // 176: SSS強度
    float    gThickness;          // 180: 厚み（0=薄い→高透過）
    float2   _ssPad;              // 184: パディング

    // -- ClearCoatパラメータ (192..207)
    float    gClearCoatStrength;  // 192: クリアコートの強度
    float    gClearCoatRoughness; // 196: クリアコート層のラフネス
    float2   _ccPad;              // 200: パディング

    // -- Toon拡張パラメータ (208..255)
    float    gOutlineWidth;       // 208: アウトライン幅
    float3   gOutlineColor;       // 212: アウトライン色
    float3   gHighColor;          // 224: ハイカラー（スペキュラ色）
    float    gShadowReceiveLevel; // 236: CSMシャドウの影響度
    float    gRimInsideMask;      // 240: リム内側マスク閾値
    float3   _toonPad2;           // 244: パディング
};

// Toon拡張エイリアス: cbuffer 256B制約のため、Phong/SS/CC領域をToon用に再利用
// ToonとPhong/SS/CCは排他的なので同時使用されない
#define gRimLightDirMask       gSpecularColor.x   // リムのライト方向制限 (0=無制限, 1=ライト側のみ)
#define gRimFeatherOff         gSpecularColor.y   // リムフェザーOFF (0=グラデ, 1=ステップ)
#define gHighColorBlendAdd     gSpecularColor.z   // ハイカラーブレンド (0=乗算, 1=加算)
#define gHighColorOnShadow     gShininess         // 影領域でのハイカラー (0=消す, 1=維持)
#define gOutlineFarDist        gSubsurfaceColor.x // アウトライン遠距離（消え始め）
#define gOutlineNearDist       gSubsurfaceColor.y // アウトライン近距離（最大幅）
#define gOutlineBlendBaseColor gSubsurfaceColor.z // ベース色のアウトラインへの混合率

cbuffer BoneConstants : register(b4)
{
    float4x4 gBones[128]; // スケルタルアニメーション用ボーン行列（最大128本）
};

// ============================================================================
// マテリアルフラグ（C++側 Material.h MaterialFlags と同一ビット定義）
// gMaterialFlags のビットごとに対応テクスチャの有無を示す。
// ============================================================================

#define HAS_ALBEDO_MAP        (1 << 0)  // アルベドテクスチャあり
#define HAS_NORMAL_MAP        (1 << 1)  // 法線マップあり
#define HAS_METROUGH_MAP      (1 << 2)  // メタリック/ラフネスマップあり
#define HAS_AO_MAP            (1 << 3)  // AOマップあり
#define HAS_EMISSIVE_MAP      (1 << 4)  // エミッシブマップあり
#define HAS_TOON_RAMP_MAP     (1 << 5)  // Toonランプテクスチャあり
#define HAS_SUBSURFACE_MAP    (1 << 6)  // サブサーフェス厚みマップあり
#define HAS_CLEARCOAT_MASK_MAP (1 << 7) // クリアコートマスクマップあり

// ============================================================================
// インスタンシング用StructuredBuffer (t14)
// DrawModelInstanced 使用時に INSTANCED マクロが定義される
// ============================================================================

#if defined(INSTANCED)
struct InstanceData
{
    float4x4 instanceWorld;            // ワールド変換行列（転置済み）
    float4x4 instanceWorldInvTranspose; // 法線変換用逆転置行列（転置済み）
};
StructuredBuffer<InstanceData> gInstanceData : register(t14);
#endif

// ============================================================================
// テクスチャバインディング (t0-t7)
// t8-t13, s2 は ShadowUtils.hlsli で宣言済み
// ============================================================================

Texture2D    tAlbedo       : register(t0);  // アルベドテクスチャ
Texture2D    tNormal       : register(t1);  // 法線マップ
Texture2D    tMetRough     : register(t2);  // メタリック(B)/ラフネス(G) (glTF規約)
Texture2D    tAO           : register(t3);  // アンビエントオクルージョン
Texture2D    tEmissive     : register(t4);  // エミッシブマップ
Texture2D    tToonRamp     : register(t5);  // Toonランプテクスチャ
Texture2D    tSubsurface   : register(t6);  // SSS厚みマップ (R)
Texture2D    tClearCoatMask : register(t7); // クリアコートマスク (R)
SamplerState sLinearWrap   : register(s0);  // リニアラップサンプラー

// ============================================================================
// 頂点フォーマット
// ============================================================================

struct VSInput
{
    float3 position : POSITION;  // ローカル座標
    float3 normal   : NORMAL;    // ローカル法線
    float2 texcoord : TEXCOORD;  // UV座標
    float4 tangent  : TANGENT;   // 接線 (.w=従接線の符号)
#if defined(SKINNED)
    uint4  joints   : JOINTS;    // ボーンインデックス (最大4本)
    float4 weights  : WEIGHTS;   // ボーンウェイト (合計=1.0)
#endif
};

struct PSInput
{
    float4 posH     : SV_Position; // クリップ空間座標
    float3 posW     : POSITION;    // ワールド座標
    float3 normalW  : NORMAL;      // ワールド法線
    float2 texcoord : TEXCOORD;    // UV座標
    float3 tangentW : TANGENT;     // ワールド接線
    float  bitangentSign : BTSIGN; // 従接線の符号 (tangent.w)
    float  viewZ    : VIEWZ;      // ビュー空間Z (CSMカスケード選択用)
};

// ============================================================================
// PSOutput（3 MRT: color, normal, albedo）
// ============================================================================

struct PSOutput
{
    float4 color  : SV_Target0; // HDRライティング結果
    float4 normal : SV_Target1; // エンコード済み法線 + メタリック/ラフネス (SSR/RTGI用)
    float4 albedo : SV_Target2; // ライティング前のアルベド (RTGI用)
};

// ============================================================================
// 頂点シェーダー
// ============================================================================

/// @brief 共通頂点シェーダー — スキニング・ワールド変換・ビュー空間Z計算を行う
/// インスタンシング時は SV_InstanceID でインスタンスごとのワールド行列を取得する
PSInput VSMain(VSInput input
#if defined(INSTANCED)
    , uint instanceID : SV_InstanceID
#endif
)
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

    // ワールド変換行列を選択（インスタンシング時はインスタンスデータから取得）
#if defined(INSTANCED)
    float4x4 worldMat = gInstanceData[instanceID].instanceWorld;
    float4x4 worldInvT = gInstanceData[instanceID].instanceWorldInvTranspose;
#else
    float4x4 worldMat = world;
    float4x4 worldInvT = worldInverseTranspose;
#endif

    float4 posW = mul(posL, worldMat);
    output.posW = posW.xyz;
    output.posH = mul(posW, gViewProjection);

    float3x3 wit = (float3x3)worldInvT;
    output.normalW = normalize(mul(normalL, wit));
    output.tangentW = normalize(mul(tangentL, (float3x3)worldMat));
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

/// シャドウファクターを保持する構造体
struct ShadowInfo
{
    float cascadeShadow; // CSMシャドウ値 (0=完全な影, 1=影なし)
};

/// @brief ディレクショナルライトのCSMシャドウを計算する
/// 法線オフセットバイアスを適用してからCSMサンプリングを行う
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
/// ディレクショナルはCSM、スポット/ポイントは専用シャドウマップから取得
float GetLightShadow(uint lightIndex, float cascadeShadow, float3 posW, float3 N)
{
    // ライト種別に応じた方向で法線オフセットバイアスを計算
    float3 lightDir;
    if (gLights[lightIndex].type == LIGHT_DIRECTIONAL)
        lightDir = normalize(gLights[lightIndex].direction);
    else
        lightDir = normalize(posW - gLights[lightIndex].position);

    float NdotL = dot(N, -lightDir);
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
