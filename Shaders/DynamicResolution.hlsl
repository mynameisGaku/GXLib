/// @file DynamicResolution.hlsl
/// @brief CAS (Contrast Adaptive Sharpening) upscale shader

cbuffer CASConstants : register(b0)
{
    float2 g_InputSize;
    float2 g_OutputSize;
    float  g_Sharpness;  // 0.0 = no sharpening, 1.0 = max
    float3 g_Pad;
};

Texture2D<float4> g_Input : register(t0);
RWTexture2D<float4> g_Output : register(u0);
SamplerState g_LinearSampler : register(s0);

float3 CASFilter(float2 uv, float2 texelSize)
{
    // 5-tap cross neighborhood
    float3 center = g_Input.SampleLevel(g_LinearSampler, uv, 0).rgb;
    float3 top    = g_Input.SampleLevel(g_LinearSampler, uv + float2(0, -texelSize.y), 0).rgb;
    float3 bottom = g_Input.SampleLevel(g_LinearSampler, uv + float2(0,  texelSize.y), 0).rgb;
    float3 left   = g_Input.SampleLevel(g_LinearSampler, uv + float2(-texelSize.x, 0), 0).rgb;
    float3 right  = g_Input.SampleLevel(g_LinearSampler, uv + float2( texelSize.x, 0), 0).rgb;

    // Local min/max for contrast calculation
    float3 minColor = min(center, min(min(top, bottom), min(left, right)));
    float3 maxColor = max(center, max(max(top, bottom), max(left, right)));

    // Adaptive sharpening weight
    float3 contrast = maxColor - minColor;
    float3 weight = saturate(1.0 - contrast * g_Sharpness);

    // Sharpened result
    float3 avg = (top + bottom + left + right) * 0.25;
    return lerp(avg, center, 1.0 + weight * g_Sharpness);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= uint(g_OutputSize.x) || dtid.y >= uint(g_OutputSize.y))
        return;

    float2 uv = (float2(dtid.xy) + 0.5) / g_OutputSize;
    float2 texelSize = 1.0 / g_InputSize;

    float3 color = CASFilter(uv, texelSize);
    g_Output[dtid.xy] = float4(color, 1.0);
}
