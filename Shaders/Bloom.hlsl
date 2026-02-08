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
    float  gThreshold;
    float  gIntensity;
    float2 gTexelSize;    // 1/width, 1/height
};

Texture2D    tSource : register(t0);
SamplerState sLinear : register(s0);

// --- 輝度 ---
float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// --- Threshold Pass ---
float4 PSThreshold(FullscreenVSOutput input) : SV_Target
{
    float3 color = tSource.Sample(sLinear, input.uv).rgb;
    float lum = Luminance(color);
    float contribution = max(lum - gThreshold, 0.0f) / max(lum, 0.001f);
    return float4(color * contribution, 1.0f);
}

// --- Downsample Pass ---
float4 PSDownsample(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float2 d = gTexelSize * 0.5f;
    float3 c  = tSource.Sample(sLinear, uv + float2(-d.x, -d.y)).rgb;
           c += tSource.Sample(sLinear, uv + float2( d.x, -d.y)).rgb;
           c += tSource.Sample(sLinear, uv + float2(-d.x,  d.y)).rgb;
           c += tSource.Sample(sLinear, uv + float2( d.x,  d.y)).rgb;
    return float4(c * 0.25f, 1.0f);
}

// --- Gaussian Blur 9-tap ---
static const float GW[5] = { 0.227027f, 0.194596f, 0.121622f, 0.054054f, 0.016216f };

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

// --- Copy Pass (point-sample for same-size copy) ---
float4 PSCopy(FullscreenVSOutput input) : SV_Target
{
    return float4(tSource.Sample(sLinear, input.uv).rgb, 1.0f);
}

// --- Additive Pass ---
// intensity乗算してそのまま出力。PSO側のブレンドステートでアディティブ合成する。
float4 PSAdditive(FullscreenVSOutput input) : SV_Target
{
    float3 bloom = tSource.Sample(sLinear, input.uv).rgb;
    return float4(bloom * gIntensity, 1.0f);
}
