/// @file Outline.hlsl
/// @brief エッジ検出アウトラインシェーダー
///
/// 深度バッファから法線を再構成し、Sobel エッジ検出 + 法線エッジで
/// アウトラインを合成する。スカイボックス (depth >= 1.0) はスキップ。

#include "Fullscreen.hlsli"

cbuffer OutlineCB : register(b0)
{
    float4x4 invProjection;  // 逆プロジェクション行列
    float depthThreshold;    // 深度エッジの感度閾値
    float normalThreshold;   // 法線エッジの感度閾値
    float intensity;         // アウトラインの強度
    float screenWidth;       // スクリーン幅
    float screenHeight;      // スクリーン高さ
    float nearZ;             // ニアクリップ距離
    float padding0;
    float padding1;
    float4 lineColor;        // アウトラインの色 (.a=不透明度)
};

Texture2D<float4> gScene : register(t0);
Texture2D<float>  gDepth : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

// 深度バッファ値 → ビュー空間Z
float ViewZFromDepth(float d)
{
    float4 vp = mul(float4(0, 0, d, 1.0), invProjection);
    return vp.z / vp.w;
}

// UV + 深度 → ビュー空間座標
float3 ReconstructViewPos(float2 uv, float depth)
{
    float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 vp = mul(float4(ndc, depth, 1.0), invProjection);
    return vp.xyz / vp.w;
}

// 法線復元（事前サンプリング済み深度値を受け取る版）
float3 ReconstructNormalFromDepths(float3 posC, float depthC, float2 uv, float2 ts,
                                   float dR, float dL, float dD, float dU)
{
    float3 pR = ReconstructViewPos(uv + float2(ts.x, 0), dR);
    float3 pL = ReconstructViewPos(uv - float2(ts.x, 0), dL);
    float3 pD = ReconstructViewPos(uv + float2(0, ts.y), dD);
    float3 pU = ReconstructViewPos(uv - float2(0, ts.y), dU);

    // エッジ跨ぎ防止: 深度差が小さい側を選択
    float3 dx = (abs(dR - depthC) < abs(dL - depthC)) ? (pR - posC) : (posC - pL);
    float3 dy = (abs(dD - depthC) < abs(dU - depthC)) ? (pD - posC) : (posC - pU);

    float3 n = normalize(cross(dx, dy));
    // カメラ方向を向くように修正
    if (dot(n, posC) > 0.0) n = -n;
    return n;
}

// 法線復元（±1px、片側差分選択方式 — 深度を内部でサンプリング）
float3 ReconstructNormal(float3 posC, float depthC, float2 uv, float2 ts)
{
    float dR = gDepth.SampleLevel(gPointSampler, uv + float2(ts.x, 0), 0).r;
    float dL = gDepth.SampleLevel(gPointSampler, uv - float2(ts.x, 0), 0).r;
    float dD = gDepth.SampleLevel(gPointSampler, uv + float2(0, ts.y), 0).r;
    float dU = gDepth.SampleLevel(gPointSampler, uv - float2(0, ts.y), 0).r;

    return ReconstructNormalFromDepths(posC, depthC, uv, ts, dR, dL, dD, dU);
}

