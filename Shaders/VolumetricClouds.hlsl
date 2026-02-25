/// @file VolumetricClouds.hlsl
/// @brief Volumetric Clouds - Raymarching pixel shader
///
/// Fullscreen pass that raymarches through a cloud layer defined by
/// bottom/top altitude. Uses FBM noise for cloud density and
/// Beer-Lambert law for light scattering.

#include "Fullscreen.hlsli"

cbuffer CloudConstants : register(b0)
{
    float4x4 invViewProjection;
    float3   cameraPosition;
    float    time;
    float3   sunDirection;   // normalised, towards the sun
    float    cloudBottom;    // world-space altitude of cloud bottom
    float3   sunColor;
    float    cloudTop;       // world-space altitude of cloud top
    float    coverage;       // 0-1 cloud coverage
    float    densityMul;     // density multiplier
    float    windSpeed;
    float    silverLining;
    float3   windDirection;
    int      marchSteps;
    int      lightSteps;
    float2   screenDimensions;
};

Texture2D<float4> sceneTexture : register(t0);
Texture2D<float>  depthTexture : register(t1);
SamplerState      linearSampler : register(s0);

// --- Hash / Noise utilities ---

float hash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f); // smoothstep

    float n000 = hash(i + float3(0, 0, 0));
    float n100 = hash(i + float3(1, 0, 0));
    float n010 = hash(i + float3(0, 1, 0));
    float n110 = hash(i + float3(1, 1, 0));
    float n001 = hash(i + float3(0, 0, 1));
    float n101 = hash(i + float3(1, 0, 1));
    float n011 = hash(i + float3(0, 1, 1));
    float n111 = hash(i + float3(1, 1, 1));

    float n00 = lerp(n000, n100, f.x);
    float n01 = lerp(n001, n101, f.x);
    float n10 = lerp(n010, n110, f.x);
    float n11 = lerp(n011, n111, f.x);

    float n0 = lerp(n00, n10, f.y);
    float n1 = lerp(n01, n11, f.y);

    return lerp(n0, n1, f.z);
}

float fbm(float3 p, int octaves)
{
    float value = 0.0;
    float amp   = 0.5;
    float freq  = 1.0;
    for (int i = 0; i < octaves; ++i)
    {
        value += amp * valueNoise(p * freq);
        freq  *= 2.0;
        amp   *= 0.5;
    }
    return value;
}

// --- Cloud density ---

float sampleCloudDensity(float3 pos)
{
    // Normalise altitude within cloud layer
    float heightFraction = saturate((pos.y - cloudBottom) / max(cloudTop - cloudBottom, 0.001));

    // Vertical profile: rounded shape (thicker in middle)
    float heightGradient = 4.0 * heightFraction * (1.0 - heightFraction);

    // Wind-driven offset
    float3 windOffset = windDirection * windSpeed * time * 0.01;
    float3 samplePos = pos * 0.0003 + windOffset;

    // FBM base shape (4 octaves)
    float shape = fbm(samplePos, 4);

    // Coverage remap: shift noise to control how much sky is covered
    float base = saturate(shape - (1.0 - coverage)) / max(coverage, 0.01);

    // Detail noise (higher frequency, subtle)
    float detail = fbm(samplePos * 3.0 + float3(0, time * 0.005, 0), 3);
    base = saturate(base - detail * 0.3);

    return base * heightGradient * densityMul;
}

// --- Light march (Beer-Lambert in-scattering) ---

float lightMarch(float3 pos)
{
    float stepSize = (cloudTop - cloudBottom) / max((float)lightSteps, 1.0);
    float density = 0.0;

    [loop]
    for (int i = 0; i < lightSteps; ++i)
    {
        pos += sunDirection * stepSize;

        // Stop if above cloud layer
        if (pos.y > cloudTop) break;

        density += max(sampleCloudDensity(pos), 0.0) * stepSize;
    }

    // Beer-Lambert transmittance
    float transmittance = exp(-density * 0.5);

    // Silver lining: forward scattering (Henyey-Greenstein approximation)
    float silver = exp(-density * 0.1) * silverLining;

    return transmittance + silver;
}

// --- Ray-layer intersection ---

// Returns (tMin, tMax) for a ray hitting the horizontal slab [yBottom, yTop]
float2 intersectCloudLayer(float3 origin, float3 dir)
{
    // Avoid division by near-zero
    if (abs(dir.y) < 0.0001)
    {
        if (origin.y >= cloudBottom && origin.y <= cloudTop)
            return float2(0.0, 100000.0);
        return float2(-1.0, -1.0);
    }

    float tBot = (cloudBottom - origin.y) / dir.y;
    float tTop = (cloudTop    - origin.y) / dir.y;

    float tMin = min(tBot, tTop);
    float tMax = max(tBot, tTop);

    tMin = max(tMin, 0.0);

    if (tMin > tMax)
        return float2(-1.0, -1.0);

    return float2(tMin, tMax);
}

// --- Main pixel shader ---

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    // Scene colour (pass-through base)
    float4 sceneColor = sceneTexture.Sample(linearSampler, input.uv);

    // Reconstruct world-space ray
    float2 ndc = float2(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0);
    float4 worldFar = mul(float4(ndc, 1.0, 1.0), invViewProjection);
    worldFar /= worldFar.w;

    float3 rayDir = normalize(worldFar.xyz - cameraPosition);

    // Scene depth (linear)
    float rawDepth = depthTexture.Sample(linearSampler, input.uv);
    float4 worldDepthPos = mul(float4(ndc, rawDepth, 1.0), invViewProjection);
    worldDepthPos /= worldDepthPos.w;
    float sceneDistance = length(worldDepthPos.xyz - cameraPosition);

    // Intersect cloud layer
    float2 tRange = intersectCloudLayer(cameraPosition, rayDir);

    if (tRange.x < 0.0)
        return sceneColor; // Ray misses cloud layer

    // Clamp to scene depth (don't render clouds behind geometry)
    tRange.y = min(tRange.y, sceneDistance);
    if (tRange.x >= tRange.y)
        return sceneColor;

    // Ray march
    float stepSize = (tRange.y - tRange.x) / max((float)marchSteps, 1.0);
    float transmittance = 1.0;
    float3 lightEnergy = float3(0, 0, 0);

    float t = tRange.x + stepSize * 0.5; // Start at half-step

    [loop]
    for (int i = 0; i < marchSteps; ++i)
    {
        if (transmittance < 0.01)
            break;

        float3 pos = cameraPosition + rayDir * t;

        float density = sampleCloudDensity(pos);

        if (density > 0.001)
        {
            float lightTransmit = lightMarch(pos);
            float3 ambient = float3(0.4, 0.45, 0.5); // Sky ambient

            float3 lightContrib = sunColor * lightTransmit + ambient;
            float extinction = density * stepSize;
            float sampleTransmittance = exp(-extinction);

            // Energy integration (Beer-Lambert)
            lightEnergy += lightContrib * density * stepSize * transmittance;
            transmittance *= sampleTransmittance;
        }

        t += stepSize;
    }

    // Composite clouds over scene
    float3 cloudColor = lightEnergy;
    float3 finalColor = sceneColor.rgb * transmittance + cloudColor;

    return float4(finalColor, sceneColor.a);
}
