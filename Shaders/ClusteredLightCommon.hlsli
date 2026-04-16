/// @file ClusteredLightCommon.hlsli
/// @brief クラスタードライティング共通定義

#ifndef CLUSTERED_LIGHT_COMMON_HLSLI
#define CLUSTERED_LIGHT_COMMON_HLSLI

struct ClusterInfo
{
    uint offset;
    uint count;
};

#ifdef CLUSTERED_LIGHTING

StructuredBuffer<ClusterInfo> gClusterInfoSRV  : register(t16);
StructuredBuffer<uint>        gLightIndicesSRV : register(t17);

/// @brief スクリーン座標と深度からクラスタインデックスを計算する
uint GetClusterIndex(float2 screenPos, float viewZ, float2 screenSize,
                     uint clusterCountX, uint clusterCountY, uint clusterCountZ,
                     float nearZ, float farZ)
{
    uint cx = uint(screenPos.x / screenSize.x * float(clusterCountX));
    uint cy = uint(screenPos.y / screenSize.y * float(clusterCountY));

    // Exponential depth slice
    float logRatio = log(farZ / nearZ);
    uint cz = uint(log(viewZ / nearZ) / logRatio * float(clusterCountZ));

    cx = min(cx, clusterCountX - 1);
    cy = min(cy, clusterCountY - 1);
    cz = min(cz, clusterCountZ - 1);

    return cz * clusterCountX * clusterCountY + cy * clusterCountX + cx;
}

#endif // CLUSTERED_LIGHTING

#endif // CLUSTERED_LIGHT_COMMON_HLSLI
