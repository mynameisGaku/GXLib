/// @file Primitive.hlsl
/// @brief プリミティブ描画用シェーダー
///
/// 【初学者向け解説】
/// プリミティブ描画とは、線分、矩形、円などの基本図形を描画することです。
/// テクスチャは使わず、頂点カラーだけで色を決定します。

// 定数バッファ: 正射影行列
cbuffer TransformCB : register(b0)
{
    float4x4 projectionMatrix;
};

// 頂点シェーダー入力
struct VSInput
{
    float2 pos   : POSITION;    // スクリーン座標（ピクセル単位）
    float4 color : COLOR;       // 頂点カラー
};

// ピクセルシェーダー入力
struct PSInput
{
    float4 pos   : SV_Position; // クリップ空間座標
    float4 color : COLOR;       // 頂点カラー
};

/// @brief プリミティブVS — スクリーン座標を正射影行列で変換
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.pos   = mul(projectionMatrix, float4(input.pos, 0.0f, 1.0f));
    output.color = input.color;
    return output;
}

/// @brief プリミティブPS — 頂点カラーをそのまま出力
float4 PSMain(PSInput input) : SV_Target
{
    return input.color;
}
