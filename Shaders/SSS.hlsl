/// @file SSS.hlsl
/// @brief Separable Subsurface Scattering (SSSSS) シェーダー
///
/// Jimenez et al. "Separable Subsurface Scattering" に基づく。
/// RGB別の異なるガウシアンカーネルでセパラブルブラーを行い、
/// スキン等の半透明マテリアルのサブサーフェスライトトランスポートを近似する。
/// 2パス構成: 水平ブラー (direction=(1,0)) → 垂直ブラー (direction=(0,1))。
/// 深度バッファ参照でピクセル毎にブラー幅を変調し、奥行き不連続での色漏れを防止する。

#include "Fullscreen.hlsli"

/// @brief SSS定数バッファ
cbuffer SSSCB : register(b0)
{
    float2 g_Direction;       ///< ブラー方向 — (1,0)で水平、(0,1)で垂直
    float  g_SSSWidth;        ///< カーネル幅（スクリーン空間）
    float  g_Strength;        ///< SSS全体強度
    float  g_ScreenWidth;     ///< スクリーン幅（ピクセル）
    float  g_ScreenHeight;    ///< スクリーン高さ（ピクセル）
    float  g_FarOverNear;     ///< far/near比率（リニア深度変換用）
    int    g_NumSamples;      ///< カーネルサンプル数 (最大25)
    float4 g_Kernel[25];      ///< .x=オフセット, .yzw=RGB重み
};

Texture2D<float4> g_Scene     : register(t0);  ///< 入力HDRシーン
Texture2D<float>  g_Depth     : register(t1);  ///< 深度バッファ
Texture2D<float4> g_Stencil   : register(t2);  ///< SSSマスク (Rチャンネル >= 0.5でSSS対象)
SamplerState      g_Linear    : register(s0);  ///< バイリニアクランプサンプラー

/// @brief セパラブルSSSブラーPS
///
/// SSSマスクで対象外ピクセルは元色をそのまま返す。
/// 対象ピクセルはRGB別カーネル重みで方向ブラーし、深度差で重みを減衰させる。
float4 PSMain(FullscreenVSOutput input) : SV_TARGET
{
    float2 uv = input.uv;

    // 元のシーンカラー
    float4 colorM = g_Scene.Sample(g_Linear, uv);

    // SSSマスクチェック: Rチャンネルが0.5未満なら非SSS対象
    float sssFlag = g_Stencil.Sample(g_Linear, uv).r;
    if (sssFlag < 0.5 || g_NumSamples <= 0)
        return colorM;

    // 中心ピクセルの深度
    float depthM = g_Depth.Sample(g_Linear, uv).r;

    // ピクセルサイズとステップ方向の計算
    float2 pixelSize = 1.0 / float2(g_ScreenWidth, g_ScreenHeight);
    float2 step = g_SSSWidth * g_Direction * pixelSize;

    // 深度ベースの補正: 近いオブジェクトほどブラーが広がる
    // （遠方のオブジェクトはスクリーン上で小さいため、ブラー幅を抑制）
    float depthCorrection = 1.0 / max(depthM, 0.001);
    step *= depthCorrection * g_Strength;

    // RGB別の重み付きブラー
    float3 resultRGB = float3(0.0, 0.0, 0.0);
    float3 totalWeight = float3(0.0, 0.0, 0.0);

    [loop] for (int i = 0; i < g_NumSamples; i++)
    {
        float offset = g_Kernel[i].x;
        float3 weight = g_Kernel[i].yzw;

        float2 sampleUV = uv + step * offset;

        // UV範囲クランプ
        sampleUV = saturate(sampleUV);

        float4 sampleColor = g_Scene.Sample(g_Linear, sampleUV);
        float sampleDepth = g_Depth.Sample(g_Linear, sampleUV).r;

        // 深度差に基づく重み減衰:
        // 異なる深度のピクセル（背景と前景の境界等）からのブラー漏れを防止。
        // 深度差が大きいほどs→0に近づき、そのサンプルの寄与を無視する。
        float depthDiff = abs(depthM - sampleDepth);
        float depthWeight = saturate(1.0 - depthDiff * 100.0);

        // RGB別の重み適用
        resultRGB += sampleColor.rgb * weight * depthWeight;
        totalWeight += weight * depthWeight;
    }

    // 正規化（ゼロ除算防止）
    resultRGB.r = totalWeight.r > 0.001 ? resultRGB.r / totalWeight.r : colorM.r;
    resultRGB.g = totalWeight.g > 0.001 ? resultRGB.g / totalWeight.g : colorM.g;
    resultRGB.b = totalWeight.b > 0.001 ? resultRGB.b / totalWeight.b : colorM.b;

    return float4(resultRGB, colorM.a);
}
