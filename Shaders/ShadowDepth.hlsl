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

cbuffer BoneConstants : register(b4)
{
    float4x4 gBones[128];
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 tangent  : TANGENT;
#if defined(SKINNED)
    uint4  joints   : JOINTS;
    float4 weights  : WEIGHTS;
#endif
};

struct PSInput
{
    float4 posH : SV_Position;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posL = float4(input.position, 1.0f);

#if defined(SKINNED)
    float4 skinnedPos = float4(0, 0, 0, 0);
    float4 w = input.weights;
    float wSum = w.x + w.y + w.z + w.w;
    if (wSum > 0.0001f) w /= wSum;
    else w = float4(1, 0, 0, 0);

    uint4 j = input.joints;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float4x4 bone = gBones[j[i]];
        skinnedPos += mul(posL, bone) * w[i];
    }
    posL = skinnedPos;
#endif

    float4 posW = mul(posL, world);
    output.posH = mul(posW, lightViewProjection);
    return output;
}

// ピクセルシェーダーは不要（深度書き込みのみ）
// ただしNull PSではコンパイルエラーになるため空PSを定義
void PSMain(PSInput input)
{
    // 深度値のみ書き込み（カラー出力なし）
}
