/// @file Tonemapping.hlsl
/// @brief トーンマッピング + ガンマ補正
///
/// HDRリニア画像をLDR sRGB画像に変換する。
/// Reinhard / ACES / Uncharted2 の3種類を cbuffer で切り替え可能。

#include "Fullscreen.hlsli"

cbuffer TonemapConstants : register(b0)
{
    uint   gTonemapMode;   // 0=Reinhard, 1=ACES, 2=Uncharted2
    float  gExposure;      // 露出補正
    float2 gPadding;
};

Texture2D    tHDRScene : register(t0);
SamplerState sLinear   : register(s0);

// --- トーンマッピング関数 ---

/// @brief Reinhard トーンマッピング — HDR→LDRの最もシンプルな方式
float3 Reinhard(float3 color)
{
    return color / (color + 1.0f);
}

/// @brief ACES Filmic — 映画調のコントラストカーブ (Stephen Hill近似)
float3 ACESFilmic(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

/// @brief Uncharted 2 部分関数 — Hableのフィルミックカーブ (John Hable)
float3 Uncharted2Partial(float3 x)
{
    float A = 0.15f; // Shoulder Strength
    float B = 0.50f; // Linear Strength
    float C = 0.10f; // Linear Angle
    float D = 0.20f; // Toe Strength
    float E = 0.02f; // Toe Numerator
    float F = 0.30f; // Toe Denominator
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

/// @brief Uncharted 2 トーンマッピング — ホワイトポイント正規化付き
float3 Uncharted2(float3 color)
{
    float W = 11.2f; // リニアホワイトポイント（この輝度が白にマッピングされる）
    float3 curr = Uncharted2Partial(color);
    float3 whiteScale = 1.0f / Uncharted2Partial(float3(W, W, W));
    return curr * whiteScale;
}

// --- ピクセルシェーダー ---

/// @brief トーンマッピングPS — 露出補正→トーンマップ→ガンマ補正をまとめて適用
float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float3 hdrColor = tHDRScene.Sample(sLinear, input.uv).rgb;

    // 露出補正
    hdrColor *= gExposure;

    // トーンマッピング
    float3 ldrColor;
    if (gTonemapMode == 1)
        ldrColor = ACESFilmic(hdrColor);
    else if (gTonemapMode == 2)
        ldrColor = Uncharted2(hdrColor);
    else
        ldrColor = Reinhard(hdrColor);

    // ガンマ補正 (Linear → sRGB)
    ldrColor = pow(ldrColor, 1.0f / 2.2f);

    return float4(ldrColor, 1.0f);
}
