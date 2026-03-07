/// @file AutoExposure.hlsl
/// @brief 自動露出用シェーダー
///
/// PSLogLuminance: HDR scene → log(luminance) R16_FLOAT
/// PSDownsample: bilinear average downsample

#include "Fullscreen.hlsli"

cbuffer AutoExposureCB : register(b0)
{
    float4 params; // Reserved: C++側CBレイアウト維持のため削除不可
};

Texture2D<float4> gInput : register(t0);
SamplerState gLinearSampler : register(s0);

/// @brief 対数輝度変換 — HDRシーンのピクセル輝度をlog空間に変換して出力
///
/// 中央重み付け + 輝度下限クランプにより、画面端の暗領域（地面等）が
/// 平均輝度を過度に引き下げて空が白飛びする問題を防止。
float PSLogLuminance(FullscreenVSOutput input) : SV_Target
{
    float4 color = gInput.Sample(gLinearSampler, input.uv);

    // 輝度計算 (Rec. 709 luminance weights)
    float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    // 輝度下限を引き上げ: 0.0001だとlog=-9.2で暗ピクセルが平均を支配してしまう
    // 0.01 → log=-4.6 で暗領域の影響を適正範囲に抑える
    luminance = max(luminance, 0.01);

    // 中央重み付け: 画面端（特に地面）の寄与を減らす
    // UV中心(0.5,0.5)からの距離に基づくガウシアン風の重み
    float2 d = input.uv - 0.5;
    float weight = exp(-2.5 * dot(d, d));
    // 重みで輝度をブレンド: weight=0の端 → 中間灰(0.18)に寄せて中性化
    luminance = lerp(0.18, luminance, weight);

    return log(luminance);
}

/// @brief バイリニアダウンサンプル — バイリニアフィルタで4テクセル平均を取得
/// RTサイズを段階的に縮小して最終的に1x1にし、シーン平均輝度を求める。
float PSDownsample(FullscreenVSOutput input) : SV_Target
{
    return gInput.Sample(gLinearSampler, input.uv).r;
}
