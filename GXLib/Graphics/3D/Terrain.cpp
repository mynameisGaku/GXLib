/// @file Terrain.cpp
/// @brief ハイトマップ地形レンダラーの実装

#include "pch.h"
#include "Graphics/3D/Terrain.h"
#include "Core/Logger.h"
#include <cmath>

namespace GX
{

// ============================================================================
// 簡易ハッシュベースのノイズ関数
// ============================================================================

static float Hash(float x, float z)
{
    // 簡易ハッシュ（sin ベース）
    float h = sinf(x * 127.1f + z * 311.7f) * 43758.5453f;
    return h - floorf(h);
}

static float SmoothNoise(float x, float z)
{
    float ix = floorf(x);
    float iz = floorf(z);
    float fx = x - ix;
    float fz = z - iz;

    // Hermite補間
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);

    float a = Hash(ix,     iz);
    float b = Hash(ix + 1, iz);
    float c = Hash(ix,     iz + 1);
    float d = Hash(ix + 1, iz + 1);

    float ab = a + (b - a) * fx;
    float cd = c + (d - c) * fx;
    return ab + (cd - ab) * fz;
}

float Terrain::ProceduralHeight(float x, float z)
{
    // 複数オクターブのFBMノイズ
    float height = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.02f;

    for (int i = 0; i < 5; ++i)
    {
        height += SmoothNoise(x * frequency, z * frequency) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return height / 1.9375f;  // 正規化 (1 + 0.5 + 0.25 + 0.125 + 0.0625)
}

// ============================================================================
// 地形生成
// ============================================================================

bool Terrain::CreateProcedural(ID3D12Device* device, float width, float depth,
                                uint32_t xSegments, uint32_t zSegments, float maxHeight)
{
    m_xSegments = xSegments;
    m_zSegments = zSegments;
    m_width     = width;
    m_depth     = depth;
    m_originX   = -width * 0.5f;
    m_originZ   = -depth * 0.5f;

    uint32_t vertexCount = (xSegments + 1) * (zSegments + 1);
    m_heights.resize(vertexCount);

    // プロシージャルハイト生成
    for (uint32_t z = 0; z <= zSegments; ++z)
    {
        for (uint32_t x = 0; x <= xSegments; ++x)
        {
            float fx = m_originX + (static_cast<float>(x) / xSegments) * width;
            float fz = m_originZ + (static_cast<float>(z) / zSegments) * depth;
            m_heights[z * (xSegments + 1) + x] = ProceduralHeight(fx, fz) * maxHeight;
        }
    }

    // 頂点生成
    std::vector<Vertex3D_PBR> vertices(vertexCount);
    for (uint32_t z = 0; z <= zSegments; ++z)
    {
        for (uint32_t x = 0; x <= xSegments; ++x)
        {
            uint32_t idx = z * (xSegments + 1) + x;
            float fx = m_originX + (static_cast<float>(x) / xSegments) * width;
            float fz = m_originZ + (static_cast<float>(z) / zSegments) * depth;
            float fy = m_heights[idx];

            vertices[idx].position = { fx, fy, fz };

            // UV（テクスチャ座標）
            vertices[idx].texcoord = {
                static_cast<float>(x) / xSegments * 4.0f,
                static_cast<float>(z) / zSegments * 4.0f
            };

            // タンジェント（X方向）
            vertices[idx].tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
        }
    }

    // 法線計算（中央差分）
    for (uint32_t z = 0; z <= zSegments; ++z)
    {
        for (uint32_t x = 0; x <= xSegments; ++x)
        {
            uint32_t idx = z * (xSegments + 1) + x;

            // 隣接セルからの高さ差分で法線を近似
            float hL = (x > 0) ? m_heights[z * (xSegments + 1) + (x - 1)] : m_heights[idx];
            float hR = (x < xSegments) ? m_heights[z * (xSegments + 1) + (x + 1)] : m_heights[idx];
            float hD = (z > 0) ? m_heights[(z - 1) * (xSegments + 1) + x] : m_heights[idx];
            float hU = (z < zSegments) ? m_heights[(z + 1) * (xSegments + 1) + x] : m_heights[idx];

            float stepX = width / xSegments;
            float stepZ = depth / zSegments;

            XMVECTOR normal = XMVector3Normalize(
                XMVectorSet(
                    (hL - hR) / (2.0f * stepX),
                    1.0f,
                    (hD - hU) / (2.0f * stepZ),
                    0.0f
                )
            );
            XMStoreFloat3(&vertices[idx].normal, normal);

            // タンジェント更新（法線に直交するようにX方向ベースで補正）
            XMVECTOR tangent = XMVector3Normalize(
                XMVector3Cross(XMVectorSet(0, 0, 1, 0), normal)
            );
            XMFLOAT3 t;
            XMStoreFloat3(&t, tangent);
            vertices[idx].tangent = { t.x, t.y, t.z, 1.0f };
        }
    }

    // インデックス生成
    uint32_t indexCount = xSegments * zSegments * 6;
    std::vector<uint32_t> indices(indexCount);
    uint32_t ii = 0;
    for (uint32_t z = 0; z < zSegments; ++z)
    {
        for (uint32_t x = 0; x < xSegments; ++x)
        {
            uint32_t topLeft     = z * (xSegments + 1) + x;
            uint32_t topRight    = topLeft + 1;
            uint32_t bottomLeft  = (z + 1) * (xSegments + 1) + x;
            uint32_t bottomRight = bottomLeft + 1;

            indices[ii++] = topLeft;
            indices[ii++] = bottomLeft;
            indices[ii++] = topRight;

            indices[ii++] = topRight;
            indices[ii++] = bottomLeft;
            indices[ii++] = bottomRight;
        }
    }

    BuildMesh(device, vertices, indices);
    GX_LOG_INFO("Terrain: Created procedural terrain (%dx%d segments, maxH=%.1f)",
                xSegments, zSegments, maxHeight);
    return true;
}

bool Terrain::CreateFromHeightmap(ID3D12Device* device,
                                    const float* heightmapData, uint32_t hmWidth, uint32_t hmHeight,
                                    float worldWidth, float worldDepth, float maxHeight)
{
    uint32_t xSegments = hmWidth - 1;
    uint32_t zSegments = hmHeight - 1;

    m_xSegments = xSegments;
    m_zSegments = zSegments;
    m_width     = worldWidth;
    m_depth     = worldDepth;
    m_originX   = -worldWidth * 0.5f;
    m_originZ   = -worldDepth * 0.5f;

    uint32_t vertexCount = hmWidth * hmHeight;
    m_heights.resize(vertexCount);

    for (uint32_t i = 0; i < vertexCount; ++i)
        m_heights[i] = heightmapData[i] * maxHeight;

    // 頂点生成
    std::vector<Vertex3D_PBR> vertices(vertexCount);
    for (uint32_t z = 0; z < hmHeight; ++z)
    {
        for (uint32_t x = 0; x < hmWidth; ++x)
        {
            uint32_t idx = z * hmWidth + x;
            float fx = m_originX + (static_cast<float>(x) / xSegments) * worldWidth;
            float fz = m_originZ + (static_cast<float>(z) / zSegments) * worldDepth;

            vertices[idx].position = { fx, m_heights[idx], fz };
            vertices[idx].texcoord = {
                static_cast<float>(x) / xSegments * 4.0f,
                static_cast<float>(z) / zSegments * 4.0f
            };
            vertices[idx].tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
        }
    }

    // 法線計算
    for (uint32_t z = 0; z < hmHeight; ++z)
    {
        for (uint32_t x = 0; x < hmWidth; ++x)
        {
            uint32_t idx = z * hmWidth + x;
            float hL = (x > 0) ? m_heights[z * hmWidth + (x - 1)] : m_heights[idx];
            float hR = (x < xSegments) ? m_heights[z * hmWidth + (x + 1)] : m_heights[idx];
            float hD = (z > 0) ? m_heights[(z - 1) * hmWidth + x] : m_heights[idx];
            float hU = (z < zSegments) ? m_heights[(z + 1) * hmWidth + x] : m_heights[idx];

            float stepX = worldWidth / xSegments;
            float stepZ = worldDepth / zSegments;

            XMVECTOR normal = XMVector3Normalize(
                XMVectorSet((hL - hR) / (2.0f * stepX), 1.0f, (hD - hU) / (2.0f * stepZ), 0.0f)
            );
            XMStoreFloat3(&vertices[idx].normal, normal);

            XMVECTOR tangent = XMVector3Normalize(
                XMVector3Cross(XMVectorSet(0, 0, 1, 0), normal)
            );
            XMFLOAT3 t;
            XMStoreFloat3(&t, tangent);
            vertices[idx].tangent = { t.x, t.y, t.z, 1.0f };
        }
    }

    // インデックス生成
    uint32_t indexCount = xSegments * zSegments * 6;
    std::vector<uint32_t> indices(indexCount);
    uint32_t ii = 0;
    for (uint32_t z = 0; z < zSegments; ++z)
    {
        for (uint32_t x = 0; x < xSegments; ++x)
        {
            uint32_t topLeft     = z * hmWidth + x;
            uint32_t topRight    = topLeft + 1;
            uint32_t bottomLeft  = (z + 1) * hmWidth + x;
            uint32_t bottomRight = bottomLeft + 1;

            indices[ii++] = topLeft;
            indices[ii++] = bottomLeft;
            indices[ii++] = topRight;

            indices[ii++] = topRight;
            indices[ii++] = bottomLeft;
            indices[ii++] = bottomRight;
        }
    }

    BuildMesh(device, vertices, indices);
    GX_LOG_INFO("Terrain: Created heightmap terrain (%dx%d, maxH=%.1f)",
                hmWidth, hmHeight, maxHeight);
    return true;
}

void Terrain::BuildMesh(ID3D12Device* device, const std::vector<Vertex3D_PBR>& vertices,
                          const std::vector<uint32_t>& indices)
{
    uint32_t vbSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex3D_PBR));
    m_vertexBuffer.CreateVertexBuffer(device, vertices.data(), vbSize, sizeof(Vertex3D_PBR));

    uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
    m_indexBuffer.CreateIndexBuffer(device, indices.data(), ibSize, DXGI_FORMAT_R32_UINT);

    m_indexCount = static_cast<uint32_t>(indices.size());
}

