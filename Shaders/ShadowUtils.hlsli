/// @file ShadowUtils.hlsli
/// @brief シャドウマップサンプリング・カスケード選択ユーティリティ

#ifndef SHADOW_UTILS_HLSLI
#define SHADOW_UTILS_HLSLI

#define NUM_CASCADES 4

// シャドウマップテクスチャ（t8-t11）
Texture2D    tShadowMap0 : register(t8);
Texture2D    tShadowMap1 : register(t9);
Texture2D    tShadowMap2 : register(t10);
Texture2D    tShadowMap3 : register(t11);

// 比較サンプラー（s2）
SamplerComparisonState sShadowCmp : register(s2);

// ============================================================================
// PCFフィルタリング（5x5カーネル）
// ============================================================================
float SampleShadowMapPCF(Texture2D shadowMap, float3 shadowCoord, float shadowMapSize)
{
    float texelSize = 1.0f / shadowMapSize;
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
// カスケード選択 + シャドウ計算
// ============================================================================
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

#endif // SHADOW_UTILS_HLSLI
