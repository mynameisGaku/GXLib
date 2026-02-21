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
    float4x4 gInvProjection;   // 逆プロジェクション行列
    float    gFocalDistance;    // フォーカス距離 (ビュー空間Z)
    float    gFocalRange;      // フォーカスが鮮明な範囲
    float    gCoCScale;        // CoC最大半径 (未使用、予約)
    float    gScreenWidth;     // スクリーン幅
    float    gScreenHeight;    // スクリーン高さ
    float    gNearZ;           // ニアクリップ距離
    float    gFarZ;            // ファークリップ距離
    float    gPadding;
};

Texture2D    tDepth  : register(t0);
SamplerState sPoint  : register(s0);

/// @brief 深度値からリニアビュー空間Zを計算 — NDC→ビュー空間に逆投影
float LinearizeDepth(float depth)
{
    // NDC→ビュー空間Z
    float4 ndc = float4(0, 0, depth, 1.0);
    float4 viewPos = mul(ndc, gInvProjection);
    return viewPos.z / viewPos.w;
}

/// @brief CoC生成PS — 深度からフォーカス距離との差を計算し、ボケ量を決定
float4 PSCoC(FullscreenVSOutput input) : SV_Target
{
    float depth = tDepth.Sample(sPoint, input.uv).r;

    // スカイ (depth ≈ 1.0) は最大ブラー
    static const float k_SkyDepthThreshold = 0.9999;
    if (depth >= k_SkyDepthThreshold)
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
// 正規化済みガウス重み (合計 = 1.0)
static const float kGaussWeightSum = 0.1963 + 2.0 * (0.1745 + 0.1217 + 0.0667 + 0.0287 + 0.0097 + 0.0026);
static const float kGaussWeights[7] = {
    0.1963 / kGaussWeightSum,   // center
    0.1745 / kGaussWeightSum,   // ±1
    0.1217 / kGaussWeightSum,   // ±2
    0.0667 / kGaussWeightSum,   // ±3
    0.0287 / kGaussWeightSum,   // ±4
    0.0097 / kGaussWeightSum,   // ±5
    0.0026 / kGaussWeightSum    // ±6
};

/// @brief DoF水平ブラー — 13タップガウシアン分離フィルタ
float4 PSBlurH(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 result = tScene.Sample(sLinear, uv) * kGaussWeights[0];

    [unroll]
    for (int i = 1; i <= kKernelRadius; ++i)
    {
        float2 offset = float2(gTexelSizeX * i * 2.0, 0); // *2: half-res RTでのサンプル間隔を倍にし、ブラー半径を拡大
        result += tScene.Sample(sLinear, uv + offset) * kGaussWeights[i];
        result += tScene.Sample(sLinear, uv - offset) * kGaussWeights[i];
    }

    return result;
}

/// @brief DoF垂直ブラー — 13タップガウシアン分離フィルタ
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

/// @brief DoF合成PS — CoC値に基づいてシャープ画像とブラー画像をlerp合成
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
