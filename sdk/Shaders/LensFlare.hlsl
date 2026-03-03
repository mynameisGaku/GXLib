/// @file LensFlare.hlsl
/// @brief レンズフレア -- ゴーストイメージ+スターバースト+色収差+ダスト
///
/// HDRシーンから閾値以上の明部を抽出し、以下のレンズ光学効果を合成する:
/// - ゴーストイメージ: UV反転+複数スケールの虚像
/// - ハローリング: 周辺部の環状グレア
/// - 色収差シフト: R/Bチャンネルの放射方向オフセット
/// - スターバーストパターン: 角度ベースの放射状フィルタ

#include "Fullscreen.hlsli"

Texture2D tHDRScene : register(t0);
SamplerState sLinear : register(s0);

cbuffer LensFlareCB : register(b0)
{
    float  gThreshold;
    float  gIntensity;
    int    gGhostCount;
    float  gGhostDispersion;
    float  gHaloRadius;
    float  gChromaticShift;
    float2 gScreenSize;
};

/// @brief 閾値フィルタ -- 輝度が閾値以上のピクセルのみ抽出
/// @param uv テクスチャ座標
/// @return 閾値を超えた部分の色 (超えない部分は黒)
float3 ThresholdFilter(float2 uv)
{
    float3 color = tHDRScene.Sample(sLinear, uv).rgb;
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    return color * max(0, brightness - gThreshold);
}

/// @brief レンズフレアを生成するピクセルシェーダー
float4 PSMain(FullscreenVSOutput input) : SV_Target0
{
    float2 texCoord = input.uv;
    float2 ghostVec = (0.5 - texCoord) * gGhostDispersion;

    float3 result = float3(0, 0, 0);

    // ゴーストイメージ (反転UV + 複数スケールの虚像)
    for (int i = 0; i < gGhostCount; ++i)
    {
        float2 offset = ghostVec * float(i);
        float2 sampleUV = frac(texCoord + offset);

        // 中心からの距離で減衰
        float weight = 1.0 - length(sampleUV - 0.5) * 2.0;
        weight = max(weight, 0.0);

        result += ThresholdFilter(sampleUV) * weight;
    }

    // ハローリング (周辺部の環状グレア)
    float ghostLen = length(ghostVec);
    float2 haloVec = (ghostLen > 0.001) ? (ghostVec / ghostLen) * gHaloRadius : float2(0, 0);
    float2 haloUV = texCoord + haloVec;
    float haloWeight = 1.0 - length(haloUV - 0.5) * 2.0;
    haloWeight = pow(max(haloWeight, 0.0), 5.0);
    result += ThresholdFilter(haloUV) * haloWeight;

    // 色収差シフト (R/Bチャンネルのオフセット)
    float2 chromaticOffset = (texCoord - 0.5) * gChromaticShift;
    result.r += ThresholdFilter(texCoord + chromaticOffset).r * 0.3;
    result.b += ThresholdFilter(texCoord - chromaticOffset).b * 0.3;

    // スターバーストパターン (角度ベースの放射状フィルタ)
    float2 centerVec = texCoord - 0.5;
    float angle = atan2(centerVec.y, centerVec.x);
    float starburst = cos(angle * 8.0) * 0.5 + 0.5;
    starburst = pow(starburst, 4.0);
    result *= lerp(1.0, starburst, 0.3);

    // 元のシーンカラー + フレア合成 (ping-pong パターン: 出力先は別RT)
    float3 scene = tHDRScene.Sample(sLinear, texCoord).rgb;
    return float4(scene + result * gIntensity, 1.0);
}
