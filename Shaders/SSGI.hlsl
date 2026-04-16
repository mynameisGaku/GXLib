/// @file SSGI.hlsl
/// @brief Screen-Space Global Illumination シェーダー
///
/// GBufferの深度・法線・カラーからスクリーン空間レイマーチングで
/// 周囲サーフェスのバウンスライト（間接照明）を計算する。
/// コサイン重み付きランダム方向にレイを飛ばし、ヒットしたカラーを
/// GI寄与として加算する。

#include "Fullscreen.hlsli"

cbuffer SSGICB : register(b0)
{
    float  gIntensity;      // GI強度
    int    gSampleCount;    // レイサンプル数
    float  gMaxDistance;    // レイ最大距離
    float  gThickness;     // ヒット判定の深度厚み
    float  gScreenWidth;   // スクリーン幅
    float  gScreenHeight;  // スクリーン高さ
    float2 gPadding;
};

Texture2D<float4> gScene  : register(t0);   // HDRシーンカラー
Texture2D<float>  gDepth  : register(t1);   // 深度バッファ
Texture2D<float4> gNormal : register(t2);   // GBuffer法線 (RGB: normal, A: valid)
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

/// @brief 擬似乱数ハッシュ（Wang hash）
float Hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

/// @brief 2Dランダムベクトル生成
float2 RandomDir(float2 uv, int sampleIdx)
{
    float angle = Hash(uv * 1000.0 + float2(sampleIdx * 7.23, sampleIdx * 13.17)) * 6.28318;
    return float2(cos(angle), sin(angle));
}

/// @brief SSGI PS - スクリーン空間レイマーチングによる間接照明
float4 PSSSGI(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 sceneColor = gScene.SampleLevel(gLinearSampler, uv, 0);
    float  depth = gDepth.SampleLevel(gPointSampler, uv, 0).r;

    // 空ピクセルはスキップ
    if (depth >= 1.0)
        return sceneColor;

    // 法線を取得
    float4 normalSample = gNormal.SampleLevel(gLinearSampler, uv, 0);
    if (normalSample.a < 0.005)
        return sceneColor;

    float3 N = normalize(normalSample.rgb * 2.0 - 1.0);

    float2 texelSize = float2(1.0 / gScreenWidth, 1.0 / gScreenHeight);

    // 間接照明の蓄積
    float3 giAccum = float3(0, 0, 0);
    float  totalWeight = 0.0;

    int count = max(gSampleCount, 4);
    float maxRadiusUV = gMaxDistance * texelSize.x * 10.0; // ワールド→UV空間の概算スケール

    for (int i = 0; i < count; i++)
    {
        // ランダム方向と距離
        float2 dir = RandomDir(uv, i);
        float  radius = (float(i) + 0.5) / float(count);
        float2 sampleUV = uv + dir * radius * maxRadiusUV;

        // スクリーン外チェック
        if (sampleUV.x < 0 || sampleUV.x > 1 || sampleUV.y < 0 || sampleUV.y > 1)
            continue;

        float  sampleDepth = gDepth.SampleLevel(gPointSampler, sampleUV, 0).r;
        if (sampleDepth >= 1.0)
            continue;

        // 深度差チェック
        float depthDiff = abs(depth - sampleDepth);
        if (depthDiff > gThickness)
            continue;

        // サンプル法線取得
        float4 sampleNormal = gNormal.SampleLevel(gLinearSampler, sampleUV, 0);
        if (sampleNormal.a < 0.005)
            continue;

        float3 sN = normalize(sampleNormal.rgb * 2.0 - 1.0);

        // 法線の向きに基づくコサイン重み
        float weight = max(dot(N, sN), 0.0);
        weight *= (1.0 - radius); // 距離減衰

        // サンプルカラーを取得してGI寄与を加算
        float3 sampleColor = gScene.SampleLevel(gLinearSampler, sampleUV, 0).rgb;
        giAccum += sampleColor * weight;
        totalWeight += weight;
    }

    // 正規化してGI合成
    if (totalWeight > 0.001)
    {
        giAccum /= totalWeight;
    }

    float3 result = sceneColor.rgb + giAccum * gIntensity;
    return float4(result, sceneColor.a);
}
