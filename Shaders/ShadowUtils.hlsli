// シャドウマップ計算ユーティリティ
// CSM (Cascaded Shadow Maps)、スポットライトシャドウ、ポイントライトシャドウ
// の3種類のシャドウマッピングを5x5 PCFフィルタリングで提供する。

/// @file ShadowUtils.hlsli
/// @brief シャドウマップサンプリング・カスケード選択ユーティリティ

#ifndef SHADOW_UTILS_HLSLI
#define SHADOW_UTILS_HLSLI

#define NUM_CASCADES 4

// シャドウマップテクスチャ（t8-t13）
Texture2D      tShadowMap0  : register(t8);  // CSMカスケード0（最近距離）
Texture2D      tShadowMap1  : register(t9);  // CSMカスケード1
Texture2D      tShadowMap2  : register(t10); // CSMカスケード2
Texture2D      tShadowMap3  : register(t11); // CSMカスケード3（最遠距離）
Texture2D      tSpotShadow  : register(t12); // スポットライトシャドウマップ
Texture2DArray tPointShadow : register(t13); // ポイントライト6面キューブシャドウ

// 比較サンプラー（深度比較付き、PCFのハードウェアフィルタリング用）
SamplerComparisonState sShadowCmp : register(s2);

// PCFサンプリング半径スケール（大きいほどソフトシャドウが広がる）
static const float k_ShadowSoftness = 2.0f;

// ============================================================================
// PCFフィルタリング（5x5カーネル = 25サンプル）
// SampleCmpLevelZeroでハードウェア深度比較を行い、平均して柔らかい影を生成。
// ============================================================================

/// @brief 5x5 PCFシャドウフィルタ — テクセル単位でオフセットして25回比較サンプリング
float SampleShadowMapPCF(Texture2D shadowMap, float3 shadowCoord, float shadowMapSize)
{
    float texelSize = k_ShadowSoftness / shadowMapSize;
    float shadow = 0.0f;

    // 5x5 PCF
    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(sShadowCmp,
                shadowCoord.xy + offset, shadowCoord.z);
        }
    }

    return shadow / 25.0f;
}

// ============================================================================
// Texture2DArray用PCFフィルタリング（ポイントライトのキューブ面に使用）
// ============================================================================

/// @brief 5x5 PCFシャドウフィルタ（配列スライス版）— キューブ面境界マージン付き
float SampleShadowArrayPCF(Texture2DArray shadowArray, float3 shadowCoord,
                             uint arraySlice, float shadowMapSize)
{
    float texelSize = k_ShadowSoftness / shadowMapSize;
    float shadow = 0.0f;

    // 面境界マージン（PCFオフセットがキューブ面UVを超えないようクランプ）
    float margin = 1.5f / shadowMapSize;

    // 5x5 PCF（CSMと同じカーネルサイズ）
    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            float2 uv = clamp(shadowCoord.xy + offset, margin, 1.0f - margin);
            float3 uva = float3(uv, (float)arraySlice);
            shadow += shadowArray.SampleCmpLevelZero(sShadowCmp, uva, shadowCoord.z);
        }
    }

    return shadow / 25.0f;
}

// ============================================================================
// CSMカスケード選択 + シャドウ計算
// ビュー空間Zからカスケードを選択し、ライト空間に変換してPCFサンプリング。
// ============================================================================

