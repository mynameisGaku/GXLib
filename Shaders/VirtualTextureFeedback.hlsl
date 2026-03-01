/// @file VirtualTextureFeedback.hlsl
/// @brief Virtual texture feedback buffer generation

cbuffer VTConstants : register(b0)
{
    uint g_TextureId;
    uint g_PageSize;
    uint g_VirtualWidth;
    uint g_VirtualHeight;
};

Texture2D<float4> g_SceneUV : register(t0);
RWStructuredBuffer<uint4> g_FeedbackBuffer : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixelCoord = dtid.xy;
    if (pixelCoord.x >= g_VirtualWidth || pixelCoord.y >= g_VirtualHeight)
        return;

    float2 uv = g_SceneUV[pixelCoord].xy;

    uint tileX = uint(uv.x * g_VirtualWidth) / g_PageSize;
    uint tileY = uint(uv.y * g_VirtualHeight) / g_PageSize;

    // Calculate mip level from screen-space derivatives (simplified)
    uint mipLevel = 0;

    uint outputIndex = pixelCoord.y * (g_VirtualWidth / g_PageSize) + pixelCoord.x;
    g_FeedbackBuffer[outputIndex] = uint4(g_TextureId, mipLevel, tileX, tileY);
}
