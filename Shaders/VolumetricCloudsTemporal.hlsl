/// @file VolumetricCloudsTemporal.hlsl
/// @brief ボリュメトリッククラウド テンポラルリゾルブ + バイラテラルアップスケール + シーン合成
///
/// 半解像度クラウドRTをバイラテラルアップスケールし、前フレームのヒストリと
/// テンポラルブレンドした上でフル解像度シーンに合成する。

#include "Fullscreen.hlsli"

// ─── 定数バッファ ─────────────────────────────────────────────
cbuffer CloudTemporalConstants : register(b0)
{
    float4x4 prevViewProjection;
    float4x4 invViewProjection;
    float    alpha;            // テンポラルブレンド比率 (0.05)
    float    screenWidth;
    float    screenHeight;
    float    hasHistory;       // 0.0 or 1.0
    float    halfWidth;
    float    halfHeight;
    float    depthThreshold;   // バイラテラル閾値 (0.01)
    float    _pad;
};

Texture2D<float4> g_CloudRT  : register(t0);  // 半解像度クラウド (RGB=light, A=transmittance)
Texture2D<float4> g_History  : register(t1);  // フル解像度ヒストリ
Texture2D<float>  g_Depth    : register(t2);  // フル解像度深度
Texture2D<float4> g_Scene    : register(t3);  // フル解像度HDRシーン
SamplerState      linearSamp : register(s0);  // linear clamp
SamplerState      pointSamp  : register(s1);  // point clamp

// ─── バイラテラルアップスケール ───────────────────────────────
// 2×2 bilinear + 深度重みで半解像度→フル解像度
float4 BilateralUpsample(float2 uv)
{
    float centerDepth = g_Depth.SampleLevel(pointSamp, uv, 0);

    // 半解像度テクセルのオフセット
    float2 halfTexelSize = float2(1.0 / halfWidth, 1.0 / halfHeight);
    float2 offsets[4] = {
        float2(-0.25, -0.25),
        float2( 0.25, -0.25),
        float2(-0.25,  0.25),
        float2( 0.25,  0.25)
    };

    float4 result = float4(0, 0, 0, 0);
    float  totalWeight = 0.0;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float2 sampleUV = uv + offsets[i] * halfTexelSize;
        float4 cloudSample = g_CloudRT.SampleLevel(linearSamp, sampleUV, 0);

        // 深度重み: 中心ピクセルと近い深度のサンプルに高い重みを付ける
        float sampleDepth = g_Depth.SampleLevel(pointSamp, sampleUV, 0);
        float depthDiff = abs(centerDepth - sampleDepth);
        float depthWeight = exp(-depthDiff / max(depthThreshold, 0.0001));

        result += cloudSample * depthWeight;
        totalWeight += depthWeight;
    }

    return result / max(totalWeight, 0.0001);
}

// ─── テンポラルリゾルブ ───────────────────────────────────────
float4 PSTemporalResolve(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;

    // 1. バイラテラルアップスケールで現フレームのクラウドデータ取得
    float4 currentCloud = BilateralUpsample(uv);

    // 2. シーンカラー取得
    float4 sceneColor = g_Scene.SampleLevel(pointSamp, uv, 0);

    // 3. ヒストリなし → 現フレームのみでシーン合成
    if (hasHistory < 0.5)
    {
        float3 result = sceneColor.rgb * currentCloud.a + currentCloud.rgb;
        return float4(result, currentCloud.a);
    }

    // 4. リプロジェクション: depth+invVP→ワールド→prevVP→prevUV
    float depth = g_Depth.SampleLevel(pointSamp, uv, 0);
    float2 ndc = float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
    float4 worldPos = mul(float4(ndc, depth, 1.0), invViewProjection);
    worldPos /= worldPos.w;

    float4 prevClip = mul(float4(worldPos.xyz, 1.0), prevViewProjection);
    float2 prevNDC = prevClip.xy / prevClip.w;
    float2 prevUV = float2(prevNDC.x * 0.5 + 0.5, 0.5 - prevNDC.y * 0.5);

    // 5. prevUV範囲外 → 現フレームのみ
    bool outOfBounds = any(prevUV < 0.0) || any(prevUV > 1.0);
    if (outOfBounds)
    {
        float3 result = sceneColor.rgb * currentCloud.a + currentCloud.rgb;
        return float4(result, currentCloud.a);
    }

    // 6. ヒストリサンプル
    float4 historyCloud = g_History.SampleLevel(linearSamp, prevUV, 0);

    // 7. 3×3 min/max クランプ（ゴースティング防止）
    // g_CloudRTは半解像度なのでneighborサンプルも半解像度テクセルサイズを使用
    float2 texelSize = float2(1.0 / halfWidth, 1.0 / halfHeight);
    float4 cMin = currentCloud;
    float4 cMax = currentCloud;

    [unroll]
    for (int y = -1; y <= 1; y++)
    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        if (x == 0 && y == 0) continue;
        float2 neighborUV = uv + float2(x, y) * texelSize;
        // Use half-res sample for the neighborhood
        float4 neighbor = g_CloudRT.SampleLevel(linearSamp, neighborUV, 0);
        cMin = min(cMin, neighbor);
        cMax = max(cMax, neighbor);
    }

    // クランプ範囲を少し緩和（急なジャンプを軽減）
    float4 margin = (cMax - cMin) * 0.15;
    historyCloud = clamp(historyCloud, cMin - margin, cMax + margin);

    // 8. モーション適応テンポラルブレンド
    // リプロジェクション時のUVずれ = カメラ移動量の指標
    float2 velocity = abs(uv - prevUV);
    float motionMag = length(velocity * float2(screenWidth, screenHeight));
    // 静止→alpha(0.1), 高速移動→0.5 でスムーズに遷移
    float adaptiveAlpha = lerp(alpha, 0.5, saturate(motionMag * 0.08));
    float4 blended = lerp(historyCloud, currentCloud, adaptiveAlpha);

    // 9. シーン合成
    float3 result = sceneColor.rgb * blended.a + blended.rgb;
    return float4(result, blended.a);
}
