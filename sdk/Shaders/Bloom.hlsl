/// @file Bloom.hlsl
/// @brief Bloomエフェクトシェーダー
///
/// パス:
/// 1. Threshold: HDRシーンから閾値以上の輝度を抽出
/// 2. Downsample: 4タップバイリニアで縮小
/// 3. GaussianBlurH/V: 9タップGaussianブラー（水平/垂直）
/// 4. Additive: ブルーム結果をintensityで乗算して出力（アディティブブレンドで合成）

#include "Fullscreen.hlsli"

cbuffer BloomConstants : register(b0)
{
    float  gThreshold;  // 輝度閾値（これ以上の輝度がブルームに寄与）
    float  gIntensity;  // ブルーム合成時の強度
    float2 gTexelSize;  // テクセルサイズ (1/width, 1/height)
};

Texture2D    tSource : register(t0);
SamplerState sLinear : register(s0);

/// @brief Rec.709輝度 — RGB→輝度変換
float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

/// @brief 閾値パス — 閾値以上の輝度を持つピクセルだけを抽出
float4 PSThreshold(FullscreenVSOutput input) : SV_Target
{
    float3 color = tSource.Sample(sLinear, input.uv).rgb;
    float lum = Luminance(color);
    float contribution = max(lum - gThreshold, 0.0f) / max(lum, 0.001f);
    return float4(color * contribution, 1.0f);
}

/// @brief ダウンサンプル — Karis averageで極端に明るいピクセルのちらつきを抑制
float4 PSDownsample(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 d = gTexelSize * 0.5f;
    float3 s0 = tSource.Sample(sLinear, uv + float2(-d.x, -d.y)).rgb;
    float3 s1 = tSource.Sample(sLinear, uv + float2( d.x, -d.y)).rgb;
    float3 s2 = tSource.Sample(sLinear, uv + float2(-d.x,  d.y)).rgb;
    float3 s3 = tSource.Sample(sLinear, uv + float2( d.x,  d.y)).rgb;

    // Karis average: 輝度の逆数で重み付け → 極端に明るいピクセルの影響を抑制
    float w0 = 1.0f / (1.0f + Luminance(s0));
    float w1 = 1.0f / (1.0f + Luminance(s1));
    float w2 = 1.0f / (1.0f + Luminance(s2));
    float w3 = 1.0f / (1.0f + Luminance(s3));
    float wSum = w0 + w1 + w2 + w3;

    float3 c = (s0 * w0 + s1 * w1 + s2 * w2 + s3 * w3) / wSum;
    return float4(c, 1.0f);
}

// --- Gaussian Blur 9-tap (中心+左右4サンプルずつ) ---
static const float GW[5] = { 0.227027f, 0.194596f, 0.121622f, 0.054054f, 0.016216f };

/// @brief 水平ガウシアンブラー — 9タップ分離フィルタの水平パス
float4 PSGaussianBlurH(FullscreenVSOutput input) : SV_Target
{
    float3 r = tSource.Sample(sLinear, input.uv).rgb * GW[0];
    for (int i = 1; i < 5; ++i)
    {
        float2 off = float2(gTexelSize.x * i, 0.0f);
        r += tSource.Sample(sLinear, input.uv + off).rgb * GW[i];
        r += tSource.Sample(sLinear, input.uv - off).rgb * GW[i];
    }
    return float4(r, 1.0f);
}

/// @brief 垂直ガウシアンブラー — 9タップ分離フィルタの垂直パス
float4 PSGaussianBlurV(FullscreenVSOutput input) : SV_Target
{
    float3 r = tSource.Sample(sLinear, input.uv).rgb * GW[0];
    for (int i = 1; i < 5; ++i)
    {
        float2 off = float2(0.0f, gTexelSize.y * i);
        r += tSource.Sample(sLinear, input.uv + off).rgb * GW[i];
        r += tSource.Sample(sLinear, input.uv - off).rgb * GW[i];
    }
    return float4(r, 1.0f);
}

/// @brief コピーパス — 同一解像度のテクスチャコピー
float4 PSCopy(FullscreenVSOutput input) : SV_Target
{
    return float4(tSource.Sample(sLinear, input.uv).rgb, 1.0f);
}

/// @brief アディティブパス — intensity乗算して出力（PSOのブレンドで加算合成される）
float4 PSAdditive(FullscreenVSOutput input) : SV_Target
{
    float3 bloom = tSource.Sample(sLinear, input.uv).rgb;
    return float4(bloom * gIntensity, 1.0f);
}