/// @brief アウトラインPS — Sobelエッジ検出(深度) + 法線エッジでアウトラインを合成
float4 PSOutline(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 sceneColor = gScene.SampleLevel(gLinearSampler, uv, 0);
    float depthC = gDepth.SampleLevel(gPointSampler, uv, 0).r;

    // スカイボックスはスキップ
    if (depthC >= 1.0)
        return sceneColor;

    float2 ts = float2(1.0 / screenWidth, 1.0 / screenHeight);

    // ============================================================
    // 0. 4方向深度を事前サンプリング（Sobel + 法線で再利用）
    // ============================================================
    float dR = gDepth.SampleLevel(gPointSampler, uv + float2(ts.x, 0), 0).r;
    float dL = gDepth.SampleLevel(gPointSampler, uv - float2(ts.x, 0), 0).r;
    float dU = gDepth.SampleLevel(gPointSampler, uv + float2(0, -ts.y), 0).r;
    float dD = gDepth.SampleLevel(gPointSampler, uv + float2(0, ts.y), 0).r;

    // ============================================================
    // 1. 深度エッジ (Sobel): 3x3近傍のビュー空間Zに Sobel 適用
    // ============================================================
    float viewZC = ViewZFromDepth(depthC);

    // 4隅は別途サンプリング（Sobel専用）
    float viewZ_TL = ViewZFromDepth(gDepth.SampleLevel(gPointSampler, uv + float2(-ts.x, -ts.y), 0).r);
    float viewZ_T  = ViewZFromDepth(dU);
    float viewZ_TR = ViewZFromDepth(gDepth.SampleLevel(gPointSampler, uv + float2( ts.x, -ts.y), 0).r);
    float viewZ_L  = ViewZFromDepth(dL);
    float viewZ_R  = ViewZFromDepth(dR);
    float viewZ_BL = ViewZFromDepth(gDepth.SampleLevel(gPointSampler, uv + float2(-ts.x,  ts.y), 0).r);
    float viewZ_B  = ViewZFromDepth(dD);
    float viewZ_BR = ViewZFromDepth(gDepth.SampleLevel(gPointSampler, uv + float2( ts.x,  ts.y), 0).r);

    // Sobel X: [-1 0 +1; -2 0 +2; -1 0 +1]
    float sobelX = -viewZ_TL - 2.0 * viewZ_L - viewZ_BL
                   + viewZ_TR + 2.0 * viewZ_R + viewZ_BR;

    // Sobel Y: [-1 -2 -1; 0 0 0; +1 +2 +1]
    float sobelY = -viewZ_TL - 2.0 * viewZ_T - viewZ_TR
                   + viewZ_BL + 2.0 * viewZ_B + viewZ_BR;

    float depthEdge = sqrt(sobelX * sobelX + sobelY * sobelY);

    // 距離非依存: 中心ビュー空間Zで正規化し、遠方の平面でもエッジ誤検出を防止
    float depthFactor = saturate(depthEdge / (depthThreshold * abs(viewZC)));

    // ============================================================
    // 2. 法線エッジ: 中心+4近傍の法線から dot の差
    // ============================================================
    float3 posC = ReconstructViewPos(uv, depthC);
    // 事前サンプリング済みの深度を再利用して法線復元
    float3 normalC = ReconstructNormalFromDepths(posC, depthC, uv, ts, dR, dL, dD, dU);

    // スカイボックス近傍はスキップ
    float normalEdge = 0.0;
    if (dR < 1.0)
    {
        float3 pR = ReconstructViewPos(uv + float2(ts.x, 0), dR);
        float3 nR = ReconstructNormal(pR, dR, uv + float2(ts.x, 0), ts);
        normalEdge = max(normalEdge, 1.0 - dot(normalC, nR));
    }
    if (dL < 1.0)
    {
        float3 pL = ReconstructViewPos(uv - float2(ts.x, 0), dL);
        float3 nL = ReconstructNormal(pL, dL, uv - float2(ts.x, 0), ts);
        normalEdge = max(normalEdge, 1.0 - dot(normalC, nL));
    }
    if (dD < 1.0)
    {
        float3 pD = ReconstructViewPos(uv + float2(0, ts.y), dD);
        float3 nD = ReconstructNormal(pD, dD, uv + float2(0, ts.y), ts);
        normalEdge = max(normalEdge, 1.0 - dot(normalC, nD));
    }
    if (dU < 1.0)
    {
        float3 pU = ReconstructViewPos(uv - float2(0, ts.y), dU);
        float3 nU = ReconstructNormal(pU, dU, uv - float2(0, ts.y), ts);
        normalEdge = max(normalEdge, 1.0 - dot(normalC, nU));
    }

    float normalFactor = saturate(normalEdge / normalThreshold);

    // ============================================================
    // 3. 合成: edge = max(depthFactor, normalFactor) * intensity
    // ============================================================
    float edge = max(depthFactor, normalFactor) * intensity;
    edge = saturate(edge);

    float3 result = lerp(sceneColor.rgb, lineColor.rgb, edge * lineColor.a);
    return float4(result, sceneColor.a);
}
