#pragma once
/// @file MeshData.h
/// @brief CPU側メッシュデータ + プリミティブ生成

#include "pch.h"
#include "Graphics/3D/Vertex3D.h"

namespace GX
{

/// @brief CPU側のメッシュデータ（頂点配列+インデックス配列）
struct MeshData
{
    std::vector<Vertex3D_PBR> vertices;
    std::vector<uint32_t>     indices;
};

/// @brief プリミティブメッシュの生成ユーティリティ
class MeshGenerator
{
public:
    /// ボックス（中心原点）
    static MeshData CreateBox(float width, float height, float depth);

    /// 球体（中心原点）
    static MeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);

    /// 平面（XZ平面、Y=0）
    static MeshData CreatePlane(float width, float depth, uint32_t xSegments, uint32_t zSegments);

    /// シリンダー（中心原点）
    static MeshData CreateCylinder(float topRadius, float bottomRadius, float height,
                                    uint32_t sliceCount, uint32_t stackCount);
};

} // namespace GX
