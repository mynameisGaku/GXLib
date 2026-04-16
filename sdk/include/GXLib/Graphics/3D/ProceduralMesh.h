#pragma once
/// @file ProceduralMesh.h
/// @brief プロシージャルメッシュビルダーAPI
///
/// 頂点とインデックスを動的に構築し、法線・タンジェント再計算、
/// メッシュ結合を行えるビルダーパターンAPI。

#include "pch_graphics.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Vertex3D.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief プロシージャルメッシュビルダー
///
/// 頂点とインデックスを手動追加し、法線・タンジェントの自動計算や
/// メッシュ結合機能を持つ。
class ProceduralMeshBuilder
{
public:
    ProceduralMeshBuilder() = default;
    ~ProceduralMeshBuilder() = default;

    /// @brief 頂点を追加する
    /// @param position 位置
    /// @param normal 法線
    /// @param texcoord テクスチャ座標
    /// @param tangent タンジェント（省略時ゼロ）
    /// @return 追加された頂点のインデックス
    uint32_t AddVertex(const Vector3& position, const Vector3& normal,
                       const Vector2& texcoord, const Vector4& tangent = {0,0,0,1});

    /// @brief 三角形を追加する
    /// @param i0 頂点インデックス0
    /// @param i1 頂点インデックス1
    /// @param i2 頂点インデックス2
    void AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2);

    /// @brief 既存MeshDataの内容を追加する
    /// @param mesh 追加するメッシュデータ
    void Append(const MeshData& mesh);

    /// @brief 面法線の平均から頂点法線を再計算する
    void ComputeNormals();

    /// @brief タンジェントベクトルを計算する（Lengyel法）
    void ComputeTangents();

    /// @brief 構築結果をMeshDataとして取得する
    /// @return 完成したメッシュデータ
    MeshData Build() const;

    /// @brief 内部データをクリアする
    void Clear();

    /// @brief 現在の頂点数を取得する
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }

    /// @brief 現在のインデックス数を取得する
    uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }

private:
    gx::Vector<Vertex3D_PBR> m_vertices;
    gx::Vector<uint32_t>     m_indices;
};

/// @}
} // namespace gx
