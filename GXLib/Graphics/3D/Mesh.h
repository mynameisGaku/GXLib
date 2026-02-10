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

private:
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    std::vector<SubMesh> m_subMeshes;
    MeshVertexType m_vertexType = MeshVertexType::PBR;
};

} // namespace GX
