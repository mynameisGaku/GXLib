/// @file VolumetricClouds.hlsl
/// @brief UE5スタイル ボリュメトリッククラウド — マルチオクターブ散乱レイマーチング
///
/// Frostbite/Nubis/UE5で使われるマルチオクターブ散乱近似を実装。
/// Beer-Powder効果、Dual-Lobe HG位相関数、大気遠近法を含む。
/// v3: 3Dノイズテクスチャサンプリング + テンポラル用PSCloudOnly追加。

#include "Fullscreen.hlsli"

// ─── 定数バッファ ─────────────────────────────────────────────
cbuffer CloudConstants : register(b0)
{
    float4x4 invViewProjection;
    float3   cameraPosition;
    float    time;
    float3   sunDirection;     // 正規化済み、太陽に向かう方向（C++側で反転済み）
    float    cloudBottom;
    float3   sunColor;
    float    cloudTop;
    float    coverage;
    float    densityMul;
    float    windSpeed;
    float    silverLining;
    float3   windDirection;
    int      marchSteps;
    int      lightSteps;
    float2   screenDimensions;
    float    _pad0;
    // --- マルチオクターブ散乱 ---
    int      msOctaves;
    float    msAttenuation;
    float    msContribution;
    float    msEccentricity;
    float    powderAmount;
    float    ambientBottom;
    float    ambientTop;
    float    atmosphereDensity;
    int      useNoiseTextures;
    float    _pad1[3];
};

Texture2D<float4>   sceneTexture       : register(t0);
Texture2D<float>    depthTexture       : register(t1);
Texture3D<float4>   baseNoiseTexture   : register(t2);
Texture3D<float4>   detailNoiseTexture : register(t3);
SamplerState        linearSampler      : register(s0);
SamplerState        wrapSampler        : register(s1);

// ─── 定数 ────────────────────────────────────────────────────
static const float PI = 3.14159265;
static const float ISOTROPIC_PHASE = 1.0 / (4.0 * PI);

// ─── ハッシュ / ノイズ（プロシージャルフォールバック用）────────

float hash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float3 hash3(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float worley(float3 p)
{
    float3 id = floor(p);
    float3 f  = frac(p);
    float minDist = 1.0;
    [unroll] for (int x = -1; x <= 1; x++)
    [unroll] for (int y = -1; y <= 1; y++)
    [unroll] for (int z = -1; z <= 1; z++)
    {
        float3 offset = float3(x, y, z);
        float3 h = hash3(id + offset);
        float3 d = offset + h - f;
        minDist = min(minDist, dot(d, d));
    }
    return sqrt(minDist);
}

float valueNoise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(lerp(hash(i + float3(0,0,0)), hash(i + float3(1,0,0)), f.x),
             lerp(hash(i + float3(0,1,0)), hash(i + float3(1,1,0)), f.x), f.y),
        lerp(lerp(hash(i + float3(0,0,1)), hash(i + float3(1,0,1)), f.x),
             lerp(hash(i + float3(0,1,1)), hash(i + float3(1,1,1)), f.x), f.y),
        f.z);
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

// Interleaved Gradient Noise（バンディング防止ジッター）
float interleavedGradientNoise(float2 pos)
{
    return frac(52.9829189 * frac(0.06711056 * pos.x + 0.00583715 * pos.y));
}

// ─── ユーティリティ: Nubis-style リマップ ──────────────────────

float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + (value - oldMin) / max(oldMax - oldMin, 0.0001) * (newMax - newMin);
}

// ─── 雲密度サンプリング（共通前処理）─────────────────────────

struct CloudSampleContext
{
    float heightFraction;
    float heightGradient;
    float3 samplePos;
};

CloudSampleContext prepareCloudSample(float3 pos)
{
    CloudSampleContext ctx;
    float cloudThickness = max(cloudTop - cloudBottom, 1.0);
    ctx.heightFraction = saturate((pos.y - cloudBottom) / cloudThickness);
    ctx.heightGradient = smoothstep(0.0, 0.07, ctx.heightFraction)
                       * smoothstep(1.0, 0.25, ctx.heightFraction);
    float3 windOffset = windDirection * windSpeed * time * 0.01;
    ctx.samplePos = pos * 0.0003 + windOffset;
    return ctx;
}

// ─── 雲密度サンプリング（フル品質）──────────────────────────

