/// @file ComputeSkinning.hlsl
/// @brief GPU Compute Shader スキニング
///
/// 静的メッシュ頂点とボーン行列から、スキニング済み頂点を計算する。

struct SkinnedVertex
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float4 tangent;
    uint4  boneIndices;
    float4 boneWeights;
};

struct OutputVertex
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float4 tangent;
};

StructuredBuffer<SkinnedVertex> InputVertices : register(t0);
StructuredBuffer<float4x4>     BoneMatrices   : register(t1);
RWStructuredBuffer<OutputVertex> OutputVertices : register(u0);

cbuffer Constants : register(b0)
{
    uint g_VertexCount;
    uint g_BonesPerVertex;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_VertexCount) return;

    SkinnedVertex v = InputVertices[id.x];

    float4 skinnedPos    = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);
    float4 skinnedTan    = float4(0, 0, 0, 0);

    float4 pos = float4(v.position, 1.0);
    float4 nrm = float4(v.normal, 0.0);
    float4 tan = float4(v.tangent.xyz, 0.0);

    uint indices[4] = { v.boneIndices.x, v.boneIndices.y, v.boneIndices.z, v.boneIndices.w };
    float weights[4] = { v.boneWeights.x, v.boneWeights.y, v.boneWeights.z, v.boneWeights.w };

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        if (i >= g_BonesPerVertex) break;
        float w = weights[i];
        if (w <= 0.0) continue;

        float4x4 bone = BoneMatrices[indices[i]];
        skinnedPos    += mul(pos, bone) * w;
        skinnedNormal += mul(nrm, bone).xyz * w;
        skinnedTan    += mul(tan, bone) * w;
    }

    OutputVertex outV;
    outV.position = skinnedPos.xyz;
    outV.normal   = normalize(skinnedNormal);
    outV.texCoord = v.texCoord;
    outV.tangent  = float4(normalize(skinnedTan.xyz), v.tangent.w);

    OutputVertices[id.x] = outV;
}
