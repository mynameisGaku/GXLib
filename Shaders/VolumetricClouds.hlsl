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
    float    coverageVariation;  // 0=均一, 1=空間変調（積雲風）
    float    cloudType;          // 0=stratus, 0.5=stratocumulus, 1.0=cumulus
    // --- AC7品質向上パラメータ ---
    float    diffusivity;        // エッジ鋭さ (0=sharp, 1=soft)
    float    upperDensity;       // 雲頂スカルプティング
    float    shadowDarkness;     // 底面暗化強度
    float    shadowBias;         // ライトマーチ初期バイアス
    float    atmosphereBlueShift; // Rayleigh青シフト
    float    ambientSkyR;        // 動的アンビエント色R
    float    ambientSkyG;        // 動的アンビエント色G
    float    ambientSkyB;        // 動的アンビエント色B
    float    detailFadeDistance;  // ディテール消失距離
    float3   _pad1;              // パディング
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

// ─── マクロ配置マスク（プロシージャル天候テクスチャ）──────────────
//
// 低周波FBMで大規模な雲域/晴天域を定義。
// 商用エンジンの天候テクスチャ（2Dテクスチャ）を手続き的に代替する。
// coverage → 閾値（全域の雲量制御）
// coverageVariation → 遷移の鋭さ（積雲群 vs 一面曇り）

float getMacroMask(float3 pos)
{
    float2 macroWind = windDirection.xz * windSpeed * time * 0.002;
    float2 macroUV = pos.xz * 0.00004 + macroWind;

    // 3オクターブFBM（Y固定で2D評価 — プロシージャル天候マップ）
    float macroNoise = fbm(float3(macroUV, 0.5), 3);
    macroNoise = saturate(macroNoise * 1.15);  // [0,0.875] → [0,1]

    // coverage→閾値: coverage=1で全域雲, coverage=0で全域晴れ
    float threshold = 1.0 - coverage;

    // coverageVariation→遷移幅: 高=くっきり積雲群, 低=なだらか一面曇り
    float spread = lerp(0.25, 0.05, coverageVariation);

    return smoothstep(threshold - spread, threshold + spread, macroNoise);
}

// ─── 雲密度サンプリング（フル品質）──────────────────────────
//
// 商用品質パイプライン（Nubis/Schneider準拠）:
//   Step 1: マクロ配置マスク（天候テクスチャ代替）
//   Step 2: 高度プロファイル（cloudType別）
//   Step 3: 3Dシェイプノイズ（ドメインワーピング付き）
//   Step 4: マクロ×シェイプ結合 + ソフト閾値
//   Step 5: Remap侵食（商用標準 — フワフワ積雲エッジ）

