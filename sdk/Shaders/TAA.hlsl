/// @file TAA.hlsl
/// @brief Temporal Anti-Aliasing シェーダー
///
/// 1. 深度からリプロジェクション → 前フレームUV取得
/// 2. 3x3近傍の variance clipping (mu +/- gamma*sigma, gamma=1.0)
/// 3. 履歴クランプ + ブレンド: lerp(current, clampedHistory, blendFactor)
/// 4. 画面外 historyUV → current そのまま

#include "Fullscreen.hlsli"

cbuffer TAACB : register(b0)
{
    float4x4 invViewProjection;       // 現フレームの逆VP行列
    float4x4 previousViewProjection;  // 前フレームのVP行列
    float2   jitterOffset;            // サブピクセルジッター量（NDC単位）
    float    blendFactor;             // 履歴のブレンド率 (高いほど安定、低いほど応答性が高い)
    float    screenWidth;             // スクリーン幅
    float    screenHeight;            // スクリーン高さ
    float3   padding;
};

Texture2D<float4> gScene   : register(t0);  // current (jittered)
Texture2D<float4> gHistory : register(t1);  // previous TAA output
Texture2D<float>  gDepth   : register(t2);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

/// Luminance (Rec.709)
float Luminance(float3 color)
{
    return dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
}

/// @brief TAA PS — リプロジェクション + variance clipping + Karisトーンマップブレンド
float4 PSTAA(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = gDepth.Sample(gPointSampler, uv).r;

    // === リプロジェクション ===
    // UV → NDC (clip space)
    float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);

    // ワールド座標再構成 (行ベクトル × 行列: DirectXMath 規約)
    float4 worldPos = mul(clipPos, invViewProjection);
    worldPos /= worldPos.w;

    // 前フレームのスクリーン座標に再投影
    float4 prevClip = mul(worldPos, previousViewProjection);
    float2 prevNDC = prevClip.xy / prevClip.w;
    float2 historyUV = prevNDC * float2(0.5, -0.5) + 0.5; // NDC[-1,1]→UV[0,1] (Y反転: D3D UV原点は左上)

    // 現フレームの色 (ジッター補正: ジッター分UVをオフセットして中心を取得)
    float2 unjitteredUV = uv - jitterOffset * float2(0.5, -0.5);
    float4 currentColor = gScene.Sample(gLinearSampler, unjitteredUV);

    // === 画面外チェック ===
    if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0)
        return currentColor;

    float4 historyColor = gHistory.Sample(gLinearSampler, historyUV);

    // === 3x3 近傍クランプ (Variance Clipping) ===
    float2 texelSize = float2(1.0 / screenWidth, 1.0 / screenHeight);

    float4 m1 = float4(0, 0, 0, 0); // 平均
    float4 m2 = float4(0, 0, 0, 0); // 二乗平均

    [unroll]
    for (int dy = -1; dy <= 1; dy++)
    {
        [unroll]
        for (int dx = -1; dx <= 1; dx++)
        {
            float2 sampleUV = uv + float2(dx, dy) * texelSize;
            float4 s = gScene.Sample(gLinearSampler, sampleUV);
            m1 += s;
            m2 += s * s;
        }
    }

    m1 /= 9.0;
    m2 /= 9.0;
    float4 sigma = sqrt(max(m2 - m1 * m1, 0.0));

    float gamma = 1.0; // クランプの緩さ (1.0 = 標準)
    float4 colorMin = m1 - gamma * sigma;
    float4 colorMax = m1 + gamma * sigma;

    // 履歴をクランプ
    float4 clampedHistory = clamp(historyColor, colorMin, colorMax);

    // === Karis 2014: tonemap-before-blend ===
    // HDR色をトーンマップ重みで圧縮してからブレンドし、逆変換で戻す
    float4 mappedCurrent = currentColor / (1.0 + Luminance(currentColor.rgb));
    float4 mappedHistory = clampedHistory / (1.0 + Luminance(clampedHistory.rgb));

    // === ブレンド ===
    float4 blended = lerp(mappedCurrent, mappedHistory, blendFactor);

    // 逆トーンマップ (epsilon でゼロ除算回避)
    float4 result = blended / max(1.0 - Luminance(blended.rgb), 1e-4);

    return result;
}
