/// @file ShadowDepth.hlsl
/// @brief 深度のみレンダリング用シェーダー（シャドウマップ生成）

cbuffer ObjectConstants : register(b0)
{
    float4x4 world;
    float4x4 worldInverseTranspose;
};

cbuffer ShadowPassConstants : register(b1)
{
    float4x4 lightViewProjection;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 tangent  : TANGENT;
};

struct PSInput
{
    float4 posH : SV_Position;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.position, 1.0f), world);
    output.posH = mul(posW, lightViewProjection);
    return output;
}

// ピクセルシェーダーは不要（深度書き込みのみ）
// ただしNull PSではコンパイルエラーになるため空PSを定義
void PSMain(PSInput input)
{
    // 深度値のみ書き込み（カラー出力なし）
}