/// @brief CSMシャドウファクター — viewZでカスケードを選び、PCFフィルタしたシャドウ値を返す
float ComputeCascadedShadow(float3 posW, float viewZ,
                             float4x4 lightVP[NUM_CASCADES],
                             float cascadeSplits[NUM_CASCADES],
                             float shadowMapSize)
{
    // カスケード選択（ビュー空間Zで判定）
    uint cascadeIndex = 0;
    for (uint i = 0; i < NUM_CASCADES - 1; ++i)
    {
        if (viewZ > cascadeSplits[i])
            cascadeIndex = i + 1;
    }

    // ライト空間座標計算
    float4 shadowPos = mul(float4(posW, 1.0f), lightVP[cascadeIndex]);
    float3 shadowCoord;
    shadowCoord.xy = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
    shadowCoord.y  = 1.0f - shadowCoord.y;  // UV Y反転
    shadowCoord.z  = shadowPos.z / shadowPos.w;

    // 範囲外チェック
    if (shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
        shadowCoord.y < 0.0f || shadowCoord.y > 1.0f ||
        shadowCoord.z < 0.0f || shadowCoord.z > 1.0f)
    {
        return 1.0f; // 影なし
    }

    // カスケードに応じたシャドウマップをサンプル
    float shadow = 1.0f;
    if (cascadeIndex == 0)
        shadow = SampleShadowMapPCF(tShadowMap0, shadowCoord, shadowMapSize);
    else if (cascadeIndex == 1)
        shadow = SampleShadowMapPCF(tShadowMap1, shadowCoord, shadowMapSize);
    else if (cascadeIndex == 2)
        shadow = SampleShadowMapPCF(tShadowMap2, shadowCoord, shadowMapSize);
    else
        shadow = SampleShadowMapPCF(tShadowMap3, shadowCoord, shadowMapSize);

    return shadow;
}

// ============================================================================
// スポットライトシャドウ計算
// ============================================================================

/// @brief スポットライトシャドウ — ライトVP変換してPCFサンプリング
float ComputeSpotShadow(float3 posW, float4x4 spotLightVP, float shadowMapSize)
{
    float4 shadowPos = mul(float4(posW, 1.0f), spotLightVP);
    float3 shadowCoord;
    shadowCoord.xy = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
    shadowCoord.y  = 1.0f - shadowCoord.y;
    shadowCoord.z  = shadowPos.z / shadowPos.w;

    // 範囲外チェック
    if (shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
        shadowCoord.y < 0.0f || shadowCoord.y > 1.0f ||
        shadowCoord.z < 0.0f || shadowCoord.z > 1.0f)
    {
        return 1.0f;
    }

    return SampleShadowMapPCF(tSpotShadow, shadowCoord, shadowMapSize);
}

// ============================================================================
// ポイントライトシャドウ計算（全方向キューブマップ方式）
// ============================================================================

/// @brief ポイントライトシャドウ — 支配軸でキューブ面を選択しPCFサンプリング
float ComputePointShadow(float3 posW, float3 lightPos,
                           float4x4 faceVP[6], float shadowMapSize)
{
    // 支配軸（最大成分の軸）でキューブマップの面を決定
    float3 diff = posW - lightPos;
    float3 absDiff = abs(diff);

    uint faceIndex = 0;
    if (absDiff.x >= absDiff.y && absDiff.x >= absDiff.z)
        faceIndex = (diff.x > 0.0f) ? 0 : 1; // +X or -X
    else if (absDiff.y >= absDiff.x && absDiff.y >= absDiff.z)
        faceIndex = (diff.y > 0.0f) ? 2 : 3; // +Y or -Y
    else
        faceIndex = (diff.z > 0.0f) ? 4 : 5; // +Z or -Z

    // ライト空間座標計算
    float4 shadowPos = mul(float4(posW, 1.0f), faceVP[faceIndex]);
    float3 shadowCoord;
    shadowCoord.xy = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
    shadowCoord.y  = 1.0f - shadowCoord.y;
    shadowCoord.z  = shadowPos.z / shadowPos.w;

    // 範囲外チェック
    if (shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
        shadowCoord.y < 0.0f || shadowCoord.y > 1.0f ||
        shadowCoord.z < 0.0f || shadowCoord.z > 1.0f)
    {
        return 1.0f;
    }

    return SampleShadowArrayPCF(tPointShadow, shadowCoord, faceIndex, shadowMapSize);
}

#endif // SHADOW_UTILS_HLSLI
