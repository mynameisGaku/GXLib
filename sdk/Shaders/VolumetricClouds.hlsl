/// @file VolumetricClouds.hlsl
/// @brief ボリュメトリッククラウド — レイマーチングピクセルシェーダー
///
/// 底面/上面高度で定義される雲レイヤーをレイマーチングするフルスクリーンパス。
/// FBMノイズで雲密度を生成し、Beer-Lambert則で光散乱を計算する。

#include "Fullscreen.hlsli"

cbuffer CloudConstants : register(b0)
{
    float4x4 invViewProjection;
    float3   cameraPosition;
    float    time;
    float3   sunDirection;   // 正規化済み、太陽方向
    float    cloudBottom;    // ワールド空間での雲底面の高度
    float3   sunColor;
    float    cloudTop;       // ワールド空間での雲上面の高度
    float    coverage;       // 0-1 雲の被覆率
    float    densityMul;     // 密度乗数
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

// --- ハッシュ / ノイズユーティリティ ---

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

// --- 雲密度 ---

float sampleCloudDensity(float3 pos)
{
    // 雲レイヤー内の高度を正規化
    float heightFraction = saturate((pos.y - cloudBottom) / max(cloudTop - cloudBottom, 0.001));

    // 垂直プロファイル: 丸みのある形状（中央が厚い）
    float heightGradient = 4.0 * heightFraction * (1.0 - heightFraction);

    // 風によるオフセット
    float3 windOffset = windDirection * windSpeed * time * 0.01;
    float3 samplePos = pos * 0.0003 + windOffset;

    // FBMベースシェイプ（4オクターブ）
    float shape = fbm(samplePos, 4);

    // 被覆率リマップ: ノイズをシフトして空の被覆量を制御
    float base = saturate(shape - (1.0 - coverage)) / max(coverage, 0.01);

    // ディテールノイズ（高周波、微細）
    float detail = fbm(samplePos * 3.0 + float3(0, time * 0.005, 0), 3);
    base = saturate(base - detail * 0.3);

    return base * heightGradient * densityMul;
}

// --- ライトマーチ（Beer-Lambert内散乱）---

float lightMarch(float3 pos)
{
    float stepSize = (cloudTop - cloudBottom) / max((float)lightSteps, 1.0);
    float density = 0.0;

    [loop]
    for (int i = 0; i < lightSteps; ++i)
    {
        pos += sunDirection * stepSize;

        // 雲レイヤーの上に出たら終了
        if (pos.y > cloudTop) break;

        density += max(sampleCloudDensity(pos), 0.0) * stepSize;
    }

    // Beer-Lambert透過率
    float transmittance = exp(-density * 0.5);

    // シルバーライニング: 前方散乱（Henyey-Greenstein近似）
    float silver = exp(-density * 0.1) * silverLining;

    return transmittance + silver;
}

// --- レイとレイヤーの交差判定 ---

// 水平スラブ [yBottom, yTop] にヒットするレイの (tMin, tMax) を返す
float2 intersectCloudLayer(float3 origin, float3 dir)
{
    // ゼロ近傍による除算を回避
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

// --- メインピクセルシェーダー ---

float4 PSMain(FullscreenVSOutput input) : SV_Target
{
    // シーンカラー（パススルーベース）
    float4 sceneColor = sceneTexture.Sample(linearSampler, input.uv);

    // ワールド空間レイを再構築
    float2 ndc = float2(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0);
    float4 worldFar = mul(float4(ndc, 1.0, 1.0), invViewProjection);
    worldFar /= worldFar.w;

    float3 rayDir = normalize(worldFar.xyz - cameraPosition);

    // シーン深度（リニア）
    float rawDepth = depthTexture.Sample(linearSampler, input.uv);
    float4 worldDepthPos = mul(float4(ndc, rawDepth, 1.0), invViewProjection);
    worldDepthPos /= worldDepthPos.w;
    float sceneDistance = length(worldDepthPos.xyz - cameraPosition);

    // 雲レイヤーとの交差判定
    float2 tRange = intersectCloudLayer(cameraPosition, rayDir);

    if (tRange.x < 0.0)
        return sceneColor; // レイが雲レイヤーに到達しない

    // シーン深度にクランプ（ジオメトリの背後に雲を描画しない）
    // 空ピクセル (far plane) は深度クランプをスキップ — 雲レイヤーが far plane 以遠でも描画可能にする
    if (rawDepth < 0.999)
        tRange.y = min(tRange.y, sceneDistance);
    if (tRange.x >= tRange.y)
        return sceneColor;

    // レイマーチ
    float stepSize = (tRange.y - tRange.x) / max((float)marchSteps, 1.0);
    float transmittance = 1.0;
    float3 lightEnergy = float3(0, 0, 0);

    float t = tRange.x + stepSize * 0.5; // ハーフステップから開始

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
            float3 ambient = float3(0.4, 0.45, 0.5); // 空のアンビエント

            float3 lightContrib = sunColor * lightTransmit + ambient;
            float extinction = density * stepSize;
            float sampleTransmittance = exp(-extinction);

            // エネルギー積分（Beer-Lambert）
            lightEnergy += lightContrib * density * stepSize * transmittance;
            transmittance *= sampleTransmittance;
        }

        t += stepSize;
    }

    // 雲をシーン上に合成
    float3 cloudColor = lightEnergy;
    float3 finalColor = sceneColor.rgb * transmittance + cloudColor;

    return float4(finalColor, sceneColor.a);
}
