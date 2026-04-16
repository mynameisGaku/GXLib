/// @file MaskDraw.hlsl
/// @brief マスク描画シェーダー
///
/// 矩形・円などのマスク形状をR8_UNORM RTに描画する。
/// gMaskValue: 書き込むマスク値 (0=透過, 1=不透明)

cbuffer MaskConstants : register(b0)
{
    float4x4 gProjection; // 正射影行列
    float gMaskValue;     // 書き込むマスク値 (0.0=透過, 1.0=不透明)
    float3 gPadding;
};

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 pos : SV_Position;
};

/// @brief マスクVS — スクリーン座標を正射影で変換
PSInput VSMask(VSInput input)
{
    PSInput o;
    o.pos = mul(float4(input.pos, 0.0f, 1.0f), gProjection);
    return o;
}

/// @brief マスクPS — 指定のマスク値をR8_UNORMに書き込む
float PSMask(PSInput input) : SV_Target
{
    return gMaskValue;
}
