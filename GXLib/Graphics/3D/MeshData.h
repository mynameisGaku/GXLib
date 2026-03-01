#pragma once
/// @file MeshData.h
/// @brief CPU側メッシュデータ + プリミティブ生成

#include "pch_graphics.h"
#include "Graphics/3D/Vertex3D.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief CPU側のメッシュデータ（頂点配列+インデックス配列）
/// GPUへ送る前の中間データ。MeshGeneratorで生成するか、自前で構築してCreateGPUMeshに渡す
struct MeshData
{
    std::vector<Vertex3D_PBR> vertices;  ///< 頂点配列
    std::vector<uint32_t>     indices;   ///< インデックス配列（三角形リスト）
};

/// @brief プリミティブメッシュの生成ユーティリティ
/// ボックスや球体などの基本形状をCPU側で生成する。生成したMeshDataは
/// Renderer3D::CreateGPUMesh() でGPUリソースに変換して描画に使う
class MeshGenerator
{
public:
    /// @brief ボックスを生成する（中心が原点）
    /// @param width X方向の幅
    /// @param height Y方向の高さ
    /// @param depth Z方向の奥行き
    /// @return 生成されたメッシュデータ
    static MeshData CreateBox(float width, float height, float depth);

    /// @brief 球体を生成する（中心が原点）
    /// @param radius 半径
    /// @param sliceCount 水平方向の分割数
    /// @param stackCount 垂直方向の分割数
    /// @return 生成されたメッシュデータ
    static MeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);

    /// @brief 平面を生成する（XZ平面上、Y=0）
    /// @param width X方向の幅
    /// @param depth Z方向の奥行き
    /// @param xSegments X方向の分割数
    /// @param zSegments Z方向の分割数
    /// @return 生成されたメッシュデータ
    static MeshData CreatePlane(float width, float depth, uint32_t xSegments, uint32_t zSegments);

    /// @brief シリンダーを生成する（中心が原点、テーパー対応）
    /// @param topRadius 上面の半径
    /// @param bottomRadius 下面の半径
    /// @param height 高さ
    /// @param sliceCount 水平方向の分割数
    /// @param stackCount 垂直方向の分割数
    /// @return 生成されたメッシュデータ
    static MeshData CreateCylinder(float topRadius, float bottomRadius, float height,
                                    uint32_t sliceCount, uint32_t stackCount);

    /// @brief トーラスを生成する
    static MeshData CreateTorus(float majorRadius, float minorRadius,
                                 uint32_t majorSegments, uint32_t minorSegments);

    /// @brief カプセルを生成する
    static MeshData CreateCapsule(float radius, float height, uint32_t segments);

    /// @brief Icosphereを生成する
    static MeshData CreateIcosphere(float radius, uint32_t subdivisions);

    /// @brief コーンを生成する
    static MeshData CreateCone(float radius, float height, uint32_t sliceCount);

    /// @brief 矢印を生成する（シャフト+ヘッド）
    static MeshData CreateArrow(float shaftRadius, float shaftLength,
                                 float headRadius, float headLength, uint32_t sliceCount);

    /// @brief 法線を再計算する（面法線の平均）
    static void ComputeNormals(MeshData& mesh);

    /// @brief タンジェントを計算する（Mikktspace近似）
    static void ComputeTangents(MeshData& mesh);

    /// @brief 2つのメッシュを結合する
    static MeshData Combine(const MeshData& a, const MeshData& b);
};

/// @}
} // namespace gx
