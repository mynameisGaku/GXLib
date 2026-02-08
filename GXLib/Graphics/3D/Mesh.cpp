/// @file Mesh.cpp
/// @brief Meshの実装
#include "pch.h"
#include "Graphics/3D/Mesh.h"

namespace GX
{

bool Mesh::CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride)
{
    return m_vertexBuffer.CreateVertexBuffer(device, data, size, stride);
}

bool Mesh::CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size, DXGI_FORMAT format)
{
    return m_indexBuffer.CreateIndexBuffer(device, data, size, format);
}

} // namespace GX
