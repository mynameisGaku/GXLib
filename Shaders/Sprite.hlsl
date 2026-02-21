/// @file Sprite.hlsl
/// @brief スプライト描画用シェーダー
///
/// 【初学者向け解説】
/// スプライトとは、2Dゲームで使われる画像のことです。
/// このシェーダーは、正射影変換でスクリーン座標をGPU座標に変換し、
/// テクスチャの色と頂点カラーを掛け合わせて最終的な色を決定します。

// 定数バッファ: 正射影行列（スクリーン座標→クリップ座標変換）
cbuffer TransformCB : register(b0)
{
    float4x4 projectionMatrix;
};

// テクスチャとサンプラー
Texture2D    tex : register(t0);
SamplerState smp : register(s0);

// 頂点シェーダー入力
struct VSInput
{
    float2 pos   : POSITION;    // スクリーン座標（ピクセル単位）
    float2 uv    : TEXCOORD;    // テクスチャUV座標
    float4 color : COLOR;       // 頂点カラー（乗算用）
};

// ピクセルシェーダー入力
struct PSInput
{
    float4 pos   : SV_Position; // クリップ空間座標
    float2 uv    : TEXCOORD;    // テクスチャUV座標
    float4 color : COLOR;       // 頂点カラー
};

/// @brief スプライトVS — スクリーン座標を正射影行列でクリップ座標に変換
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.pos   = mul(projectionMatrix, float4(input.pos, 0.0f, 1.0f));
    output.uv    = input.uv;
    output.color = input.color;
    return output;
}

/// @brief スプライトPS — テクスチャ色と頂点カラーを乗算して出力
float4 PSMain(PSInput input) : SV_Target
{
    return tex.Sample(smp, input.uv) * input.color;
}
