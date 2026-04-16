/// @file ContactShadows.hlsl
/// @brief Screen Space Contact Shadows
///
/// CSMが表現できない細部のコンタクトシャドウをスクリーン空間レイマーチで近似する。
/// バイラテラル法線復元 + smoothstep N·L フェード + IGNディザリングで
/// 自己遮蔽とNaN伝播を防止する。
///
/// 4つのパスを含む:
/// - PSGenerate: 深度からコンタクトシャドウを計算（スクリーン空間マーチ）
/// - PSBlurH / PSBlurV: ガウシアンブラー
/// - PSComposite: シャドウマスクを乗算合成用に出力

#include "Fullscreen.hlsli"

// ============================================================================
// コンタクトシャドウ生成パス
// ============================================================================

cbuffer ContactShadowConstants : register(b0)
{
    float4x4 gView;            // ビュー行列
    float4x4 gProjection;      // プロジェクション行列
    float4x4 gInvProjection;   // 逆プロジェクション行列
    float3   gLightDirView;    // ビュー空間でのライト方向（正規化済み）
    float    gMaxDistance;      // マーチ最大距離（ビュー空間）
    float    gScreenWidth;     // スクリーン幅
    float    gScreenHeight;    // スクリーン高さ
    int      gStepCount;       // マーチステップ数
    float    gThickness;       // 厚み閾値
    float    gIntensity;       // シャドウ強度
    float    gNearZ;           // ニアクリップ
    float    gFarZ;            // ファークリップ
    float    gPadding;
};

Texture2D    tDepth  : register(t0);
SamplerState sPoint  : register(s0);

/// @brief Interleaved Gradient Noise (Jimenez 2014)
float InterleavedGradientNoise(float2 screenPos)
{
    return frac(52.9829189 * frac(dot(screenPos, float2(0.06711056, 0.00583715))));
}

/// @brief NDC深度からビュー空間Zを取得
/// gInvProjection の Z列のみ使用（フル行列乗算不要）
float LinearDepth(float ndcDepth)
{
    float4 v = mul(float4(0, 0, ndcDepth, 1), gInvProjection);
    return v.z / v.w;
}

/// @brief 深度値からビュー空間位置を復元
float3 ReconstructViewPos(float2 uv, float depth)
{
    float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
    ndc.y = -ndc.y;
    float4 viewPos = mul(ndc, gInvProjection);
    return viewPos.xyz / viewPos.w;
}

/// @brief ビュー空間座標をスクリーン空間（UV + NDC深度）に射影
float3 ProjectToScreen(float3 viewPos)
{
    float4 clip = mul(float4(viewPos, 1.0), gProjection);
    clip.xyz /= clip.w;
    float2 uv = clip.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;
    return float3(uv, clip.z);
}

