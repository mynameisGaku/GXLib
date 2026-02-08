/// @file Primitive3D.hlsl
/// @brief 3Dプリミティブ描画シェーダー（ライン/ワイヤフレーム）

cbuffer Constants : register(b0)
{
    float4x4 gViewProjection;
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

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH  = mul(float4(input.position, 1.0f), gViewProjection);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_Target
{
    return input.color;
}