float sampleCloudDensity(float3 pos)
{
    CloudSampleContext ctx = prepareCloudSample(pos);

    float shape;
    if (useNoiseTextures)
    {
        float4 baseN = baseNoiseTexture.SampleLevel(wrapSampler, ctx.samplePos, 0);
        shape = saturate(baseN.r * 2.0);
    }
    else
    {
        shape = fbm(ctx.samplePos, 4);
    }

    // カバレッジリマップ
    float base = saturate((shape - (1.0 - coverage)) / max(coverage, 0.001));

    // 完全に空の空間のみスキップ（閾値を限りなく低くしてエッジフリッカー防止）
    if (base <= 0.0)
        return 0.0;

    // ディテール侵食（Worley） — エッジを複雑にしてシャープに見せる
    float detail;
    if (useNoiseTextures)
        detail = detailNoiseTexture.SampleLevel(wrapSampler, ctx.samplePos * 4.0 + float3(0, time * 0.003, 0), 0).r;
    else
        detail = worley(ctx.samplePos * 4.0 + float3(0, time * 0.003, 0));

    float detailStrength = 0.35 * lerp(0.3, 1.0, ctx.heightFraction);
    base = saturate(base - detail * detailStrength);

    // エッジコントラスト強調: 低密度域を引き締めて輪郭をくっきりさせる
    float density = base * ctx.heightGradient * densityMul;
    density *= smoothstep(0.0, 0.02, density);

    return density;
}

// ─── 雲密度サンプリング（LOD — lightMarch用軽量版）──────────

float sampleCloudDensityLOD(float3 pos)
{
    CloudSampleContext ctx = prepareCloudSample(pos);

    // 3オクターブFBM（Worleyなし） — lightMarchでは形状ディテールのみ、侵食不要
    float shape;
    if (useNoiseTextures)
    {
        float4 baseN = baseNoiseTexture.SampleLevel(wrapSampler, ctx.samplePos, 0);
        shape = saturate(baseN.r * 2.0);
    }
    else
    {
        shape = fbm(ctx.samplePos, 3);
    }

    float base = saturate((shape - (1.0 - coverage)) / max(coverage, 0.001));
    return base * ctx.heightGradient * densityMul;
}

// ─── Henyey-Greenstein 位相関数 ──────────────────────────────

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

// ─── Dual-Lobe HG位相関数 + Silver Lining ────────────────────

float dualLobeHG(float cosTheta, float g)
{
    float forward  = HenyeyGreenstein(cosTheta, g);
    float backward = HenyeyGreenstein(cosTheta, -g * 0.4);
    float silver   = HenyeyGreenstein(cosTheta, 0.99) * silverLining * 0.005;
    return lerp(forward, backward, 0.2) + silver;
}

// ─── 大気散乱による太陽色補正 ────────────────────────────────

float3 ComputeAtmosphericSunColor(float3 baseSunColor, float sunElevation)
{
    float airMass = 1.0 / max(sunElevation, 0.04);
    float3 rayleighOD = float3(0.06, 0.12, 0.28) * min(airMass, 30.0);
    return baseSunColor * exp(-rayleighOD);
}

// ─── ライトマーチ ────────────────────────────────────────────

float lightMarch(float3 pos)
{
    float stepSize = (cloudTop - cloudBottom) / max((float)lightSteps, 1.0);
    float opticalDepth = 0.0;

    [loop]
    for (int i = 0; i < lightSteps; ++i)
    {
        pos += sunDirection * stepSize;
        if (pos.y > cloudTop || pos.y < cloudBottom) break;
        opticalDepth += max(sampleCloudDensityLOD(pos), 0.0) * stepSize;
    }
    return opticalDepth;
}

// ─── レイとクラウドレイヤーの交差判定 ─────────────────────────

float2 intersectCloudLayer(float3 origin, float3 dir)
{
    if (abs(dir.y) < 0.0001)
    {
        if (origin.y >= cloudBottom && origin.y <= cloudTop)
            return float2(0.0, 100000.0);
        return float2(-1.0, -1.0);
    }

    float tBot = (cloudBottom - origin.y) / dir.y;
    float tTop = (cloudTop    - origin.y) / dir.y;

    float tMin = max(min(tBot, tTop), 0.0);
    float tMax = max(tBot, tTop);

    return (tMin > tMax) ? float2(-1.0, -1.0) : float2(tMin, tMax);
}

// ─── レイマーチコア ──────────────────────────────────────────

struct CloudResult
{
    float3 lightEnergy;
    float  transmittance;
};

