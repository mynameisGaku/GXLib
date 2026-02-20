/// @file InfiniteGrid.hlsl
/// @brief Shader-based infinite grid on Y=0 plane
///
/// Fullscreen triangle + ray-plane intersection in pixel shader.
/// Minor grid (1m), major grid (10m), axis colors (X=red, Z=blue),
/// distance-based fade, and correct depth output.

#include "Fullscreen.hlsli"

cbuffer GridCB : register(b0)
{
    float4x4 gViewProjectionInverse;
    float4x4 gViewProjection;
    float3   gCameraPos;
    float    gGridScale;       // base grid spacing (default 1.0)
    float    gFadeDistance;    // distance at which grid fades out
    float    gMajorLineEvery; // major line every N minor lines (default 10)
    float    _pad0;
    float    _pad1;
};

struct PSOutput
{
    float4 color : SV_Target;
    float  depth : SV_Depth;
};

PSOutput GridPS(FullscreenVSOutput input)
{
    PSOutput output;

    // Reconstruct world-space ray from screen UV
    float2 ndc = input.uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    float4 nearPoint = mul(float4(ndc, 0.0, 1.0), gViewProjectionInverse);
    float4 farPoint  = mul(float4(ndc, 1.0, 1.0), gViewProjectionInverse);
    nearPoint /= nearPoint.w;
    farPoint  /= farPoint.w;

    float3 rayDir = farPoint.xyz - nearPoint.xyz;

    // Intersect with Y=0 plane
    float t = -nearPoint.y / rayDir.y;

    // Discard pixels where ray doesn't hit the plane (behind camera or parallel)
    if (t < 0.0)
        discard;

    // World-space intersection point
    float3 worldPos = nearPoint.xyz + rayDir * t;

    // Grid computation using screen-space derivatives for anti-aliasing
    float2 coord = worldPos.xz / gGridScale;
    float2 derivative = fwidth(coord);

    // Minor grid lines
    float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
    float minorLine = min(grid.x, grid.y);
    float minorAlpha = 1.0 - min(minorLine, 1.0);

    // Major grid lines (every N minor lines)
    float2 coordMajor = coord / gMajorLineEvery;
    float2 derivativeMajor = derivative / gMajorLineEvery;
    float2 gridMajor = abs(frac(coordMajor - 0.5) - 0.5) / derivativeMajor;
    float majorLine = min(gridMajor.x, gridMajor.y);
    float majorAlpha = 1.0 - min(majorLine, 1.0);

    // Axis coloring: X axis (red) and Z axis (blue)
    float3 lineColor = float3(0.0, 0.0, 0.0); // black grid lines

    // Check if close to Z axis (worldPos.x near 0) - X axis line is red
    float xAxisDist = abs(worldPos.z) / (derivative.y * gGridScale);
    float xAxisAlpha = 1.0 - min(xAxisDist, 1.0);

    // Check if close to X axis (worldPos.z near 0) - Z axis line is blue
    float zAxisDist = abs(worldPos.x) / (derivative.x * gGridScale);
    float zAxisAlpha = 1.0 - min(zAxisDist, 1.0);

    // Compose color
    float3 color = lineColor;
    float alpha = minorAlpha * 0.25; // minor lines are subtle

    // Major lines override (darker black, stronger alpha)
    color = lerp(color, float3(0.0, 0.0, 0.0), majorAlpha);
    alpha = max(alpha, majorAlpha * 0.5);

    // Axis lines override (strongest)
    color = lerp(color, float3(0.8, 0.15, 0.15), zAxisAlpha * 0.9); // X-axis red (along X, z=0)
    alpha = max(alpha, zAxisAlpha * 0.8);

    color = lerp(color, float3(0.15, 0.15, 0.8), xAxisAlpha * 0.9); // Z-axis blue (along Z, x=0)
    alpha = max(alpha, xAxisAlpha * 0.8);

    // Distance-based fade
    float dist = length(worldPos - gCameraPos);
    float fade = 1.0 - smoothstep(gFadeDistance * 0.5, gFadeDistance, dist);
    alpha *= fade;

    // Discard fully transparent pixels
    if (alpha < 0.001)
        discard;

    output.color = float4(color, alpha);

    // Compute proper depth for depth testing with scene objects
    float4 clipPos = mul(float4(worldPos, 1.0), gViewProjection);
    output.depth = clipPos.z / clipPos.w;

    return output;
}
