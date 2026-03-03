/// @file SSAO.hlsl
/// @brief Screen Space Ambient Occlusion
///
/// 3つのパスを含む:
/// - PSGenerate: 深度から AO を計算
/// - PSBlurH / PSBlurV: バイラテラルブラー（エッジ保持）
/// - PSComposite: AO を乗算合成用に出力

#include "Fullscreen.hlsli"

// ============================================================================
// AO生成パス
// ============================================================================

cbuffer SSAOConstants : register(b0)
{
    float4x4 gProjection;       // プロジェクション行列
    float4x4 gInvProjection;    // 逆プロジェクション行列
    float4   gSamples[64];      // 半球サンプルカーネル（接線空間）
    float    gRadius;           // サンプリング半径
    float    gBias;             // 深度比較バイアス（セルフオクルージョン防止）
    float    gPower;            // AO強調指数
    float    gScreenWidth;      // スクリーン幅
    float    gScreenHeight;     // スクリーン高さ
    float    gNearZ;            // ニアクリップ距離
    float    gFarZ;             // ファークリップ距離
    float    gPadding;
};

Texture2D    tDepth  : register(t0);
SamplerState sPoint  : register(s0);

/// @brief TBN行列用のランダム回転ベクトルを生成 — ピクセル座標からハッシュで決定論的に生成
float3 HashRotation(float2 pixelPos)
{
    // 2つのハッシュ値から回転角を生成
    float n = dot(pixelPos, float2(12.9898, 78.233));
    float hash1 = frac(sin(n) * 43758.5453);
    float hash2 = frac(sin(n * 1.5 + 7.136) * 23421.6312);
    float angle = hash1 * 6.283185; // 2*PI
    return float3(cos(angle), sin(angle), 0.0);
}

/// @brief 深度値からビュー空間位置を復元 — 逆プロジェクションでNDC→ビュー空間
float3 ReconstructViewPos(float2 uv, float depth)
{
    // NDC座標に変換
    float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
    ndc.y = -ndc.y; // D3D UV座標系の反転

    // 逆射影でビュー空間へ
    float4 viewPos = mul(ndc, gInvProjection);
    return viewPos.xyz / viewPos.w;
}

