/// @file FXAA.hlsl
/// @brief FXAA 3.11 Quality — 高速アンチエイリアシング
///
/// トーンマッピング後のLDR画像に適用する。
/// 輝度ベースのエッジ検出 → サブピクセルAA → ジャギー軽減

#include "Fullscreen.hlsli"

cbuffer FXAAConstants : register(b0)
{
    float2 gRcpFrame;     // 1/width, 1/height
    float  gFxaaQualitySubpix;        // サブピクセルAA品質 (default: 0.75)
    float  gFxaaQualityEdgeThreshold; // エッジ閾値 (default: 0.166)
};

Texture2D    tScene  : register(t0);
SamplerState sLinear : register(s0);

float FxaaLuma(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 rcp = gRcpFrame;

    // 周囲5点の輝度
    float3 rgbM  = tScene.Sample(sLinear, uv).rgb;
    float3 rgbN  = tScene.Sample(sLinear, uv + float2( 0, -rcp.y)).rgb;
    float3 rgbS  = tScene.Sample(sLinear, uv + float2( 0,  rcp.y)).rgb;
    float3 rgbW  = tScene.Sample(sLinear, uv + float2(-rcp.x,  0)).rgb;
    float3 rgbE  = tScene.Sample(sLinear, uv + float2( rcp.x,  0)).rgb;

    float lumM = FxaaLuma(rgbM);
    float lumN = FxaaLuma(rgbN);
    float lumS = FxaaLuma(rgbS);
    float lumW = FxaaLuma(rgbW);
    float lumE = FxaaLuma(rgbE);

    float lumMin = min(lumM, min(min(lumN, lumS), min(lumW, lumE)));
    float lumMax = max(lumM, max(max(lumN, lumS), max(lumW, lumE)));
    float lumRange = lumMax - lumMin;

    // エッジが小さければ早期リターン
    if (lumRange < max(0.0312f, lumMax * gFxaaQualityEdgeThreshold))
        return float4(rgbM, 1.0f);

    // 斜め方向
    float3 rgbNW = tScene.Sample(sLinear, uv + float2(-rcp.x, -rcp.y)).rgb;
    float3 rgbNE = tScene.Sample(sLinear, uv + float2( rcp.x, -rcp.y)).rgb;
    float3 rgbSW = tScene.Sample(sLinear, uv + float2(-rcp.x,  rcp.y)).rgb;
    float3 rgbSE = tScene.Sample(sLinear, uv + float2( rcp.x,  rcp.y)).rgb;

    float lumNW = FxaaLuma(rgbNW);
    float lumNE = FxaaLuma(rgbNE);
    float lumSW = FxaaLuma(rgbSW);
    float lumSE = FxaaLuma(rgbSE);

    // サブピクセルAA
    float lumaAvg = (lumN + lumS + lumW + lumE) * 0.25f;
    float subpixA = saturate(abs(lumaAvg - lumM) / lumRange);
    float subpixB = (-2.0f * subpixA + 3.0f) * subpixA * subpixA;
    float subpixC = subpixB * subpixB * gFxaaQualitySubpix;

    // エッジ方向判定
    float edgeH = abs(lumNW + lumNE - 2.0f * lumN)
                + abs(lumW  + lumE  - 2.0f * lumM) * 2.0f
                + abs(lumSW + lumSE - 2.0f * lumS);
    float edgeV = abs(lumNW + lumSW - 2.0f * lumW)
                + abs(lumN  + lumS  - 2.0f * lumM) * 2.0f
                + abs(lumNE + lumSE - 2.0f * lumE);

    bool isHorizontal = (edgeH >= edgeV);

    // エッジに沿ったブレンド方向
    float2 step = isHorizontal ? float2(0, rcp.y) : float2(rcp.x, 0);
    float lumP = isHorizontal ? lumS : lumE;
    float lumN2 = isHorizontal ? lumN : lumW;

    float gradientP = abs(lumP - lumM);
    float gradientN = abs(lumN2 - lumM);

    float2 offset = (gradientP >= gradientN) ? step : -step;

    // エッジに沿ってサンプリング
    float3 rgbL = (rgbN + rgbS + rgbW + rgbE + rgbNW + rgbNE + rgbSW + rgbSE + rgbM) / 9.0f;

    // ブレンド
    float blendFactor = max(subpixC,
        saturate(max(gradientP, gradientN) / lumRange));
    blendFactor = min(blendFactor, 0.75f);

    float3 result = lerp(rgbM, tScene.Sample(sLinear, uv + offset * blendFactor).rgb, 0.5f);
    result = lerp(result, rgbL, subpixC);

    return float4(result, 1.0f);
}
