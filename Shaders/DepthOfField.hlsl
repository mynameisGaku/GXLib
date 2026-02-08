/// @file DepthOfField.hlsl
/// @brief Depth of Field (被写界深度) シェーダー
///
/// 4つのパスを含む:
/// - PSCoC: 深度からCoC (Circle of Confusion) を計算
/// - PSBlurH / PSBlurV: ガウシアンブラー (水平/垂直分離)
/// - PSComposite: シャープとブラーをCoC値でlerp合成

#include "Fullscreen.hlsli"

// ============================================================================
// Pass 1: CoC生成
// ============================================================================

cbuffer CoCConstants : register(b0)
{
    float4x4 gInvProjection;   // 64B
    float    gFocalDistance;    // フォーカス距離 (ビュー空間Z)
    float    gFocalRange;      // フォーカス鮮明範囲
    float    gCoCScale;        // CoC最大半径 (px)
    float    gScreenWidth;
    float    gScreenHeight;
    float    gNearZ;
    float    gFarZ;
    float    gPadding;
};

Texture2D    tDepth  : register(t0);
SamplerState sPoint  : register(s0);

// 深度値からリニアビュー空間Zを計算
float LinearizeDepth(float depth)
{
    // NDC→ビュー空間Z
    float4 ndc = float4(0, 0, depth, 1.0);
    float4 viewPos = mul(ndc, gInvProjection);
    return viewPos.z / viewPos.w;
}

float4 PSCoC(FullscreenVSOutput input) : SV_Target
{
    float depth = tDepth.Sample(sPoint, input.uv).r;

    // スカイ (depth ≈ 1.0) は最大ブラー
    if (depth >= 0.9999)
        return float4(1.0, 0, 0, 0);

    // ビュー空間のZ値を取得
    float linearZ = LinearizeDepth(depth);

    // フォーカス距離からのずれ → CoC
    float diff = abs(linearZ - gFocalDistance);
    float coc = saturate((diff - gFocalRange * 0.5) / (gFocalRange * 0.5 + 0.001));

    return float4(coc, 0, 0, 0);
}

// ============================================================================
// Pass 2/3: ガウシアンブラー (H/V分離)
// ============================================================================

cbuffer BlurConstants : register(b0)
{
    float gTexelSizeX;
    float gTexelSizeY;
    float gBlurPadding[2];
};

Texture2D    tScene      : register(t0);
SamplerState sLinear     : register(s0);

static const int kKernelRadius = 6;
static const float kGaussWeights[7] = {
    0.1963,   // center
    0.1745,   // ±1
    0.1217,   // ±2
    0.0667,   // ±3
    0.0287,   // ±4
    0.0097,   // ±5
    0.0026    // ±6
};

float4 PSBlurH(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 result = tScene.Sample(sLinear, uv) * kGaussWeights[0];

    [unroll]
    for (int i = 1; i <= kKernelRadius; ++i)
    {
        float2 offset = float2(gTexelSizeX * i * 2.0, 0); // *2 でblur半径増大
        result += tScene.Sample(sLinear, uv + offset) * kGaussWeights[i];
        result += tScene.Sample(sLinear, uv - offset) * kGaussWeights[i];
    }

    return result;
}

float4 PSBlurV(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 result = tScene.Sample(sLinear, uv) * kGaussWeights[0];

    [unroll]
    for (int i = 1; i <= kKernelRadius; ++i)
    {
        float2 offset = float2(0, gTexelSizeY * i * 2.0);
        result += tScene.Sample(sLinear, uv + offset) * kGaussWeights[i];
        result += tScene.Sample(sLinear, uv - offset) * kGaussWeights[i];
    }

    return result;
}

// ============================================================================
// Pass 4: 合成
// ============================================================================

cbuffer CompositeConstants : register(b0)
{
    float gDummy;
    float gCompPadding[3];
};

Texture2D tSharp   : register(t0);   // シャープ元画像 (full-res HDR)
Texture2D tBlurred : register(t1);   // ブラー済み画像 (half-res HDR)
Texture2D tCoC     : register(t2);   // CoC map (full-res R16_FLOAT)

SamplerState sCompLinear : register(s0);  // ブラー画像のアップサンプル用
SamplerState sCompPoint  : register(s1);  // シャープ/CoC のポイントサンプル用

float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;

    float4 sharp   = tSharp.Sample(sCompPoint, uv);
    float4 blurred = tBlurred.Sample(sCompLinear, uv);
    float  coc     = tCoC.Sample(sCompPoint, uv).r;

    // CoC値でシャープとブラーをlerp
    // coc=0: 完全にシャープ（フォーカス内）
    // coc=1: 完全にブラー（フォーカス外）
    float4 result = lerp(sharp, blurred, coc);
    result.a = 1.0;

    return result;
}