/// @brief AO生成PS — 64サンプル半球カーネルで遮蔽率を計算
float4 PSGenerate(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = tDepth.Sample(sPoint, uv).r;

    // スカイ（深度=1）はAO=1（遮蔽なし）
    if (depth >= 1.0)
        return float4(1, 1, 1, 1);

    // ビュー空間位置を復元
    float3 viewPos = ReconstructViewPos(uv, depth);

    // 4方向バイラテラル法線復元（深度不連続面でのアーティファクト軽減）
    float2 texelSize = float2(1.0 / gScreenWidth, 1.0 / gScreenHeight);

    float depthR = tDepth.Sample(sPoint, uv + float2(texelSize.x, 0)).r;
    float depthL = tDepth.Sample(sPoint, uv - float2(texelSize.x, 0)).r;
    float depthD = tDepth.Sample(sPoint, uv + float2(0, texelSize.y)).r;
    float depthU = tDepth.Sample(sPoint, uv - float2(0, texelSize.y)).r;

    float3 viewPosR = ReconstructViewPos(uv + float2(texelSize.x, 0), depthR);
    float3 viewPosL = ReconstructViewPos(uv - float2(texelSize.x, 0), depthL);
    float3 viewPosD = ReconstructViewPos(uv + float2(0, texelSize.y), depthD);
    float3 viewPosU = ReconstructViewPos(uv - float2(0, texelSize.y), depthU);

    // 深度差が小さい側を選択（エッジ跨ぎ防止）
    float3 dx = (abs(depthR - depth) < abs(depthL - depth))
        ? (viewPosR - viewPos) : (viewPos - viewPosL);
    float3 dy = (abs(depthD - depth) < abs(depthU - depth))
        ? (viewPosD - viewPos) : (viewPos - viewPosU);

    float3 normal = normalize(cross(dx, dy));

    // ランダム回転ベクトル
    float3 randomVec = HashRotation(input.pos.xy);

    // TBN行列（接線空間→ビュー空間）
    float3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    // 半球サンプリングによる遮蔽計算
    float occlusion = 0.0;
    int sampleCount = 64;

    for (int i = 0; i < sampleCount; ++i)
    {
        // カーネルサンプルを接線空間からビュー空間へ
        float3 sampleDir = mul(gSamples[i].xyz, TBN);
        float3 samplePos = viewPos + sampleDir * gRadius;

        // サンプル位置をスクリーン空間に射影
        float4 offset = mul(float4(samplePos, 1.0), gProjection);
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        offset.y = 1.0 - offset.y; // UV反転

        // サンプル位置の深度を取得
        float sampleDepth = tDepth.Sample(sPoint, offset.xy).r;
        float3 sampleViewPos = ReconstructViewPos(offset.xy, sampleDepth);

        // 遮蔽判定
        float rangeCheck = smoothstep(0.0, 1.0, gRadius / max(abs(viewPos.z - sampleViewPos.z), 0.0001));
        occlusion += (sampleViewPos.z <= samplePos.z - gBias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = 1.0 - (occlusion / (float)sampleCount);
    ao = pow(saturate(ao), gPower);

    return float4(ao, ao, ao, 1.0);
}

// ============================================================================
// バイラテラルブラー
// ============================================================================

cbuffer BlurConstants : register(b0)
{
    float2 gBlurDirection;  // ブラー方向 (水平: 1/w,0 / 垂直: 0,1/h)
    float2 gBlurPadding;
};

Texture2D    tSource     : register(t0);
SamplerState sBlurPoint  : register(s0);

static const float kGaussWeights[5] = { 0.227027, 0.194596, 0.121621, 0.054054, 0.016216 };

/// @brief 水平バイラテラルブラー — 深度差が大きい隣接ピクセルの重みを下げてエッジを保持
float4 PSBlurH(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float center = tSource.Sample(sBlurPoint, uv).r;
    float result = center * kGaussWeights[0];
    float totalWeight = kGaussWeights[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(gBlurDirection.x * i, 0);

        float s1 = tSource.Sample(sBlurPoint, uv + offset).r;
        float s2 = tSource.Sample(sBlurPoint, uv - offset).r;

        float w1 = kGaussWeights[i] * smoothstep(0.2, 0.0, abs(center - s1));
        float w2 = kGaussWeights[i] * smoothstep(0.2, 0.0, abs(center - s2));

        result += s1 * w1 + s2 * w2;
        totalWeight += w1 + w2;
    }

    result /= totalWeight;
    return float4(result, result, result, 1.0);
}

/// @brief 垂直バイラテラルブラー — 水平ブラーと同一アルゴリズムの垂直方向版
float4 PSBlurV(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float center = tSource.Sample(sBlurPoint, uv).r;
    float result = center * kGaussWeights[0];
    float totalWeight = kGaussWeights[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(0, gBlurDirection.y * i);

        float s1 = tSource.Sample(sBlurPoint, uv + offset).r;
        float s2 = tSource.Sample(sBlurPoint, uv - offset).r;

        float w1 = kGaussWeights[i] * smoothstep(0.2, 0.0, abs(center - s1));
        float w2 = kGaussWeights[i] * smoothstep(0.2, 0.0, abs(center - s2));

        result += s1 * w1 + s2 * w2;
        totalWeight += w1 + w2;
    }

    result /= totalWeight;
    return float4(result, result, result, 1.0);
}

// ============================================================================
// 合成パス (MultiplyBlend PSOで使用: Result = Dest * SrcColor)
// ============================================================================

/// @brief AO合成パス — AO値を出力（MultiplyBlend PSOでシーンに乗算合成される）
float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float ao = tSource.Sample(sBlurPoint, input.uv).r;
    return float4(ao, ao, ao, 1.0);
}