CloudResult traceCloud(float2 uv, float3 rayDir, float rawDepth, float sceneDistance)
{
    CloudResult r;
    r.lightEnergy = float3(0, 0, 0);
    r.transmittance = 1.0;

    float2 tRange = intersectCloudLayer(cameraPosition, rayDir);
    if (tRange.x < 0.0)
        return r;

    // シーンジオメトリの手前で雲をクリップ
    if (rawDepth < 0.999)
        tRange.y = min(tRange.y, sceneDistance);
    if (tRange.x >= tRange.y)
        return r;

    // ライティング準備
    float3 atmSunColor = ComputeAtmosphericSunColor(sunColor, max(sunDirection.y, 0.0));
    float cosTheta = dot(rayDir, sunDirection);

    float3 warmAmbient = float3(0.6, 0.4, 0.25);
    float3 coolAmbient = float3(0.35, 0.45, 0.6);
    float3 ambientColor = lerp(warmAmbient, coolAmbient, saturate(sunDirection.y));

    // レイマーチ
    float stepSize = (tRange.y - tRange.x) / max((float)marchSteps, 1.0);
    float jitter = interleavedGradientNoise(uv * screenDimensions);
    float t = tRange.x + stepSize * jitter;

    [loop]
    for (int i = 0; i < marchSteps; ++i)
    {
        if (r.transmittance < 0.01) break;

        float3 pos = cameraPosition + rayDir * t;
        float density = sampleCloudDensity(pos);

        if (density > 0.00001)
        {
            float opticalDepth = lightMarch(pos);

            // マルチオクターブ散乱
            float3 scatterLuminance = float3(0, 0, 0);
            float oAtten   = 1.0;
            float oContrib = 1.0;
            float oEccen   = 1.0;

            [unroll]
            for (int oct = 0; oct < 8; ++oct)
            {
                if (oct >= msOctaves) break;

                float beer   = exp(-opticalDepth * oAtten);
                float powder = 1.0 - exp(-opticalDepth * oAtten * 2.0);
                float beerPowder = beer * lerp(1.0, powder, powderAmount);

                float phaseVal = lerp(ISOTROPIC_PHASE,
                                      dualLobeHG(cosTheta, 0.76 * oEccen),
                                      oEccen);

                scatterLuminance += oContrib * beerPowder * phaseVal * atmSunColor;

                oAtten   *= msAttenuation;
                oContrib *= msContribution;
                oEccen   *= msEccentricity;
            }

            // 高度依存アンビエント
            float hFrac = saturate((pos.y - cloudBottom) / max(cloudTop - cloudBottom, 1.0));
            float ambStr = lerp(ambientBottom, ambientTop, hFrac);
            float3 ambient = ambientColor * ambStr;

            float3 lightContrib = scatterLuminance + ambient;
            float extinction = density * stepSize;
            float sampleTransmittance = exp(-extinction);

            r.lightEnergy += lightContrib * (1.0 - sampleTransmittance) * r.transmittance;
            r.transmittance *= sampleTransmittance;
        }

        t += stepSize;
    }

    // 大気遠近法
    float cloudDistance = tRange.x;
    float fogFactor = 1.0 - exp(-cloudDistance * atmosphereDensity);
    float3 fogColor = ambientColor * 0.6;
    r.lightEnergy = lerp(r.lightEnergy, fogColor * (1.0 - r.transmittance), fogFactor);
    r.transmittance = lerp(r.transmittance, 1.0, fogFactor * 0.5);

    return r;
}

// ─── PSMain — フルスクリーン合成（従来パス）──────────────────

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    float4 sceneColor = sceneTexture.Sample(linearSampler, input.uv);

    // ワールド空間レイ再構築
    float2 ndc = float2(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0);
    float4 worldFar = mul(float4(ndc, 1.0, 1.0), invViewProjection);
    worldFar /= worldFar.w;
    float3 rayDir = normalize(worldFar.xyz - cameraPosition);

    // シーン深度
    float rawDepth = depthTexture.Sample(linearSampler, input.uv);
    float4 worldDepthPos = mul(float4(ndc, rawDepth, 1.0), invViewProjection);
    worldDepthPos /= worldDepthPos.w;
    float sceneDistance = length(worldDepthPos.xyz - cameraPosition);

    CloudResult r = traceCloud(input.uv, rayDir, rawDepth, sceneDistance);

    float3 finalColor = sceneColor.rgb * r.transmittance + r.lightEnergy;
    return float4(finalColor, r.transmittance);
}

// ─── PSCloudOnly — クラウドのみ出力（テンポラルパス用）────────

float4 PSCloudOnly(FullscreenVSOutput input) : SV_Target
{
    // ワールド空間レイ再構築
    float2 ndc = float2(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0);
    float4 worldFar = mul(float4(ndc, 1.0, 1.0), invViewProjection);
    worldFar /= worldFar.w;
    float3 rayDir = normalize(worldFar.xyz - cameraPosition);

    // シーン深度
    float rawDepth = depthTexture.Sample(linearSampler, input.uv);
    float4 worldDepthPos = mul(float4(ndc, rawDepth, 1.0), invViewProjection);
    worldDepthPos /= worldDepthPos.w;
    float sceneDistance = length(worldDepthPos.xyz - cameraPosition);

    CloudResult r = traceCloud(input.uv, rayDir, rawDepth, sceneDistance);

    // RGB = light energy, A = transmittance
    return float4(r.lightEnergy, r.transmittance);
}
