/// @file SSGITemporal.hlsl
/// @brief SSGI テンポラルリプロジェクション / リゾルブシェーダー
///
/// 現フレームのSSGI結果と前フレームのヒストリを深度ベースのリプロジェクションで
/// ブレンドし、フレーム間のノイズを低減する。近傍クランプでゴーストを防止する。

#include "Fullscreen.hlsli"

cbuffer TemporalCB : register(b0)
{
    float4x4 g_PrevViewProj;
    float4x4 g_InvViewProj;
    float    g_Alpha;
    float    g_ScreenWidth;
    float    g_ScreenHeight;
    float    g_HasHistory;
};

Texture2D<float4> g_CurrentGI  : register(t0);
Texture2D<float4> g_History    : register(t1);
Texture2D<float>  g_Depth      : register(t2);
SamplerState      g_Linear     : register(s0);

/// @brief テンポラルリゾルブPS
/// 現フレームSSGI + 前フレームヒストリをリプロジェクション付きで混合する。
/// 近傍3x3クランプでゴースティングアーティファクトを防止。
float4 PSTemporalResolve(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 current = g_CurrentGI.Sample(g_Linear, uv);

    // ヒストリが無い初回フレームは現フレーム結果をそのまま返す
    if (g_HasHistory < 0.5)
        return current;

    // 深度から現フレームのワールド座標を再構成する
    float depth = g_Depth.Sample(g_Linear, uv).r;

    // 空ピクセル (depth >= 1.0) はテンポラルブレンド不要
    if (depth >= 1.0)
        return current;

    // クリップ空間座標を構築 (UV -> NDC)
    float4 clipPos = float4(uv.x * 2.0 - 1.0,
                            (1.0 - uv.y) * 2.0 - 1.0,
                            depth,
                            1.0);

    // ワールド座標に逆変換
    float4 worldPos = mul(g_InvViewProj, clipPos);
    worldPos /= worldPos.w;

    // 前フレームのクリップ空間にリプロジェクション
    float4 prevClip = mul(g_PrevViewProj, worldPos);
    float2 prevUV = float2(prevClip.x / prevClip.w * 0.5 + 0.5,
                           0.5 - prevClip.y / prevClip.w * 0.5);

    // 画面外にリプロジェクトされた場合は現フレーム結果のみ使用
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0)
        return current;

    float4 history = g_History.Sample(g_Linear, prevUV);

    // 近傍3x3クランプ: ゴーストを防止するためヒストリを現フレームの
    // 近傍カラーレンジにクランプする
    float2 texelSize = 1.0 / float2(g_ScreenWidth, g_ScreenHeight);
    float4 minC = current;
    float4 maxC = current;

    [unroll]
    for (int dy = -1; dy <= 1; dy++)
    {
        [unroll]
        for (int dx = -1; dx <= 1; dx++)
        {
            float4 s = g_CurrentGI.Sample(g_Linear, uv + float2(dx, dy) * texelSize);
            minC = min(minC, s);
            maxC = max(maxC, s);
        }
    }
    history = clamp(history, minC, maxC);

    // 指数移動平均で現フレームとヒストリをブレンド
    return lerp(history, current, g_Alpha);
}
