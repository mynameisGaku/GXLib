/// @file ClothRender.hlsl
/// @brief Cloth mesh rendering -- vertex fetch from particle buffer

struct ClothVertex
{
    float3 position;
    float  inverseMass;
    float3 prevPosition;
    float  _pad0;
    float3 acceleration;
    float  _pad1;
};

StructuredBuffer<ClothVertex> gParticles : register(t0);

cbuffer ClothRenderCB : register(b0)
{
    float4x4 gViewProj;
    float3   gLightDir;
    float    _pad;
    float4   gClothColor;
    uint     gWidth;
    uint     gHeight;
    float2   _pad2;
};

struct VSInput
{
    uint vertexID : SV_VertexID;
};

struct PSInput
{
    float4 posH   : SV_Position;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD;
};

// -----------------------------------------------------------------------
// Vertex Shader
// -----------------------------------------------------------------------
PSInput VSMain(VSInput input)
{
    PSInput output;

    // Compute grid coordinates from vertex ID (triangle list for quad grid)
    uint quadIdx    = input.vertexID / 6;
    uint vertInQuad = input.vertexID % 6;
    uint qx = quadIdx % (gWidth - 1);
    uint qy = quadIdx / (gWidth - 1);

    // Two triangles per quad: (0,0)(1,0)(0,1) and (1,0)(1,1)(0,1)
    uint2 offsets[6] = {
        uint2(0, 0), uint2(1, 0), uint2(0, 1),
        uint2(1, 0), uint2(1, 1), uint2(0, 1)
    };
    uint2 coord = uint2(qx, qy) + offsets[vertInQuad];
    uint particleIdx = coord.y * gWidth + coord.x;

    float3 pos = gParticles[particleIdx].position;
    output.posH = mul(float4(pos, 1.0), gViewProj);

    // Compute face normal from neighboring particles
    uint maxIdx = gWidth * gHeight - 1;
    float3 right = gParticles[min(particleIdx + 1, maxIdx)].position - pos;
    float3 down  = gParticles[min(particleIdx + gWidth, maxIdx)].position - pos;
    output.normal = normalize(cross(right, down));

    // UV from grid coordinates
    output.uv = float2(coord) / float2(gWidth - 1, gHeight - 1);

    return output;
}

// -----------------------------------------------------------------------
// Pixel Shader
// -----------------------------------------------------------------------
float4 PSMain(PSInput input) : SV_Target0
{
    float NdotL = saturate(dot(input.normal, -gLightDir));
    float3 lit = gClothColor.rgb * (NdotL * 0.8 + 0.2); // Simple diffuse + ambient
    return float4(lit, gClothColor.a);
}