float sampleCloudDensity(float3 pos)
{
    // ── Step 1: マクロ配置マスク（天候テクスチャ代替）──
    float macroMask = getMacroMask(pos);
    if (macroMask <= 0.001) return 0.0;

    // ── Step 2: 高度プロファイル ──
    float cloudThickness = max(cloudTop - cloudBottom, 1.0);
    float hf = saturate((pos.y - cloudBottom) / cloudThickness);

    float4 gradStratus  = float4(0.02, 0.05, 0.09, 0.11);
    float4 gradStratoCu = float4(0.02, 0.07, 0.35, 0.55);
    float4 gradCumulus  = float4(0.01, 0.08, 0.70, 0.95);
    float smallW  = 1.0 - saturate(cloudType * 2.0);
    float mediumW = 1.0 - abs(cloudType - 0.5) * 2.0;
    float largeW  = saturate(cloudType - 0.5) * 2.0;
    float4 grad = gradStratus * smallW + gradStratoCu * mediumW + gradCumulus * largeW;
    float heightGrad = smoothstep(grad.x, grad.y, hf) - smoothstep(grad.z, grad.w, hf);

    // ドーム変調 — 上に行くほど密度が減衰（積雲らしいドーム形状）
    float domeShape = 1.0 - hf * hf;
    heightGrad *= lerp(1.0, domeShape, cloudType);

    // macroMaskベース高さフェード（エッジ雲を低くする）
    float heightCap = lerp(0.3, 1.0, macroMask);
    heightGrad *= 1.0 - smoothstep(heightCap * 0.7, heightCap, hf);

    if (heightGrad <= 0.0) return 0.0;

    // ── Step 3: 3Dシェイプノイズ（中周波数, ~2km特徴）──
    float3 windOff = windDirection * windSpeed * time * 0.01;
    float3 samplePos = pos * 0.00012 + windOff;
    samplePos += hf * windDirection * 0.15;   // 高度シアー

    float shape;
    if (useNoiseTextures)
    {
        // ドメインワーピング
        float3 warpVal = baseNoiseTexture.SampleLevel(wrapSampler, samplePos * 0.3, 0).gba;
        float3 warpedPos = samplePos + (warpVal - 0.5) * 0.15;

        float4 baseN = baseNoiseTexture.SampleLevel(wrapSampler, warpedPos, 0);
        float lowFreq = baseN.g * 0.625 + baseN.b * 0.25 + baseN.a * 0.125;
        shape = remap(baseN.r, lowFreq - 1.0, 1.0, 0.0, 1.0);
    }
    else
    {
        shape = fbm(samplePos, 4);
    }

    // ── Step 4: Nubis準拠カバレッジ閾値 ──
    // macroMask = 空間的に変動するカバレッジ（天候テクスチャ相当）
    // remap(shape, 1-cov, 1, 0, 1) → 閾値以下のノイズをカット
    float density = saturate(remap(shape, 1.0 - macroMask, 1.0, 0.0, 1.0));
    if (density <= 0.0) return 0.0;

    // *= coverage: 雲域内は macroMask≈0.7-1.0 なので密度を保持
    density *= macroMask;

    // 高度プロファイル適用
    density *= heightGrad;

    // ── Step 5: ディテール侵食（距離適応 + Diffusivity制御）──
    if (useNoiseTextures)
    {
        float sampleDist = length(pos - cameraPosition);
        float detailWeight = 1.0 - saturate(sampleDist / detailFadeDistance);

        if (detailWeight > 0.01)
        {
            // Curl風の位置歪み（ディテールの乱流感）
            float3 curlOffset = baseNoiseTexture.SampleLevel(wrapSampler, samplePos * 2.5, 0).gba;
            float3 detailPos = samplePos * 6.0 + float3(0, time * 0.003, 0);
            detailPos += (curlOffset - 0.5) * 0.1 * saturate(1.0 - hf);

            float4 detN = detailNoiseTexture.SampleLevel(wrapSampler, detailPos, 0);
            float detail = detN.r * 0.625 + detN.g * 0.25 + detN.b * 0.125;

            // 高さ依存方向反転: 底=滑らか(1-detail), 頂=モコモコ(detail)
            float detailMod = lerp(1.0 - detail, detail, saturate(hf * 5.0));

            // Diffusivity制御: low = aggressive erosion = sharp edges
            float erosionStrength = lerp(0.45, 0.20, diffusivity) * detailWeight;
            density = remap(density, detailMod * erosionStrength, 1.0, 0.0, 1.0);
            density = max(0.0, density);
        }
    }

    // ── Step 6: Upper Density — 雲頂カリフラワーディテール ──
    if (upperDensity > 0.001 && useNoiseTextures)
    {
        float topMask = smoothstep(0.5, 0.9, hf);
        float4 upperN = detailNoiseTexture.SampleLevel(wrapSampler,
                         samplePos * 3.5 + float3(time * 0.001, 0, 0), 0);
        float upperDetail = upperN.r * 0.6 + upperN.g * 0.4;
        density += topMask * upperDensity * upperDetail * macroMask * 0.5;
    }

    return density * densityMul;
}

// ─── 雲密度サンプリング（LOD — lightMarch用軽量版）──────────
//
// マクロマスク含む。ドメインワーピング・ディテール侵食はスキップ。

