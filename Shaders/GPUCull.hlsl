/// @file GPUCull.hlsl
/// @brief GPU frustum culling compute shader
///
/// Per-object bounding sphere frustum test. Visible objects are appended
/// to a RWStructuredBuffer and their count is tracked via an
/// RWByteAddressBuffer counter so the CPU can read back the total.

cbuffer CullConstants : register(b0)
{
    float4x4 g_ViewProj;
    uint g_ObjectCount;
    uint3 g_Pad;
};

struct MeshletBounds
{
    float3 center;
    float radius;
};

struct DrawCommand
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int  baseVertexLocation;
    uint startInstanceLocation;
    uint meshletIndex;
    uint objectIndex;
};

StructuredBuffer<MeshletBounds>    g_Bounds       : register(t0);
RWStructuredBuffer<DrawCommand>    g_DrawCommands : register(u0);
RWByteAddressBuffer                g_Counter      : register(u1);

// Extract six frustum planes from a row-major view-projection matrix.
// Each plane is stored as (A, B, C, D) where Ax+By+Cz+D=0 and
// (A,B,C) is normalised so that the distance test gives a true metric value.
void ExtractFrustumPlanes(float4x4 vp, out float4 planes[6])
{
    // Left
    planes[0] = float4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0],
                        vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
    // Right
    planes[1] = float4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0],
                        vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
    // Bottom
    planes[2] = float4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1],
                        vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
    // Top
    planes[3] = float4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1],
                        vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
    // Near
    planes[4] = float4(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
    // Far
    planes[5] = float4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2],
                        vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

    [unroll]
    for (uint i = 0; i < 6; i++)
    {
        float len = length(planes[i].xyz);
        planes[i] /= len;
    }
}

bool FrustumTest(float3 center, float radius, float4 planes[6])
{
    [unroll]
    for (uint i = 0; i < 6; i++)
    {
        float dist = dot(planes[i].xyz, center) + planes[i].w;
        if (dist < -radius)
            return false;
    }
    return true;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= g_ObjectCount)
        return;

    MeshletBounds bounds = g_Bounds[idx];

    float4 planes[6];
    ExtractFrustumPlanes(g_ViewProj, planes);

    if (FrustumTest(bounds.center, bounds.radius, planes))
    {
        // Atomically increment the visible count and use the returned
        // value as the write index into the draw command buffer.
        uint visibleIndex;
        g_Counter.InterlockedAdd(0, 1, visibleIndex);

        DrawCommand cmd;
        cmd.indexCountPerInstance = 0;
        cmd.instanceCount = 1;
        cmd.startIndexLocation = 0;
        cmd.baseVertexLocation = 0;
        cmd.startInstanceLocation = 0;
        cmd.meshletIndex = idx;
        cmd.objectIndex = idx;
        g_DrawCommands[visibleIndex] = cmd;
    }
}
