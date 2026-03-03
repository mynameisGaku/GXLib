/// @file ClusteredLightAssign.hlsl
/// @brief クラスタライト割当 -- AABB-球テストで各クラスタのライトリストを構築

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
    float2   gScreenSize;
    uint     gMaxLightsPerCluster;
    float    _pad;
};

/// @brief クラスタのAABBをビュー空間で計算する
void GetClusterAABB(uint3 clusterIdx, out float3 aabbMin, out float3 aabbMax)
{
    float clusterSizeX = gScreenSize.x / float(gClusterCountX);
    float clusterSizeY = gScreenSize.y / float(gClusterCountY);

    // Screen-space bounds
    float2 minScreen = float2(clusterIdx.x, clusterIdx.y) * float2(clusterSizeX, clusterSizeY);
    float2 maxScreen = minScreen + float2(clusterSizeX, clusterSizeY);

    // Depth slice (exponential distribution)
    float nearSlice = gNearZ * pow(gFarZ / gNearZ, float(clusterIdx.z) / float(gClusterCountZ));
    float farSlice  = gNearZ * pow(gFarZ / gNearZ, float(clusterIdx.z + 1) / float(gClusterCountZ));

    // Convert screen to NDC
    float2 ndcMin = minScreen / gScreenSize * 2.0 - 1.0;
    float2 ndcMax = maxScreen / gScreenSize * 2.0 - 1.0;
    ndcMin.y = -ndcMin.y;
    ndcMax.y = -ndcMax.y;

    aabbMin = float3(min(ndcMin, ndcMax) * farSlice, nearSlice);
    aabbMax = float3(max(ndcMin, ndcMax) * farSlice, farSlice);
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
