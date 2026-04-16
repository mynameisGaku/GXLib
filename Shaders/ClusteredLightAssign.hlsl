/// @file ClusteredLightAssign.hlsl
/// @brief クラスタライト割当 -- AABB-球テストで各クラスタのライトリストを構築
///
/// 各スレッドグループが1つのクラスタ（フロクセル）を担当し、全ライトとの
/// 交差判定を行ってマッチするライトインデックスをリストに書き込む。
/// 深度スライスは対数分割（exponential depth slicing）を使用。

struct LightData
{
    float3 position;
    float  range;
    float3 direction;
    float  spotAngle;
    float3 color;
    float  intensity;
    uint   type;
    float3 _pad;
};

struct ClusterInfo
{
    uint offset;
    uint count;
};

StructuredBuffer<LightData>      gLights       : register(t0);
RWStructuredBuffer<ClusterInfo>  gClusterInfo  : register(u0);
RWStructuredBuffer<uint>         gLightIndices : register(u1);

cbuffer ClusterCB : register(b0)
{
    float4x4 gView;
    float4x4 gProjection;
    float    gNearZ;
    float    gFarZ;
    uint     gNumLights;
    uint     gClusterCountX;
    uint     gClusterCountY;
    uint     gClusterCountZ;
    float    gScreenWidth;
    float    gScreenHeight;
    uint     gMaxLightsPerCluster;
    float    _pad;
};

/// @brief スクリーンUVと深度からビュー空間座標を復元する
/// @param uv   スクリーン空間UV [0,1]
/// @param z    ビュー空間深度 (正値)
/// @return ビュー空間座標
float3 ScreenToView(float2 uv, float z)
{
    // UV → NDC: x = uv.x * 2 - 1, y = 1 - uv.y * 2 (Y反転)
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);

    // Projection行列からtanHalfFovとアスペクトを抽出
    // Projection._11 = 1/(aspect*tan(fov/2)), Projection._22 = 1/tan(fov/2)
    float tanHalfFovY = 1.0 / gProjection._22;
    float tanHalfFovX = 1.0 / gProjection._11;

    return float3(ndc.x * tanHalfFovX * z,
                  ndc.y * tanHalfFovY * z,
                  z);
}

/// @brief クラスタのAABBをビュー空間で計算する
/// @param clusterIdx クラスタ3D座標
/// @param aabbMin    出力: AABB最小点
/// @param aabbMax    出力: AABB最大点
void GetClusterAABB(uint3 clusterIdx, out float3 aabbMin, out float3 aabbMax)
{
    // スクリーン空間UV [0,1]
    float2 uv0 = float2(clusterIdx.xy) / float2(gClusterCountX, gClusterCountY);
    float2 uv1 = float2(clusterIdx.xy + uint2(1, 1)) / float2(gClusterCountX, gClusterCountY);

    // 対数深度スライス: depth = near * (far/near)^(z/numZ)
    float depthRatio = gFarZ / gNearZ;
    float nearSlice = gNearZ * pow(depthRatio, float(clusterIdx.z) / float(gClusterCountZ));
    float farSlice  = gNearZ * pow(depthRatio, float(clusterIdx.z + 1) / float(gClusterCountZ));

    // 4つの角をビュー空間に変換し、near/farスライスの両方で評価
    float3 p0 = ScreenToView(float2(uv0.x, uv0.y), nearSlice);
    float3 p1 = ScreenToView(float2(uv1.x, uv0.y), nearSlice);
    float3 p2 = ScreenToView(float2(uv0.x, uv1.y), nearSlice);
    float3 p3 = ScreenToView(float2(uv1.x, uv1.y), nearSlice);

    float3 p4 = ScreenToView(float2(uv0.x, uv0.y), farSlice);
    float3 p5 = ScreenToView(float2(uv1.x, uv0.y), farSlice);
    float3 p6 = ScreenToView(float2(uv0.x, uv1.y), farSlice);
    float3 p7 = ScreenToView(float2(uv1.x, uv1.y), farSlice);

    // 8頂点からAABBを構成
    aabbMin = min(min(min(p0, p1), min(p2, p3)), min(min(p4, p5), min(p6, p7)));
    aabbMax = max(max(max(p0, p1), max(p2, p3)), max(max(p4, p5), max(p6, p7)));
}

/// @brief 球とAABBの交差判定
bool SphereAABBIntersect(float3 center, float radius, float3 aabbMin, float3 aabbMax)
{
    float3 closest = clamp(center, aabbMin, aabbMax);
    float3 delta = center - closest;
    return dot(delta, delta) <= radius * radius;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint3 clusterIdx = dtid;
    if (clusterIdx.x >= gClusterCountX ||
        clusterIdx.y >= gClusterCountY ||
        clusterIdx.z >= gClusterCountZ)
        return;

    float3 aabbMin, aabbMax;
    GetClusterAABB(clusterIdx, aabbMin, aabbMax);

    uint clusterLinearIdx = clusterIdx.z * gClusterCountX * gClusterCountY
                          + clusterIdx.y * gClusterCountX + clusterIdx.x;
    uint baseOffset = clusterLinearIdx * gMaxLightsPerCluster;
    uint count = 0;

    for (uint i = 0; i < gNumLights && count < gMaxLightsPerCluster; ++i)
    {
        LightData light = gLights[i];

        if (light.type == 0) // Directional -- affects all clusters
        {
            gLightIndices[baseOffset + count] = i;
            count++;
        }
        else
        {
            // Transform light position to view space
            float3 lightPosView = mul(float4(light.position, 1.0), gView).xyz;

            if (SphereAABBIntersect(lightPosView, light.range, aabbMin, aabbMax))
            {
                gLightIndices[baseOffset + count] = i;
                count++;
            }
        }
    }

    gClusterInfo[clusterLinearIdx].offset = baseOffset;
    gClusterInfo[clusterLinearIdx].count  = count;
}
