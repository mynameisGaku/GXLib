/// @file SkyAtmosphere.hlsl
/// @brief 大気散乱 — 単一散乱 Rayleigh + Mie

cbuffer SkyAtmosphereCB : register(b0)
{
    float4x4 InvViewProjection;
    float3   CameraPosition;
    float    PlanetRadius;
    float3   SunDirection;
    float    AtmosphereRadius;
    float3   SunColor;
    float    SunIntensity;
    float3   RayleighCoeff;
    float    RayleighScaleHeight;
    float    MieCoeff;
    float    MieScaleHeight;
    float    MieG;
    int      NumSteps;
    float2   ScreenDimensions;
    float2   _pad;
};

Texture2D<float> DepthTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

// フルスクリーン三角形
VSOutput VS(uint vertexId : SV_VertexID)
{
    VSOutput o;
    o.uv = float2((vertexId << 1) & 2, vertexId & 2);
    o.position = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

// レイと球の交差判定（最も近い正のtペアを返す、ミスなら(-1,-1)）
float2 RaySphereIntersect(float3 origin, float3 dir, float3 center, float radius)
{
    float3 oc = origin - center;
    float b = dot(oc, dir);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0) return float2(-1.0, -1.0);
    float sq = sqrt(disc);
    return float2(-b - sq, -b + sq);
}

float4 PS(VSOutput input) : SV_Target
{
    // ワールド空間のレイ方向を復元
    float2 ndc = input.uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    float4 clipPos = float4(ndc, 1.0, 1.0);
    float4 worldPos = mul(clipPos, InvViewProjection);
    float3 rayDir = normalize(worldPos.xyz / worldPos.w - CameraPosition);

    // 前方にジオメトリがあるか確認（不透明ジオメトリでは空をスキップ）
    float depth = DepthTexture.SampleLevel(LinearSampler, input.uv, 0);
    if (depth < 1.0) discard;

    // カメラを惑星表面に配置
    float3 earthCenter = float3(0.0, -PlanetRadius, 0.0);
    float3 rayOrigin = CameraPosition + float3(0.0, PlanetRadius, 0.0);

    // 大気との交差判定
    float2 atmoHit = RaySphereIntersect(rayOrigin, rayDir, float3(0, 0, 0), AtmosphereRadius);
    if (atmoHit.y < 0.0) discard;

    float tStart = max(atmoHit.x, 0.0);
    float tEnd = atmoHit.y;
    float stepSize = (tEnd - tStart) / float(NumSteps);

    // 数値積分
    float3 rayleighSum = float3(0, 0, 0);
    float mieSum = 0.0;
    float opticalDepthR = 0.0;
    float opticalDepthM = 0.0;

    float3 sunDir = normalize(SunDirection);

    for (int i = 0; i < NumSteps; i++)
    {
        float t = tStart + (float(i) + 0.5) * stepSize;
        float3 samplePos = rayOrigin + rayDir * t;
        float altitude = length(samplePos) - PlanetRadius;

        float hr = exp(-altitude / RayleighScaleHeight) * stepSize;
        float hm = exp(-altitude / MieScaleHeight) * stepSize;
        opticalDepthR += hr;
        opticalDepthM += hm;

        // 光線の光学的深度（簡略化: 太陽方向に4サンプル）
        float2 sunHit = RaySphereIntersect(samplePos, sunDir, float3(0, 0, 0), AtmosphereRadius);
        float sunStepSize = sunHit.y / 4.0;
        float odLightR = 0.0, odLightM = 0.0;
        for (int j = 0; j < 4; j++)
        {
            float ts = (float(j) + 0.5) * sunStepSize;
            float3 lightPos = samplePos + sunDir * ts;
            float lightAlt = length(lightPos) - PlanetRadius;
            odLightR += exp(-lightAlt / RayleighScaleHeight) * sunStepSize;
            odLightM += exp(-lightAlt / MieScaleHeight) * sunStepSize;
        }

        float3 tau = RayleighCoeff * (opticalDepthR + odLightR) +
                     MieCoeff * 1.1 * (opticalDepthM + odLightM);
        float3 attenuation = exp(-tau);

        rayleighSum += hr * attenuation;
        mieSum += hm * attenuation.x;  // use single channel for Mie
    }

    // 位相関数
    float cosTheta = dot(rayDir, sunDir);
    float cos2 = cosTheta * cosTheta;

    // Rayleigh位相関数
    float phaseR = 3.0 / (16.0 * 3.14159265) * (1.0 + cos2);

    // Henyey-Greenstein位相関数（Mie）
    float g = MieG;
    float g2 = g * g;
    float phaseM = 3.0 / (8.0 * 3.14159265) * ((1.0 - g2) * (1.0 + cos2)) /
                   ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));

    float3 color = SunIntensity * SunColor * (rayleighSum * RayleighCoeff * phaseR + mieSum * MieCoeff * phaseM);

    return float4(color, 1.0);
}