float Terrain::GetHeight(float x, float z) const
{
    if (m_heights.empty() || m_xSegments == 0 || m_zSegments == 0)
        return 0.0f;

    // ローカル座標に変換
    float lx = (x - m_originX) / m_width;
    float lz = (z - m_originZ) / m_depth;

    // 範囲外チェック
    lx = (std::max)(0.0f, (std::min)(1.0f, lx));
    lz = (std::max)(0.0f, (std::min)(1.0f, lz));

    // グリッドセルの座標
    float gx = lx * m_xSegments;
    float gz = lz * m_zSegments;

    uint32_t ix = (std::min)(static_cast<uint32_t>(gx), m_xSegments - 1);
    uint32_t iz = (std::min)(static_cast<uint32_t>(gz), m_zSegments - 1);

    float fx = gx - ix;
    float fz = gz - iz;

    // バイリニア補間
    uint32_t stride = m_xSegments + 1;
    float h00 = m_heights[iz * stride + ix];
    float h10 = m_heights[iz * stride + ix + 1];
    float h01 = m_heights[(iz + 1) * stride + ix];
    float h11 = m_heights[(iz + 1) * stride + ix + 1];

    float h0 = h00 + (h10 - h00) * fx;
    float h1 = h01 + (h11 - h01) * fx;
    return h0 + (h1 - h0) * fz;
}

} // namespace GX
