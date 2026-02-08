#pragma once
/// @file Mesh.h
/// @brief GPU上のメッシュ（VB/IBラッパー）

#include "pch.h"
#include "Graphics/Resource/Buffer.h"

namespace GX
{

/// @brief サブメッシュ情報
struct SubMesh
{
    uint32_t indexCount  = 0;
    uint32_t indexOffset = 0;
    uint32_t vertexOffset = 0;
    int      materialHandle = -1;
};

/// @brief GPU上のメッシュ（1つのVB/IBに複数SubMeshを格納）
class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    bool CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride);
    bool CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size, DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);

    void AddSubMesh(const SubMesh& subMesh) { m_subMeshes.push_back(subMesh); }

    const Buffer& GetVertexBuffer() const { return m_vertexBuffer; }
    const Buffer& GetIndexBuffer() const { return m_indexBuffer; }
    const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }

private:
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    std::vector<SubMesh> m_subMeshes;
};

} // namespace GX
