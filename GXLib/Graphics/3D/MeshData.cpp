/// @file MeshData.cpp
/// @brief プリミティブメッシュ生成の実装
#include "pch.h"
#include "Graphics/3D/MeshData.h"
#include <cmath>

namespace GX
{

MeshData MeshGenerator::CreateBox(float width, float height, float depth)
{
    MeshData mesh;

    float hw = width  * 0.5f;
    float hh = height * 0.5f;
    float hd = depth  * 0.5f;

    // 各面に頂点4つ × 6面 = 24頂点
    // Front face (+Z)
    mesh.vertices.push_back({ { -hw, -hh,  hd }, {  0,  0,  1 }, { 0, 1 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ { -hw,  hh,  hd }, {  0,  0,  1 }, { 0, 0 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw,  hh,  hd }, {  0,  0,  1 }, { 1, 0 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw, -hh,  hd }, {  0,  0,  1 }, { 1, 1 }, { 1, 0, 0, 1 } });

    // Back face (-Z)
    mesh.vertices.push_back({ {  hw, -hh, -hd }, {  0,  0, -1 }, { 0, 1 }, { -1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw,  hh, -hd }, {  0,  0, -1 }, { 0, 0 }, { -1, 0, 0, 1 } });
    mesh.vertices.push_back({ { -hw,  hh, -hd }, {  0,  0, -1 }, { 1, 0 }, { -1, 0, 0, 1 } });
    mesh.vertices.push_back({ { -hw, -hh, -hd }, {  0,  0, -1 }, { 1, 1 }, { -1, 0, 0, 1 } });

    // Top face (+Y)
    mesh.vertices.push_back({ { -hw,  hh,  hd }, {  0,  1,  0 }, { 0, 1 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ { -hw,  hh, -hd }, {  0,  1,  0 }, { 0, 0 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw,  hh, -hd }, {  0,  1,  0 }, { 1, 0 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw,  hh,  hd }, {  0,  1,  0 }, { 1, 1 }, { 1, 0, 0, 1 } });

    // Bottom face (-Y)
    mesh.vertices.push_back({ { -hw, -hh, -hd }, {  0, -1,  0 }, { 0, 1 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ { -hw, -hh,  hd }, {  0, -1,  0 }, { 0, 0 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw, -hh,  hd }, {  0, -1,  0 }, { 1, 0 }, { 1, 0, 0, 1 } });
    mesh.vertices.push_back({ {  hw, -hh, -hd }, {  0, -1,  0 }, { 1, 1 }, { 1, 0, 0, 1 } });

    // Right face (+X)
    mesh.vertices.push_back({ {  hw, -hh,  hd }, {  1,  0,  0 }, { 0, 1 }, { 0, 0, -1, 1 } });
    mesh.vertices.push_back({ {  hw,  hh,  hd }, {  1,  0,  0 }, { 0, 0 }, { 0, 0, -1, 1 } });
    mesh.vertices.push_back({ {  hw,  hh, -hd }, {  1,  0,  0 }, { 1, 0 }, { 0, 0, -1, 1 } });
    mesh.vertices.push_back({ {  hw, -hh, -hd }, {  1,  0,  0 }, { 1, 1 }, { 0, 0, -1, 1 } });

    // Left face (-X)
    mesh.vertices.push_back({ { -hw, -hh, -hd }, { -1,  0,  0 }, { 0, 1 }, { 0, 0, 1, 1 } });
    mesh.vertices.push_back({ { -hw,  hh, -hd }, { -1,  0,  0 }, { 0, 0 }, { 0, 0, 1, 1 } });
    mesh.vertices.push_back({ { -hw,  hh,  hd }, { -1,  0,  0 }, { 1, 0 }, { 0, 0, 1, 1 } });
    mesh.vertices.push_back({ { -hw, -hh,  hd }, { -1,  0,  0 }, { 1, 1 }, { 0, 0, 1, 1 } });

    // インデックス（各面2三角形）
    for (uint32_t face = 0; face < 6; ++face)
    {
        uint32_t base = face * 4;
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }

    return mesh;
}

MeshData MeshGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
    MeshData mesh;

    // 北極
    mesh.vertices.push_back({ { 0, radius, 0 }, { 0, 1, 0 }, { 0, 0 }, { 1, 0, 0, 1 } });

    float phiStep   = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    for (uint32_t i = 1; i < stackCount; ++i)
    {
        float phi = i * phiStep;
        for (uint32_t j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;

            Vertex3D_PBR v;
            v.position.x = radius * sinf(phi) * cosf(theta);
            v.position.y = radius * cosf(phi);
            v.position.z = radius * sinf(phi) * sinf(theta);

            XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&v.position));
            XMStoreFloat3(&v.normal, n);

            v.texcoord.x = theta / (2.0f * XM_PI);
            v.texcoord.y = phi / XM_PI;

            // tangent: d(position)/d(theta)
            v.tangent.x = -sinf(theta);
            v.tangent.y = 0.0f;
            v.tangent.z = cosf(theta);
            v.tangent.w = 1.0f;

            mesh.vertices.push_back(v);
        }
    }

    // 南極
    mesh.vertices.push_back({ { 0, -radius, 0 }, { 0, -1, 0 }, { 0, 1 }, { 1, 0, 0, 1 } });

    // 北極のインデックス
    for (uint32_t j = 0; j < sliceCount; ++j)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(j + 1);
        mesh.indices.push_back(j + 2);
    }

