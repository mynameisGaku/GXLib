/// @file VolumetricLight.hlsl
/// @brief ボリューメトリックライト（ゴッドレイ）シェーダー
///
/// GPU Gems 3 スクリーン空間ラディアルブラー方式。
/// 太陽スクリーン位置からラディアルブラーで光の筋を合成する。
/// ジッター + リニア深度サンプリングでエッジのジャギーを軽減。

#include "Fullscreen.hlsli"

cbuffer VolumetricLightCB : register(b0)
{
    float2 sunScreenPos;    // UV空間の太陽位置
    float  density;         // 散乱密度
    float  decay;           // 減衰
    float  weight;          // サンプルウェイト
    float  exposure;        // 露出
    int    numSamples;      // サンプル数
    float  intensity;       // 全体強度
    float3 lightColor;      // 光の色
    float  sunVisible;      // 太陽可視性 (0-1)
};

Texture2D<float4> gScene : register(t0);
Texture2D<float>  gDepth : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

// ピクセル座標からハッシュノイズ (0-1)
float Hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 sceneColor = gScene.SampleLevel(gLinearSampler, uv, 0);

    // 太陽が見えない場合はシーン色をそのまま返す
    if (sunVisible <= 0.0)
        return sceneColor;

    // ピクセルUVから太陽スクリーン位置へ向かうレイを計算
    float2 deltaTexCoord = (uv - sunScreenPos) * density / float(numSamples);

    // ジッター: ピクセル毎にレイ開始位置をランダムにずらしてバンディングを軽減
    float jitter = Hash(input.pos.xy);
    float2 sampleUV = uv - deltaTexCoord * jitter;

    float illuminationDecay = 1.0;
    float3 godRay = float3(0.0, 0.0, 0.0);

    // ラディアルブラー: 太陽方向にレイマーチ
    for (int i = 0; i < numSamples; i++)
    {
        sampleUV -= deltaTexCoord;

        // 画面外チェック
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            break;

        // リニアサンプリングでエッジを自然にブレンド + smoothstep で滑らかな遷移
        float sampleDepth = gDepth.SampleLevel(gLinearSampler, sampleUV, 0).r;
        float occlusion = smoothstep(0.99, 1.0, sampleDepth);

        // ウェイト累積
        godRay += occlusion * weight * illuminationDecay;

        // 減衰
        illuminationDecay *= decay;
    }

    // 最終色: ゴッドレイ × 露出 × 強度 × 太陽可視性 × ライト色
    float3 finalGodRay = godRay * exposure * intensity * sunVisible * lightColor;

    // シーンに加算合成
    float3 result = sceneColor.rgb + finalGodRay;
    return float4(result, sceneColor.a);
}
