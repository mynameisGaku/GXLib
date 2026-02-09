/// @file MaskDraw.hlsl
/// @brief マスク描画シェーダー
///
/// 矩形・円などのマスク形状をR8_UNORM RTに描画する。
/// gMaskValue: 書き込むマスク値 (0=透過, 1=不透明)

cbuffer MaskConstants : register(b0)
{
    float4x4 gProjection;
    float gMaskValue;
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

PSInput VSMask(VSInput input)
{
    PSInput o;
    o.pos = mul(float4(input.pos, 0.0f, 1.0f), gProjection);
    return o;
}

float PSMask(PSInput input) : SV_Target
{
    return gMaskValue;
}