    // 中間の帯
    uint32_t ringVertexCount = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount - 2; ++i)
    {
        for (uint32_t j = 0; j < sliceCount; ++j)
        {
            uint32_t base = 1 + i * ringVertexCount;
            mesh.indices.push_back(base + j);
            mesh.indices.push_back(base + j + ringVertexCount);
            mesh.indices.push_back(base + j + 1);

            mesh.indices.push_back(base + j + 1);
            mesh.indices.push_back(base + j + ringVertexCount);
            mesh.indices.push_back(base + j + ringVertexCount + 1);
        }
    }

    // 南極のインデックス
    uint32_t southPoleIndex = static_cast<uint32_t>(mesh.vertices.size()) - 1;
    uint32_t baseIndex = southPoleIndex - ringVertexCount;
    for (uint32_t j = 0; j < sliceCount; ++j)
    {
        mesh.indices.push_back(southPoleIndex);
        mesh.indices.push_back(baseIndex + j + 1);
        mesh.indices.push_back(baseIndex + j);
    }

    return mesh;
}

MeshData MeshGenerator::CreatePlane(float width, float depth, uint32_t xSegments, uint32_t zSegments)
{
    MeshData mesh;

    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    float dx = width / xSegments;
    float dz = depth / zSegments;
    float du = 1.0f / xSegments;
    float dv = 1.0f / zSegments;

    for (uint32_t z = 0; z <= zSegments; ++z)
    {
        for (uint32_t x = 0; x <= xSegments; ++x)
        {
            Vertex3D_PBR v;
            v.position = { -hw + x * dx, 0.0f, -hd + z * dz };
            v.normal   = { 0.0f, 1.0f, 0.0f };
            v.texcoord = { x * du, z * dv };
            v.tangent  = { 1.0f, 0.0f, 0.0f, 1.0f };
            mesh.vertices.push_back(v);
        }
    }

    for (uint32_t z = 0; z < zSegments; ++z)
    {
        for (uint32_t x = 0; x < xSegments; ++x)
        {
            uint32_t row  = z * (xSegments + 1);
            uint32_t next = (z + 1) * (xSegments + 1);

            mesh.indices.push_back(row + x);
            mesh.indices.push_back(row + x + 1);
            mesh.indices.push_back(next + x);

            mesh.indices.push_back(row + x + 1);
            mesh.indices.push_back(next + x + 1);
            mesh.indices.push_back(next + x);
        }
    }

    return mesh;
}

