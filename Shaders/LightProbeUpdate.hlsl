/// @file LightProbeUpdate.hlsl
/// @brief DDGIプローブの放射照度・深度アトラスを更新するコンピュートシェーダー。

cbuffer ProbeUpdateCB : register(b0)
{
    float3 GridOrigin;
    float  _pad0;
    float3 GridSpacing;
    float  Hysteresis;
    uint3  GridDimensions;
    int    RaysPerProbe;
    int    ProbeCount;
    int    IrradianceTexWidth;
    int    DepthTexWidth;
    float  _pad1;
};

RWTexture2D<float4> IrradianceAtlas : register(u0);
RWTexture2D<float2> DepthAtlas : register(u1);

// 球面Fibonacci分布による均一球面サンプリング
float3 SphericalFibonacci(int index, int total)
{
    float phi = (1.0 + sqrt(5.0)) * 0.5;  // 黄金比
    float theta = 2.0 * 3.14159265 * float(index) / phi;
    float cosAlpha = 1.0 - (2.0 * float(index) + 1.0) / float(total);
    float sinAlpha = sqrt(1.0 - cosAlpha * cosAlpha);
    return float3(sinAlpha * cos(theta), cosAlpha, sinAlpha * sin(theta));
}

// 3Dインデックスからプローブのワールド位置を取得
float3 GetProbeWorldPos(uint3 probeIdx)
{
    return GridOrigin + float3(probeIdx) * GridSpacing;
}

// プローブのリニアインデックスを3Dグリッドインデックスに変換
uint3 ProbeIndexTo3D(uint idx)
{
    uint z = idx / (GridDimensions.x * GridDimensions.y);
    uint rem = idx % (GridDimensions.x * GridDimensions.y);
    uint y = rem / GridDimensions.x;
    uint x = rem % GridDimensions.x;
    return uint3(x, y, z);
}

[numthreads(64, 1, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint probeIndex = dtid.x;
    if (probeIndex >= (uint)ProbeCount) return;

    uint3 probeIdx3D = ProbeIndexTo3D(probeIndex);
    float3 probePos = GetProbeWorldPos(probeIdx3D);

    // 球面上のレイトレースで放射照度を計算
    float3 irradiance = float3(0, 0, 0);
    float totalWeight = 0.0;
    float meanDepth = 0.0;
    float meanDepthSq = 0.0;

    for (int r = 0; r < RaysPerProbe; r++)
    {
        float3 rayDir = SphericalFibonacci(r, RaysPerProbe);

        // 簡略化: スカイライトの寄与を仮定
        // 完全な実装ではシーンジオメトリに対してトレースする
        float skyFactor = max(0.0, rayDir.y);
        float3 sampleColor = float3(0.6, 0.7, 0.9) * skyFactor * 0.5;

        float weight = max(0.0, rayDir.y); // コサイン重み付け
        irradiance += sampleColor * weight;
        totalWeight += weight;

        // 深度（簡略化: 一定距離またはスカイでのヒットを仮定）
        float hitDistance = rayDir.y > 0.1 ? 100.0 : 5.0;
        meanDepth += hitDistance;
        meanDepthSq += hitDistance * hitDistance;
    }

    if (totalWeight > 0.0)
        irradiance /= totalWeight;

    meanDepth /= float(RaysPerProbe);
    meanDepthSq /= float(RaysPerProbe);

    // ヒステリシス付きで放射照度アトラスに書き込み
    // このプローブのアトラス座標を計算
    int probesPerRow = IrradianceTexWidth / 6;  // k_IrradianceProbeSize = 6（プローブ1つ分のテクセル）
    int atlasX = (int(probeIndex) % probesPerRow) * 6;
    int atlasY = (int(probeIndex) / probesPerRow) * 6;

    // プローブの中心テクセルに書き込み（簡略化 - 完全実装では6x6全体に書き込む）
    int cx = atlasX + 3;
    int cy = atlasY + 3;

    float4 prevIrr = IrradianceAtlas[int2(cx, cy)];
    float4 newIrr = float4(irradiance, 1.0);
    IrradianceAtlas[int2(cx, cy)] = lerp(newIrr, prevIrr, Hysteresis);

    // 深度アトラスに書き込み
    int dProbesPerRow = DepthTexWidth / 14;  // k_DepthProbeSize = 14（深度プローブ1つ分のテクセル）
    int dAtlasX = (int(probeIndex) % dProbesPerRow) * 14 + 7;
    int dAtlasY = (int(probeIndex) / dProbesPerRow) * 14 + 7;

    float2 prevDepth = DepthAtlas[int2(dAtlasX, dAtlasY)];
    float2 newDepth = float2(meanDepth, meanDepthSq);
    DepthAtlas[int2(dAtlasX, dAtlasY)] = lerp(newDepth, prevDepth, Hysteresis);
}
