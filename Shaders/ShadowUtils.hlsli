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

// ============================================================================
// 通常サンプラー（PCSSブロッカーサーチ用 — 深度値の直接読み取り）
// ============================================================================
SamplerState sShadowPointClamp : register(s3);

// ============================================================================
// PCF ヘルパー（カスケードインデックス指定版）
// posW からライト空間座標を計算してカスケードテクスチャをPCFサンプルする
// ============================================================================

/// @brief カスケードインデックス指定PCF — ワールド座標をライト空間に変換してPCFサンプル
float SampleCascadePCF(float3 posW, float4x4 lightVP, float shadowMapSize, int cascadeIdx)
{
    float4 shadowPos = mul(float4(posW, 1.0f), lightVP);
    float3 shadowCoord;
    shadowCoord.xy = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
    shadowCoord.y  = 1.0f - shadowCoord.y;
    shadowCoord.z  = shadowPos.z / shadowPos.w;

    if (shadowCoord.x < 0.0f || shadowCoord.x > 1.0f ||
        shadowCoord.y < 0.0f || shadowCoord.y > 1.0f ||
        shadowCoord.z < 0.0f || shadowCoord.z > 1.0f)
    {
        return 1.0f;
    }

    if (cascadeIdx == 0)
        return SampleShadowMapPCF(tShadowMap0, shadowCoord, shadowMapSize);
    else if (cascadeIdx == 1)
        return SampleShadowMapPCF(tShadowMap1, shadowCoord, shadowMapSize);
    else if (cascadeIdx == 2)
        return SampleShadowMapPCF(tShadowMap2, shadowCoord, shadowMapSize);
    else
        return SampleShadowMapPCF(tShadowMap3, shadowCoord, shadowMapSize);
}

// ============================================================================
// Cascade Blend Shadow (Wave 10-2-1)
// ============================================================================

/// @brief カスケードブレンド付きシャドウ計算
/// 隣接カスケード境界でlerpすることでハードな切り替え線を除去する
float ComputeCascadedShadowBlended(
    float3 posW, float viewZ,
    float4x4 lightVP[NUM_CASCADES],
    float cascadeSplits[NUM_CASCADES],
    float shadowMapSize, float blendWidth)
{
    int cascadeIndex = NUM_CASCADES - 1;
    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        if (viewZ < cascadeSplits[i])
        {
            cascadeIndex = i;
            break;
        }
    }

    float shadow0 = SampleCascadePCF(posW, lightVP[cascadeIndex], shadowMapSize, cascadeIndex);

    // Blend zone: transition region near cascade boundary
    if (cascadeIndex < NUM_CASCADES - 1 && blendWidth > 0.0)
    {
        float splitDist = cascadeSplits[cascadeIndex];
        float blendStart = splitDist - splitDist * blendWidth;
        if (viewZ > blendStart)
        {
            float shadow1 = SampleCascadePCF(posW, lightVP[cascadeIndex + 1], shadowMapSize, cascadeIndex + 1);
            float t = saturate((viewZ - blendStart) / (splitDist - blendStart));
            // Interleaved Gradient Noise dither for smoother transition
            float noise = frac(52.9829189 * frac(0.06711056 * posW.x + 0.00583715 * posW.z));
            t = saturate(t + (noise - 0.5) * 0.1);
            shadow0 = lerp(shadow0, shadow1, t);
        }
    }

    return shadow0;
}

// ============================================================================
// PCSS - Percentage-Closer Soft Shadows (Wave 10-2-2)
// ============================================================================

// Poisson disk samples for blocker search (16 taps)
static const float2 g_PoissonDisk16[16] = {
    float2(-0.94201624, -0.39906216), float2( 0.94558609, -0.76890725),
    float2(-0.09418410, -0.92938870), float2( 0.34495938,  0.29387760),
    float2(-0.91588581,  0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543,  0.27676845), float2( 0.97484398,  0.75648379),
    float2( 0.44323325, -0.97511554), float2( 0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2( 0.79197514,  0.19090188),
    float2(-0.24188840,  0.99706507), float2(-0.81409955,  0.91437590),
    float2( 0.19984126,  0.78641367), float2( 0.14383161, -0.14100790)
};

/// @brief カスケードインデックスから対応するシャドウマップの深度を読み取る
float SampleCascadeDepth(int cascadeIdx, float2 uv)
{
    if (cascadeIdx == 0)
        return tShadowMap0.SampleLevel(sShadowPointClamp, uv, 0).r;
    else if (cascadeIdx == 1)
        return tShadowMap1.SampleLevel(sShadowPointClamp, uv, 0).r;
    else if (cascadeIdx == 2)
        return tShadowMap2.SampleLevel(sShadowPointClamp, uv, 0).r;
    else
        return tShadowMap3.SampleLevel(sShadowPointClamp, uv, 0).r;
}

/// @brief PCSS blocker search — 光源方向からの平均遮蔽深度を求める
/// @param shadowCoord シャドウマップ上のUV+depth
/// @param lightSize 光源の面積サイズ
/// @param cascadeIdx カスケードインデックス
/// @param shadowMapSize シャドウマップ解像度
/// @return float2(avgBlockerDepth, blockerCount)
float2 BlockerSearch(float3 shadowCoord, float lightSize, int cascadeIdx, float shadowMapSize)
{
    float searchWidth = lightSize / shadowMapSize * 16.0;
    float blockerSum = 0.0;
    float blockerCount = 0.0;

    for (int i = 0; i < 16; ++i)
    {
        float2 offset = g_PoissonDisk16[i] * searchWidth;
        float2 sampleUV = shadowCoord.xy + offset;
        float sampleDepth = SampleCascadeDepth(cascadeIdx, sampleUV);
        if (sampleDepth < shadowCoord.z)
        {
            blockerSum += sampleDepth;
            blockerCount += 1.0;
        }
    }

    return float2(blockerSum / max(blockerCount, 1.0), blockerCount);
}

/// @brief PCSS penumbra幅推定
float EstimatePenumbra(float receiverDepth, float avgBlockerDepth, float lightSize)
{
    return lightSize * (receiverDepth - avgBlockerDepth) / avgBlockerDepth;
}

/// @brief PCSSシャドウサンプリング — 可変カーネルPCF
float SampleShadowMapPCSS(float3 posW, float4x4 lightVP, float shadowMapSize,
                           int cascadeIdx, float lightSize)
{
    float4 shadowClip = mul(float4(posW, 1.0), lightVP);
    float3 shadowNDC = shadowClip.xyz / shadowClip.w;
    float2 shadowUV = shadowNDC.xy * float2(0.5, -0.5) + 0.5;
    float  depth = shadowNDC.z;

    if (any(shadowUV < 0.0) || any(shadowUV > 1.0))
        return 1.0;

    // Step 1: Blocker search
    float2 blockerInfo = BlockerSearch(float3(shadowUV, depth), lightSize, cascadeIdx, shadowMapSize);
    if (blockerInfo.y < 1.0)
        return 1.0; // No blockers found

    // Step 2: Penumbra estimation
    float penumbraWidth = EstimatePenumbra(depth, blockerInfo.x, lightSize);
    float filterRadius = penumbraWidth / shadowMapSize * 16.0;

    // Step 3: Variable-size PCF with 16 Poisson taps
    float shadow = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = g_PoissonDisk16[i] * filterRadius;
        float2 sampleUV = shadowUV + offset;
        float sampleDepth = SampleCascadeDepth(cascadeIdx, sampleUV);
        shadow += (depth <= sampleDepth + 0.001) ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

#endif // SHADOW_UTILS_HLSLI