MeshData MeshGenerator::CreateCylinder(float topRadius, float bottomRadius, float height,
                                         uint32_t sliceCount, uint32_t stackCount)
{
    MeshData mesh;

    float stackHeight = height / stackCount;
    float radiusStep  = (topRadius - bottomRadius) / stackCount;
    float thetaStep   = 2.0f * XM_PI / sliceCount;

    // 側面頂点
    for (uint32_t i = 0; i <= stackCount; ++i)
    {
        float y = -height * 0.5f + i * stackHeight;
        float r = bottomRadius + i * radiusStep;

        for (uint32_t j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;
            float c = cosf(theta);
            float s = sinf(theta);

            Vertex3D_PBR v;
            v.position = { r * c, y, r * s };

            // 法線（テーパー考慮）
            float dr = bottomRadius - topRadius;
            XMFLOAT3 tangent = { -s, 0, c };
            XMFLOAT3 bitangent = { dr * c, height, dr * s };
            XMVECTOR T = XMLoadFloat3(&tangent);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&v.normal, N);

            v.texcoord = { static_cast<float>(j) / sliceCount,
                           1.0f - static_cast<float>(i) / stackCount };
            v.tangent = { tangent.x, tangent.y, tangent.z, 1.0f };

            mesh.vertices.push_back(v);
        }
    }

    // 側面インデックス
    uint32_t ringCount = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount; ++i)
    {
        for (uint32_t j = 0; j < sliceCount; ++j)
        {
            uint32_t base = i * ringCount;
            mesh.indices.push_back(base + j);
            mesh.indices.push_back(base + j + ringCount);
            mesh.indices.push_back(base + j + 1);

            mesh.indices.push_back(base + j + 1);
            mesh.indices.push_back(base + j + ringCount);
            mesh.indices.push_back(base + j + ringCount + 1);
        }
    }

    // 上面キャップ
    uint32_t topCenterIndex = static_cast<uint32_t>(mesh.vertices.size());
    float topY = height * 0.5f;
    mesh.vertices.push_back({ { 0, topY, 0 }, { 0, 1, 0 }, { 0.5f, 0.5f }, { 1, 0, 0, 1 } });
    for (uint32_t j = 0; j <= sliceCount; ++j)
    {
        float theta = j * thetaStep;
        mesh.vertices.push_back({ { topRadius * cosf(theta), topY, topRadius * sinf(theta) },
                                   { 0, 1, 0 },
                                   { cosf(theta) * 0.5f + 0.5f, sinf(theta) * 0.5f + 0.5f },
                                   { 1, 0, 0, 1 } });
    }
    for (uint32_t j = 0; j < sliceCount; ++j)
    {
        mesh.indices.push_back(topCenterIndex);
        mesh.indices.push_back(topCenterIndex + j + 1);
        mesh.indices.push_back(topCenterIndex + j + 2);
    }

    // 下面キャップ
    uint32_t bottomCenterIndex = static_cast<uint32_t>(mesh.vertices.size());
    float bottomY = -height * 0.5f;
    mesh.vertices.push_back({ { 0, bottomY, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f }, { 1, 0, 0, 1 } });
    for (uint32_t j = 0; j <= sliceCount; ++j)
    {
        float theta = j * thetaStep;
        mesh.vertices.push_back({ { bottomRadius * cosf(theta), bottomY, bottomRadius * sinf(theta) },
                                   { 0, -1, 0 },
                                   { cosf(theta) * 0.5f + 0.5f, sinf(theta) * 0.5f + 0.5f },
                                   { 1, 0, 0, 1 } });
    }
    for (uint32_t j = 0; j < sliceCount; ++j)
    {
        mesh.indices.push_back(bottomCenterIndex);
        mesh.indices.push_back(bottomCenterIndex + j + 2);
        mesh.indices.push_back(bottomCenterIndex + j + 1);
    }

    return mesh;
}

} // namespace GX
