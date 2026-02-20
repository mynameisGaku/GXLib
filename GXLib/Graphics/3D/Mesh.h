#pragma once
/// @file Mesh.h
/// @brief GPUメッシュ（VB/IB + サブメッシュ）
/// 初学者向け: VB=頂点バッファ、IB=インデックスバッファ。GPUに渡す基本単位です。

#include "pch.h"
#include "Graphics/Resource/Buffer.h"

namespace GX
{

/// @brief サブメッシュ情報
struct SubMesh
{
    uint32_t indexCount   = 0;
    uint32_t indexOffset  = 0;
    uint32_t vertexOffset = 0;
    int      materialHandle = -1;
    int      shaderHandle   = -1; // -1 = デフォルトPBRシェーダー
};

/// @brief メッシュ頂点レイアウト種別
enum class MeshVertexType
{
    PBR,        ///< Vertex3D_PBR
    SkinnedPBR  ///< Vertex3D_Skinned
};

/// @brief GPUメッシュ（単一VB/IB + 複数サブメッシュ）
class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    bool CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride);
    bool CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size,
                           DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);

    void AddSubMesh(const SubMesh& subMesh) { m_subMeshes.push_back(subMesh); }

    const Buffer& GetVertexBuffer() const { return m_vertexBuffer; }
    const Buffer& GetIndexBuffer() const { return m_indexBuffer; }
    const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
    std::vector<SubMesh>& GetSubMeshes() { return m_subMeshes; }

    void SetVertexType(MeshVertexType type) { m_vertexType = type; }
    MeshVertexType GetVertexType() const { return m_vertexType; }
    bool IsSkinned() const { return m_vertexType == MeshVertexType::SkinnedPBR; }

    /// @brief スムース法線バッファを作成する
    bool CreateSmoothNormalBuffer(ID3D12Device* device, const XMFLOAT3* data, uint32_t vertexCount);

    /// @brief スムース法線バッファのVBVを取得する
    const D3D12_VERTEX_BUFFER_VIEW& GetSmoothNormalBufferView() const { return m_smoothNormalBuffer.GetVertexBufferView(); }

    /// @brief スムース法線バッファが存在するかチェック
    bool HasSmoothNormals() const { return m_hasSmoothNormals; }

private:
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    Buffer m_smoothNormalBuffer;
    std::vector<SubMesh> m_subMeshes;
    MeshVertexType m_vertexType = MeshVertexType::PBR;
    bool m_hasSmoothNormals = false;
};

/// @brief 同一位置の頂点法線を平均化してスムース法線を計算する
/// @param positions 頂点位置配列
/// @param normals 頂点法線配列
/// @param vertexCount 頂点数
/// @return スムース法線配列
std::vector<XMFLOAT3> ComputeSmoothNormals(const XMFLOAT3* positions, const XMFLOAT3* normals, uint32_t vertexCount);

} // namespace GX
