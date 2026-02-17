/// @file FXAA.hlsl
/// @brief FXAA 3.11 Quality — 高速アンチエイリアシング
///
/// トーンマッピング後のLDR画像に適用する。
/// 輝度ベースのエッジ検出 → エッジウォーキング → サブピクセルAA → ジャギー軽減

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

// エッジウォーキング品質プリセット（FXAA 3.11 Quality 29相当）
static const int FXAA_SEARCH_STEPS = 12;
static const float FXAA_QUALITY[12] = {
    1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0
};

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 rcp = gRcpFrame;

    // ============================================================
    // 1. ローカルコントラスト判定
    // ============================================================
    float3 rgbM  = tScene.SampleLevel(sLinear, uv, 0).rgb;
    float3 rgbN  = tScene.SampleLevel(sLinear, uv + float2( 0, -rcp.y), 0).rgb;
    float3 rgbS  = tScene.SampleLevel(sLinear, uv + float2( 0,  rcp.y), 0).rgb;
    float3 rgbW  = tScene.SampleLevel(sLinear, uv + float2(-rcp.x,  0), 0).rgb;
    float3 rgbE  = tScene.SampleLevel(sLinear, uv + float2( rcp.x,  0), 0).rgb;

    float lumM = FxaaLuma(rgbM);
    float lumN = FxaaLuma(rgbN);
    float lumS = FxaaLuma(rgbS);
    float lumW = FxaaLuma(rgbW);
    float lumE = FxaaLuma(rgbE);

    float lumMin = min(lumM, min(min(lumN, lumS), min(lumW, lumE)));
    float lumMax = max(lumM, max(max(lumN, lumS), max(lumW, lumE)));
    float lumRange = lumMax - lumMin;

    // コントラストが低ければ処理不要
    if (lumRange < max(0.0312f, lumMax * gFxaaQualityEdgeThreshold))
        return float4(rgbM, 1.0f);

    // ============================================================
    // 2. 斜めサンプリング + エッジ方向判定
    // ============================================================
    float lumNW = FxaaLuma(tScene.SampleLevel(sLinear, uv + float2(-rcp.x, -rcp.y), 0).rgb);
    float lumNE = FxaaLuma(tScene.SampleLevel(sLinear, uv + float2( rcp.x, -rcp.y), 0).rgb);
    float lumSW = FxaaLuma(tScene.SampleLevel(sLinear, uv + float2(-rcp.x,  rcp.y), 0).rgb);
    float lumSE = FxaaLuma(tScene.SampleLevel(sLinear, uv + float2( rcp.x,  rcp.y), 0).rgb);

    // サブピクセルAA (12タップ加重平均)
    float lumNS = lumN + lumS;
    float lumWE = lumW + lumE;
    float lumNWSW = lumNW + lumSW;
    float lumNESE = lumNE + lumSE;
    float lumaAvg = (2.0f * (lumNS + lumWE) + lumNWSW + lumNESE) / 12.0f;
    float subpixA = saturate(abs(lumaAvg - lumM) / lumRange);
    float subpixB = (-2.0f * subpixA + 3.0f) * subpixA * subpixA;
    float subpixC = subpixB * subpixB * gFxaaQualitySubpix;

    // エッジ方向判定（水平 or 垂直）
    float edgeH = abs(lumNW + lumNE - 2.0f * lumN)
                + abs(lumW  + lumE  - 2.0f * lumM) * 2.0f
                + abs(lumSW + lumSE - 2.0f * lumS);
    float edgeV = abs(lumNW + lumSW - 2.0f * lumW)
                + abs(lumN  + lumS  - 2.0f * lumM) * 2.0f
                + abs(lumNE + lumSE - 2.0f * lumE);

    bool isHorizontal = (edgeH >= edgeV);

    // ============================================================
    // 3. エッジの正負側判定
    // ============================================================
    // エッジに垂直な2方向の輝度勾配を比較
    float lum1 = isHorizontal ? lumN : lumW;   // 負方向（上 or 左）
    float lum2 = isHorizontal ? lumS : lumE;   // 正方向（下 or 右）
    float gradient1 = abs(lum1 - lumM);
    float gradient2 = abs(lum2 - lumM);

    bool is1Steeper = gradient1 >= gradient2;
    float gradientScaled = 0.25f * max(gradient1, gradient2);

    // エッジに垂直方向のステップ
    float stepLength = isHorizontal ? rcp.y : rcp.x;
    float lumaLocalAvg;
    if (is1Steeper)
    {
        stepLength = -stepLength; // 負方向にオフセット
        lumaLocalAvg = 0.5f * (lum1 + lumM);
    }
    else
    {
        lumaLocalAvg = 0.5f * (lum2 + lumM);
    }

    // エッジ上のサンプリング位置（エッジを跨いだ半ピクセル位置）
    float2 edgeUV = uv;
    if (isHorizontal)
        edgeUV.y += stepLength * 0.5f;
    else
        edgeUV.x += stepLength * 0.5f;

    // ============================================================
    // 4. エッジウォーキング: エッジに沿って両方向に探索
    // ============================================================
    float2 edgeStep = isHorizontal ? float2(rcp.x, 0) : float2(0, rcp.y);

    float2 uvP = edgeUV + edgeStep;
    float2 uvN = edgeUV - edgeStep;

    float lumEndP = FxaaLuma(tScene.SampleLevel(sLinear, uvP, 0).rgb) - lumaLocalAvg;
    float lumEndN = FxaaLuma(tScene.SampleLevel(sLinear, uvN, 0).rgb) - lumaLocalAvg;

    bool doneP = abs(lumEndP) >= gradientScaled;
    bool doneN = abs(lumEndN) >= gradientScaled;

    [unroll]
    for (int i = 0; i < FXAA_SEARCH_STEPS; i++)
    {
        if (doneP && doneN) break;

        if (!doneP)
        {
            uvP += edgeStep * FXAA_QUALITY[i];
            lumEndP = FxaaLuma(tScene.SampleLevel(sLinear, uvP, 0).rgb) - lumaLocalAvg;
            doneP = abs(lumEndP) >= gradientScaled;
        }
        if (!doneN)
        {
            uvN -= edgeStep * FXAA_QUALITY[i];
            lumEndN = FxaaLuma(tScene.SampleLevel(sLinear, uvN, 0).rgb) - lumaLocalAvg;
            doneN = abs(lumEndN) >= gradientScaled;
        }
    }

    // ============================================================
    // 5. ブレンド比率計算
    // ============================================================
    // 最近傍の端点までの距離
    float distP = isHorizontal ? (uvP.x - uv.x) : (uvP.y - uv.y);
    float distN = isHorizontal ? (uv.x - uvN.x) : (uv.y - uvN.y);

    bool directionIsP = distP <= distN;
    float lumEndNearest = directionIsP ? lumEndP : lumEndN;
    float spanLength = distP + distN;
    float shortestDist = min(distP, distN);

    // 端点の輝度変化方向が中心と一致するか確認
    bool goodSpan = (lumEndNearest < 0.0f) != (lumM - lumaLocalAvg < 0.0f);

    // ピクセルオフセット: エッジ上の位置に基づくブレンド
    float pixelOffset = goodSpan ? (0.5f - shortestDist / spanLength) : 0.0f;

    // サブピクセルAA と エッジAAの大きい方を採用
    float finalOffset = max(pixelOffset, subpixC);

    // ============================================================
    // 6. 最終サンプリング（エッジに垂直方向にオフセット）
    // ============================================================
    float2 finalUV = uv;
    if (isHorizontal)
        finalUV.y += finalOffset * stepLength;
    else
        finalUV.x += finalOffset * stepLength;

    return float4(tScene.SampleLevel(sLinear, finalUV, 0).rgb, 1.0f);
}
