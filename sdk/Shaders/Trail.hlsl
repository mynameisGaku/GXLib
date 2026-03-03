/// @file Trail.hlsl
/// @brief トレイル描画シェーダー

cbuffer TrailCB : register(b0)
{
    float4x4 gViewProjection;
};

Texture2D    gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0), gViewProjection);
    output.uv       = input.uv;
    output.color    = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    return texColor * input.color;
}