/// @brief コンタクトシャドウ生成
///
/// 1. バイラテラル法線復元（ddx/ddyのNaN問題を回避）
/// 2. smoothstep N·L フェードで浅い角度の自己遮蔽を軽減
/// 3. スクリーン空間レイマーチで均一サンプリング密度を実現
/// 4. 深度差ベースの連続ペナンブラ計算でバンディング防止
float4 PSGenerate(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = tDepth.Sample(sPoint, uv).r;

    if (depth >= 1.0)
        return float4(1, 1, 1, 1);

    float3 viewPos = ReconstructViewPos(uv, depth);

    // ── バイラテラル法線復元（SSAO.hlsl と同方式）──
    // ddx/ddy は深度不連続面で NaN を生むため使用しない
    float2 texelSize = float2(1.0 / gScreenWidth, 1.0 / gScreenHeight);

    float depthR = tDepth.Sample(sPoint, uv + float2( texelSize.x, 0)).r;
    float depthL = tDepth.Sample(sPoint, uv + float2(-texelSize.x, 0)).r;
    float depthD = tDepth.Sample(sPoint, uv + float2(0,  texelSize.y)).r;
    float depthU = tDepth.Sample(sPoint, uv + float2(0, -texelSize.y)).r;

    float3 viewPosR = ReconstructViewPos(uv + float2( texelSize.x, 0), depthR);
    float3 viewPosL = ReconstructViewPos(uv + float2(-texelSize.x, 0), depthL);
    float3 viewPosD = ReconstructViewPos(uv + float2(0,  texelSize.y), depthD);
    float3 viewPosU = ReconstructViewPos(uv + float2(0, -texelSize.y), depthU);

    // 深度差が小さい側を選択（エッジ跨ぎ防止）
    float3 ddx_ = (abs(depthR - depth) < abs(depthL - depth))
        ? (viewPosR - viewPos) : (viewPos - viewPosL);
    float3 ddy_ = (abs(depthD - depth) < abs(depthU - depth))
        ? (viewPosD - viewPos) : (viewPos - viewPosU);

    float3 N = normalize(cross(ddx_, ddy_));

    // カメラ方向を向くように修正（cross積の向き不定性解消）
    if (dot(N, viewPos) > 0.0)
        N = -N;

    // ── 滑らかな N·L フェード ──
    float NdotL = dot(N, -gLightDirView);
    if (NdotL <= 0.0)
        return float4(1, 1, 1, 1);

    // 浅い角度ではシャドウ強度を段階的に減衰（自己遮蔽を軽減）
    float angleFade = smoothstep(0.0, 0.15, NdotL);

    // ── スクリーン空間レイマーチ ──
    // ビュー空間でレイ終点を計算し、スクリーン空間に射影
    float3 rayEndView = viewPos + (-gLightDirView) * gMaxDistance;
    float3 screenEnd   = ProjectToScreen(rayEndView);
    float3 screenStart = float3(uv, depth);

    float3 rayStep = (screenEnd - screenStart) / (float)gStepCount;

    // フルステップIGNジッター（[0,1)で完全なサブステップオフセット）
    float noise = InterleavedGradientNoise(input.pos.xy);
    float3 rayPos = screenStart + rayStep * noise;

    float occlusion = 0.0;

    [loop]
    for (int i = 0; i < gStepCount; ++i)
    {
        rayPos += rayStep;

        // スクリーン外チェック
        if (rayPos.x < 0.0 || rayPos.x > 1.0 ||
            rayPos.y < 0.0 || rayPos.y > 1.0 ||
            rayPos.z < 0.0 || rayPos.z > 1.0)
            break;

        // NDC深度比較: レイが深度バッファより奥にある場合
        float sampledDepth = tDepth.Sample(sPoint, rayPos.xy).r;
        if (rayPos.z > sampledDepth)
        {
            // ビュー空間の厚みチェック（LinearDepth で正確に比較）
            float sceneZ = LinearDepth(sampledDepth);
            float rayZ   = LinearDepth(rayPos.z);
            float depthDiff = rayZ - sceneZ;

            // 深度比例バイアス: 微小な自己交差を排除
            float bias = viewPos.z * 0.002;

            if (depthDiff > bias && depthDiff < gThickness)
            {
                // 深度差ベースの連続ペナンブラ（離散フェードを排除）
                float thicknessFade = 1.0 - saturate(depthDiff / gThickness);
                float t = (float)(i + 1) / (float)gStepCount;
                float distanceFade = 1.0 - t * t;
                occlusion = max(occlusion, thicknessFade * distanceFade);
            }
        }
    }

    float shadow = 1.0 - gIntensity * angleFade * occlusion;
    return float4(shadow, shadow, shadow, 1.0);
}

// ============================================================================
// ガウシアンブラー（IGNノイズ除去用、2倍スプレッドで実効半径8テクセル）
// ============================================================================

cbuffer BlurConstants : register(b0)
{
    float2 gBlurDirection;  // ブラー方向 (水平: 1/w,0 / 垂直: 0,1/h)
    float2 gBlurPadding;
};

Texture2D    tSource     : register(t0);
SamplerState sBlurPoint  : register(s0);

static const float kGaussWeights[5] = { 0.227027, 0.194596, 0.121621, 0.054054, 0.016216 };
static const float kBlurSpread = 2.0;  // サンプル間隔2テクセル → 実効半径8テクセル

/// @brief 水平ガウシアンブラー — IGNノイズパターンを確実に平滑化
float4 PSBlurH(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float result = tSource.Sample(sBlurPoint, uv).r * kGaussWeights[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(gBlurDirection.x * i * kBlurSpread, 0);

        float s1 = tSource.Sample(sBlurPoint, uv + offset).r;
        float s2 = tSource.Sample(sBlurPoint, uv - offset).r;

        result += (s1 + s2) * kGaussWeights[i];
    }

    return float4(result, result, result, 1.0);
}

/// @brief 垂直ガウシアンブラー — 水平ブラーと同一アルゴリズムの垂直方向版
float4 PSBlurV(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float result = tSource.Sample(sBlurPoint, uv).r * kGaussWeights[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(0, gBlurDirection.y * i * kBlurSpread);

        float s1 = tSource.Sample(sBlurPoint, uv + offset).r;
        float s2 = tSource.Sample(sBlurPoint, uv - offset).r;

        result += (s1 + s2) * kGaussWeights[i];
    }

    return float4(result, result, result, 1.0);
}

// ============================================================================
// 合成パス (MultiplyBlend PSOで使用)
// ============================================================================

/// @brief シャドウマスク合成パス — MultiplyBlend PSOでHDRシーンに乗算合成
float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float shadow = tSource.Sample(sBlurPoint, input.uv).r;
    return float4(shadow, shadow, shadow, 1.0);
}