float sampleCloudDensityLOD(float3 pos)
{
    float macroMask = getMacroMask(pos);
    if (macroMask <= 0.001) return 0.0;

    float cloudThickness = max(cloudTop - cloudBottom, 1.0);
    float hf = saturate((pos.y - cloudBottom) / cloudThickness);

    // 高度プロファイル（同一）
    float4 gradStratus  = float4(0.02, 0.05, 0.09, 0.11);
    float4 gradStratoCu = float4(0.02, 0.07, 0.35, 0.55);
    float4 gradCumulus  = float4(0.01, 0.08, 0.70, 0.95);
    float smallW  = 1.0 - saturate(cloudType * 2.0);
    float mediumW = 1.0 - abs(cloudType - 0.5) * 2.0;
    float largeW  = saturate(cloudType - 0.5) * 2.0;
    float4 grad = gradStratus * smallW + gradStratoCu * mediumW + gradCumulus * largeW;
    float heightGrad = smoothstep(grad.x, grad.y, hf) - smoothstep(grad.z, grad.w, hf);

    // ドーム変調 — 上に行くほど密度が減衰（積雲らしいドーム形状）
    float domeShape = 1.0 - hf * hf;
    heightGrad *= lerp(1.0, domeShape, cloudType);

    // macroMaskベース高さフェード（エッジ雲を低くする）
    float heightCap = lerp(0.3, 1.0, macroMask);
    heightGrad *= 1.0 - smoothstep(heightCap * 0.7, heightCap, hf);

    if (heightGrad <= 0.0) return 0.0;

    // シェイプノイズ（ドメインワーピングなし — LOD省略）
    float3 windOff = windDirection * windSpeed * time * 0.01;
    float3 samplePos = pos * 0.00012 + windOff;
    samplePos += hf * windDirection * 0.15;

    float shape;
    if (useNoiseTextures)
    {
        float4 baseN = baseNoiseTexture.SampleLevel(wrapSampler, samplePos, 0);
        float lowFreq = baseN.g * 0.625 + baseN.b * 0.25 + baseN.a * 0.125;
        shape = remap(baseN.r, lowFreq - 1.0, 1.0, 0.0, 1.0);
    }
    else
    {
        shape = fbm(samplePos, 3);
    }

    float density = saturate(remap(shape, 1.0 - macroMask, 1.0, 0.0, 1.0));
    density *= macroMask;
    density *= heightGrad;

    return density * densityMul;
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
    float totalRange = (cloudTop - cloudBottom);
    float stepSize = totalRange * 0.5 / max((float)lightSteps, 1.0);
    float opticalDepth = shadowBias;

    [loop]
    for (int i = 0; i < lightSteps; ++i)
    {
        pos += sunDirection * stepSize;
        if (pos.y > cloudTop || pos.y < cloudBottom) break;
        opticalDepth += max(sampleCloudDensityLOD(pos), 0.0) * stepSize;
        stepSize *= 1.5;  // Cone stepping: exponential step increase
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

    // CB駆動動的アンビエント色（太陽高度ベース — C++側で計算）
    float3 ambientColor = float3(ambientSkyR, ambientSkyG, ambientSkyB);

    // レイマーチ（LOD空域スキップ + フル品質雲内サンプリング）
    float baseStepSize = (tRange.y - tRange.x) / max((float)marchSteps, 1.0);
    float jitter = interleavedGradientNoise(uv * screenDimensions);
    jitter = frac(jitter + frac(time * 7.23) * 0.61803398875);
    float t = tRange.x + baseStepSize * jitter;

    float stepSize = baseStepSize;
    int zeroCount = 0;

    [loop]
    for (int i = 0; i < marchSteps && t < tRange.y; ++i)
    {
        if (r.transmittance < 0.01) break;

        float3 pos = cameraPosition + rayDir * t;

        // 連続空域 → LODで安価にスキップ判定
        if (zeroCount >= 3)
        {
            float cheapDensity = sampleCloudDensityLOD(pos);
            if (cheapDensity <= 0.001)
            {
                stepSize = min(stepSize * 2.0, baseStepSize * 4.0);
                t += stepSize;
                continue;
            }
            // LODが密度検出 → ステップ縮小してフルサンプルへ落下
            stepSize = baseStepSize;
            zeroCount = 0;
        }

        float density = sampleCloudDensity(pos);

        if (density > 0.00001)
        {
            stepSize = baseStepSize;
            zeroCount = 0;

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

            // 高度依存アンビエント + 底面暗化
            float hFrac = saturate((pos.y - cloudBottom) / max(cloudTop - cloudBottom, 1.0));
            float ambStr = lerp(ambientBottom, ambientTop, hFrac);
            float bottomDarken = 1.0 - shadowDarkness * (1.0 - hFrac) * (1.0 - hFrac);
            float3 ambient = ambientColor * ambStr * bottomDarken;

            float3 lightContrib = scatterLuminance + ambient;
            float extinction = density * stepSize;
            float sampleTransmittance = exp(-extinction);

            r.lightEnergy += lightContrib * (1.0 - sampleTransmittance) * r.transmittance;
            r.transmittance *= sampleTransmittance;
        }
        else
        {
            zeroCount++;
        }

        t += stepSize;
    }

    // 大気遠近法（Rayleigh波長依存散乱）
    float cloudDistance = tRange.x;
    float fogFactor = 1.0 - exp(-cloudDistance * atmosphereDensity);
    // Rayleigh: 赤は遠くまで届き、青は散乱で失われる → 遠景雲が青みを帯びる
    float3 rayleighCoeff = float3(0.06, 0.12, 0.28);
    float3 extinction = exp(-rayleighCoeff * cloudDistance * atmosphereDensity * 15.0);
    // inscattering: 大気が青い光を加える
    float3 inscatter = (1.0 - extinction) * ambientColor;
    float3 neutralFogColor = ambientColor * 0.6;
    float3 fogColor = lerp(neutralFogColor, inscatter, atmosphereBlueShift);
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
