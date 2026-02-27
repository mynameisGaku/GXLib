/// @file ContactShadows.hlsl
/// @brief Screen Space Contact Shadows
///
/// CSMが表現できない細部のセルフシャドウをスクリーンスペースで近似する。
/// 各ピクセルからライト方向へスクリーンスペースでレイマーチし、
/// 深度バッファとの交差でシャドウを判定する。
///
/// 2つのパスを含む:
/// - PSGenerate: 深度からコンタクトシャドウを計算
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

/// @brief 深度値からビュー空間位置を復元
float3 ReconstructViewPos(float2 uv, float depth)
{
    float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
    ndc.y = -ndc.y;
    float4 viewPos = mul(ndc, gInvProjection);
    return viewPos.xyz / viewPos.w;
}

/// @brief ビュー空間座標をUV+深度に射影
float3 ProjectToScreen(float3 viewPos)
{
    float4 clip = mul(float4(viewPos, 1.0), gProjection);
    clip.xyz /= clip.w;
    float2 uv = clip.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;
    return float3(uv, clip.z);
}

/// @brief コンタクトシャドウ生成PS — ライト方向へレイマーチして遮蔽を検出
float4 PSGenerate(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float depth = tDepth.Sample(sPoint, uv).r;

    // スカイはシャドウなし
    if (depth >= 1.0)
        return float4(1, 1, 1, 1);

    // ビュー空間位置を復元
    float3 viewPos = ReconstructViewPos(uv, depth);

    // ライト方向の逆（ピクセルからライトに向かう方向）
    float3 rayDir = -gLightDirView;

    // レイマーチ
    float stepSize = gMaxDistance / (float)gStepCount;
    float shadow = 1.0;

    // 最初のステップを少しオフセット（ハッシュでディザリング）
    float hash = frac(sin(dot(input.pos.xy, float2(12.9898, 78.233))) * 43758.5453);
    float3 rayPos = viewPos + rayDir * stepSize * hash * 0.5;

    [loop]
    for (int i = 0; i < gStepCount; ++i)
    {
        rayPos += rayDir * stepSize;

        // スクリーン空間に射影
        float3 screenPos = ProjectToScreen(rayPos);

        // スクリーン外チェック
        if (screenPos.x < 0.0 || screenPos.x > 1.0 ||
            screenPos.y < 0.0 || screenPos.y > 1.0)
            break;

        // この位置の深度を取得
        float sampledDepth = tDepth.Sample(sPoint, screenPos.xy).r;

        // 深度比較: レイが深度バッファよりも奥にある場合 → 遮蔽
        if (screenPos.z > sampledDepth)
        {
            // ビュー空間での厚みチェック（背面からの突き抜けを防止）
            float3 occluderViewPos = ReconstructViewPos(screenPos.xy, sampledDepth);
            float depthDiff = rayPos.z - occluderViewPos.z;

            if (depthDiff > 0.0 && depthDiff < gThickness)
            {
                // 距離によるフェードアウト
                float t = (float)i / (float)gStepCount;
                float fade = 1.0 - t * t;
                shadow = 1.0 - gIntensity * fade;
                break;
            }
        }
    }

    return float4(shadow, shadow, shadow, 1.0);
}

// ============================================================================
// 合成パス (MultiplyBlend PSOで使用)
// ============================================================================

Texture2D    tSource : register(t0);
SamplerState sLinear : register(s0);

/// @brief シャドウマスク合成パス — MultiplyBlend PSOでHDRシーンに乗算合成
float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float shadow = tSource.Sample(sLinear, input.uv).r;
    return float4(shadow, shadow, shadow, 1.0);
}
