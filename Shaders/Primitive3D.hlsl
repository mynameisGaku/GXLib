// 3Dプリミティブ描画シェーダー
// デバッグ描画やワイヤフレーム表示で使用する。テクスチャなし、頂点カラーのみ。

/// @file Primitive3D.hlsl
/// @brief 3Dプリミティブ描画シェーダー（ライン/ワイヤフレーム）

cbuffer Constants : register(b0)
{
    float4x4 gViewProjection; // ビュー×プロジェクション行列
};

struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct PSInput
{
    float4 posH  : SV_Position;
    float4 color : COLOR;
};

/// @brief 3DプリミティブVS — ワールド座標をVP行列でクリップ空間に変換
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH  = mul(float4(input.position, 1.0f), gViewProjection);
    output.color = input.color;
    return output;
}

/// @brief 3DプリミティブPS — 頂点カラーをそのまま出力
float4 PSMain(PSInput input) : SV_Target
{
    return input.color;
}
