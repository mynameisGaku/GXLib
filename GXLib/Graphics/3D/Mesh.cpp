/// @file Mesh.cpp
/// @brief Meshの実装
#include "pch.h"
#include "Graphics/3D/Mesh.h"
#include "Core/Logger.h"
#include <unordered_map>

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

bool Mesh::CreateSmoothNormalBuffer(ID3D12Device* device, const XMFLOAT3* data, uint32_t vertexCount)
{
    uint32_t bufferSize = vertexCount * sizeof(XMFLOAT3);
    m_hasSmoothNormals = m_smoothNormalBuffer.CreateVertexBuffer(device, data, bufferSize, sizeof(XMFLOAT3));
    return m_hasSmoothNormals;
}

// ============================================================================
// スムース法線計算（同一位置の頂点の法線を平均化）
// ============================================================================

/// @brief 位置量子化キー（精度0.0001）
struct PosKey
{
    int32_t x, y, z;
    bool operator==(const PosKey& other) const { return x == other.x && y == other.y && z == other.z; }
};

struct PosKeyHash
{
    size_t operator()(const PosKey& k) const
    {
        size_t h = std::hash<int32_t>()(k.x);
        h ^= std::hash<int32_t>()(k.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int32_t>()(k.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

std::vector<XMFLOAT3> ComputeSmoothNormals(const XMFLOAT3* positions, const XMFLOAT3* normals, uint32_t vertexCount)
{
    constexpr float k_Precision = 10000.0f; // 0.0001

    // 位置でグループ化
    std::unordered_map<PosKey, XMFLOAT3, PosKeyHash> normalAccum;
    normalAccum.reserve(vertexCount);

    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        PosKey key;
        key.x = static_cast<int32_t>(positions[i].x * k_Precision);
        key.y = static_cast<int32_t>(positions[i].y * k_Precision);
        key.z = static_cast<int32_t>(positions[i].z * k_Precision);

        auto it = normalAccum.find(key);
        if (it != normalAccum.end())
        {
            it->second.x += normals[i].x;
            it->second.y += normals[i].y;
            it->second.z += normals[i].z;
        }
        else
        {
            normalAccum[key] = normals[i];
        }
    }

    // 正規化
    for (auto& [key, n] : normalAccum)
    {
        XMVECTOR v = XMLoadFloat3(&n);
        float len = XMVectorGetX(XMVector3Length(v));
        if (len > 1e-6f)
        {
            v = XMVectorScale(v, 1.0f / len);
            XMStoreFloat3(&n, v);
        }
    }

    // 各頂点にスムース法線を割り当て
    std::vector<XMFLOAT3> result(vertexCount);
    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        PosKey key;
        key.x = static_cast<int32_t>(positions[i].x * k_Precision);
        key.y = static_cast<int32_t>(positions[i].y * k_Precision);
        key.z = static_cast<int32_t>(positions[i].z * k_Precision);

        auto it = normalAccum.find(key);
        if (it != normalAccum.end())
        {
            // ゼロベクトル安全チェック（退化時は元の法線にフォールバック）
            XMVECTOR v = XMLoadFloat3(&it->second);
            float len = XMVectorGetX(XMVector3Length(v));
            if (len > 1e-6f)
                result[i] = it->second;
            else
                result[i] = normals[i];
        }
        else
        {
            result[i] = normals[i];
        }
    }

    return result;
}

} // namespace GX
