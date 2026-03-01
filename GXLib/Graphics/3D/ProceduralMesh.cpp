/// @file ProceduralMesh.cpp
/// @brief プロシージャルメッシュビルダー + MeshGenerator拡張形状

#include "pch_graphics.h"
#include "Graphics/3D/ProceduralMesh.h"
#include <cmath>
#include <algorithm>
#include <map>

namespace gx
{

// ============================================================================
// ProceduralMeshBuilder
// ============================================================================

uint32_t ProceduralMeshBuilder::AddVertex(const XMFLOAT3& position, const XMFLOAT3& normal,
                                           const XMFLOAT2& texcoord, const XMFLOAT4& tangent)
{
    Vertex3D_PBR v;
    v.position = position;
    v.normal   = normal;
    v.texcoord = texcoord;
    v.tangent  = tangent;
    uint32_t idx = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(v);
    return idx;
}

void ProceduralMeshBuilder::AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2)
{
    m_indices.push_back(i0);
    m_indices.push_back(i1);
    m_indices.push_back(i2);
}

void ProceduralMeshBuilder::Append(const MeshData& mesh)
{
    uint32_t offset = static_cast<uint32_t>(m_vertices.size());
    m_vertices.insert(m_vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    for (uint32_t idx : mesh.indices)
        m_indices.push_back(idx + offset);
}

void ProceduralMeshBuilder::ComputeNormals()
{
    // Zero all normals
    for (auto& v : m_vertices)
        v.normal = {0.0f, 0.0f, 0.0f};

    // Accumulate face normals
    for (size_t i = 0; i + 2 < m_indices.size(); i += 3)
    {
        auto& v0 = m_vertices[m_indices[i]];
        auto& v1 = m_vertices[m_indices[i + 1]];
        auto& v2 = m_vertices[m_indices[i + 2]];

        XMFLOAT3 e1 = {v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z};
        XMFLOAT3 e2 = {v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z};
        // e2 x e1 for left-handed (DirectX) coordinate system
        XMFLOAT3 n = {
            e2.y * e1.z - e2.z * e1.y,
            e2.z * e1.x - e2.x * e1.z,
            e2.x * e1.y - e2.y * e1.x
        };

        for (int j = 0; j < 3; ++j)
        {
            auto& vn = m_vertices[m_indices[i + j]].normal;
            vn.x += n.x; vn.y += n.y; vn.z += n.z;
        }
    }

    // Normalize
    for (auto& v : m_vertices)
    {
        float len = std::sqrt(v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z);
        if (len > 1e-6f) { v.normal.x /= len; v.normal.y /= len; v.normal.z /= len; }
    }
}

void ProceduralMeshBuilder::ComputeTangents()
{
    std::vector<XMFLOAT3> tan1(m_vertices.size(), {0,0,0});
    std::vector<XMFLOAT3> tan2(m_vertices.size(), {0,0,0});

    for (size_t i = 0; i + 2 < m_indices.size(); i += 3)
    {
        uint32_t i0 = m_indices[i], i1 = m_indices[i+1], i2 = m_indices[i+2];
        auto& v0 = m_vertices[i0];
        auto& v1 = m_vertices[i1];
        auto& v2 = m_vertices[i2];

        float dx1 = v1.position.x - v0.position.x, dy1 = v1.position.y - v0.position.y, dz1 = v1.position.z - v0.position.z;
        float dx2 = v2.position.x - v0.position.x, dy2 = v2.position.y - v0.position.y, dz2 = v2.position.z - v0.position.z;
        float du1 = v1.texcoord.x - v0.texcoord.x, dv1 = v1.texcoord.y - v0.texcoord.y;
        float du2 = v2.texcoord.x - v0.texcoord.x, dv2 = v2.texcoord.y - v0.texcoord.y;

        float r = 1.0f / (du1 * dv2 - du2 * dv1 + 1e-10f);
        XMFLOAT3 sdir = {
            (dv2 * dx1 - dv1 * dx2) * r,
            (dv2 * dy1 - dv1 * dy2) * r,
            (dv2 * dz1 - dv1 * dz2) * r
        };
        XMFLOAT3 tdir = {
            (du1 * dx2 - du2 * dx1) * r,
            (du1 * dy2 - du2 * dy1) * r,
            (du1 * dz2 - du2 * dz1) * r
        };

        for (uint32_t idx : {i0, i1, i2})
        {
            tan1[idx].x += sdir.x; tan1[idx].y += sdir.y; tan1[idx].z += sdir.z;
            tan2[idx].x += tdir.x; tan2[idx].y += tdir.y; tan2[idx].z += tdir.z;
        }
    }

    for (size_t i = 0; i < m_vertices.size(); ++i)
    {
        XMVECTOR n = XMLoadFloat3(&m_vertices[i].normal);
        XMVECTOR t = XMLoadFloat3(&tan1[i]);
        // Gram-Schmidt orthogonalize
        XMVECTOR tangent = XMVector3Normalize(t - n * XMVector3Dot(n, t));
        // Handedness
        XMVECTOR b = XMLoadFloat3(&tan2[i]);
        float w = (XMVectorGetX(XMVector3Dot(XMVector3Cross(n, t), b)) < 0.0f) ? -1.0f : 1.0f;
        XMFLOAT3 result;
        XMStoreFloat3(&result, tangent);
        m_vertices[i].tangent = {result.x, result.y, result.z, w};
    }
}

MeshData ProceduralMeshBuilder::Build() const
{
    MeshData mesh;
    mesh.vertices = m_vertices;
    mesh.indices  = m_indices;
    return mesh;
}

void ProceduralMeshBuilder::Clear()
{
    m_vertices.clear();
    m_indices.clear();
}

// ============================================================================
// MeshGenerator Extensions
// ============================================================================

MeshData MeshGenerator::CreateTorus(float majorRadius, float minorRadius,
                                     uint32_t majorSegments, uint32_t minorSegments)
{
    MeshData mesh;
    constexpr float PI = 3.14159265358979323846f;

    for (uint32_t i = 0; i <= majorSegments; ++i)
    {
        float u = static_cast<float>(i) / majorSegments;
        float theta = u * 2.0f * PI;
        float cosTheta = std::cos(theta), sinTheta = std::sin(theta);

        for (uint32_t j = 0; j <= minorSegments; ++j)
        {
            float v = static_cast<float>(j) / minorSegments;
            float phi = v * 2.0f * PI;
            float cosPhi = std::cos(phi), sinPhi = std::sin(phi);

            float x = (majorRadius + minorRadius * cosPhi) * cosTheta;
            float y = minorRadius * sinPhi;
            float z = (majorRadius + minorRadius * cosPhi) * sinTheta;

            // Normal: direction from ring center to vertex
            float nx = cosPhi * cosTheta;
            float ny = sinPhi;
            float nz = cosPhi * sinTheta;

            Vertex3D_PBR vert;
            vert.position = {x, y, z};
            vert.normal = {nx, ny, nz};
            vert.texcoord = {u, v};
            vert.tangent = {-sinTheta, 0.0f, cosTheta, 1.0f};
            mesh.vertices.push_back(vert);
        }
    }

    for (uint32_t i = 0; i < majorSegments; ++i)
    {
        for (uint32_t j = 0; j < minorSegments; ++j)
        {
            uint32_t a = i * (minorSegments + 1) + j;
            uint32_t b = a + minorSegments + 1;
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(a + 1);
            mesh.indices.push_back(a + 1);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b + 1);
        }
    }

    return mesh;
}

MeshData MeshGenerator::CreateCapsule(float radius, float height, uint32_t segments)
{
    constexpr float PI = 3.14159265358979323846f;
    MeshData mesh;
    uint32_t rings = segments / 2;
    float halfHeight = height * 0.5f;

    // Top hemisphere
    for (uint32_t i = 0; i <= rings; ++i)
    {
        float phi = PI * 0.5f * static_cast<float>(i) / rings;
        float y = halfHeight + radius * std::cos(phi);
        float r = radius * std::sin(phi);
        for (uint32_t j = 0; j <= segments; ++j)
        {
            float theta = 2.0f * PI * static_cast<float>(j) / segments;
            Vertex3D_PBR v;
            v.position = {r * std::cos(theta), y, r * std::sin(theta)};
            float len = std::sqrt(v.position.x * v.position.x + (v.position.y - halfHeight) * (v.position.y - halfHeight) + v.position.z * v.position.z);
            v.normal = {v.position.x / len, (v.position.y - halfHeight) / len, v.position.z / len};
            v.texcoord = {static_cast<float>(j) / segments, static_cast<float>(i) / (2 * rings + 1)};
            v.tangent = {-std::sin(theta), 0, std::cos(theta), 1.0f};
            mesh.vertices.push_back(v);
        }
    }

    // Cylinder
    for (uint32_t i = 0; i <= 1; ++i)
    {
        float y = halfHeight - static_cast<float>(i) * height;
        for (uint32_t j = 0; j <= segments; ++j)
        {
            float theta = 2.0f * PI * static_cast<float>(j) / segments;
            Vertex3D_PBR v;
            v.position = {radius * std::cos(theta), y, radius * std::sin(theta)};
            v.normal = {std::cos(theta), 0.0f, std::sin(theta)};
            v.texcoord = {static_cast<float>(j) / segments, static_cast<float>(rings + i) / (2 * rings + 1)};
            v.tangent = {-std::sin(theta), 0, std::cos(theta), 1.0f};
            mesh.vertices.push_back(v);
        }
    }

    // Bottom hemisphere
    for (uint32_t i = 0; i <= rings; ++i)
    {
        float phi = PI * 0.5f + PI * 0.5f * static_cast<float>(i) / rings;
        float y = -halfHeight + radius * std::cos(phi);
        float r = radius * std::sin(phi);
        for (uint32_t j = 0; j <= segments; ++j)
        {
            float theta = 2.0f * PI * static_cast<float>(j) / segments;
            Vertex3D_PBR v;
            v.position = {r * std::cos(theta), y, r * std::sin(theta)};
            float len = std::sqrt(v.position.x * v.position.x + (v.position.y + halfHeight) * (v.position.y + halfHeight) + v.position.z * v.position.z);
            if (len < 1e-6f) len = 1.0f;
            v.normal = {v.position.x / len, (v.position.y + halfHeight) / len, v.position.z / len};
            v.texcoord = {static_cast<float>(j) / segments, static_cast<float>(rings + 2 + i) / (2 * rings + 1)};
            v.tangent = {-std::sin(theta), 0, std::cos(theta), 1.0f};
            mesh.vertices.push_back(v);
        }
    }

    // Generate indices for all rings
    uint32_t totalRings = rings + 2 + rings; // top hemisphere + cylinder(2) + bottom hemisphere
    for (uint32_t i = 0; i < totalRings; ++i)
    {
        for (uint32_t j = 0; j < segments; ++j)
        {
            uint32_t a = i * (segments + 1) + j;
            uint32_t b = a + segments + 1;
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(a + 1);
            mesh.indices.push_back(a + 1);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b + 1);
        }
    }

    return mesh;
}

MeshData MeshGenerator::CreateIcosphere(float radius, uint32_t subdivisions)
{
    MeshData mesh;
    constexpr float t = 1.618033988749895f; // golden ratio

    // 12 initial vertices of icosahedron
    auto addVert = [&](float x, float y, float z) {
        float len = std::sqrt(x*x + y*y + z*z);
        Vertex3D_PBR v;
        v.position = {x/len*radius, y/len*radius, z/len*radius};
        v.normal = {x/len, y/len, z/len};
        v.texcoord = {0.0f, 0.0f}; // Will be calculated later
        v.tangent = {1,0,0,1};
        mesh.vertices.push_back(v);
    };

    addVert(-1,  t, 0); addVert( 1,  t, 0); addVert(-1, -t, 0); addVert( 1, -t, 0);
    addVert( 0, -1,  t); addVert( 0,  1,  t); addVert( 0, -1, -t); addVert( 0,  1, -t);
    addVert( t,  0, -1); addVert( t,  0,  1); addVert(-t,  0, -1); addVert(-t,  0,  1);

    // 20 triangles of icosahedron
    uint32_t faces[] = {
        0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
        1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
        3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
        4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1
    };
    for (int i = 0; i < 60; ++i) mesh.indices.push_back(faces[i]);

    // Subdivide
    for (uint32_t s = 0; s < subdivisions; ++s)
    {
        std::vector<uint32_t> newIndices;
        std::map<uint64_t, uint32_t> midpointCache;

        auto getMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t {
            uint64_t key = (static_cast<uint64_t>(std::min(a,b)) << 32ULL) | std::max(a,b);
            auto it = midpointCache.find(key);
            if (it != midpointCache.end()) return it->second;

            auto& va = mesh.vertices[a];
            auto& vb = mesh.vertices[b];
            float mx = (va.position.x + vb.position.x) * 0.5f;
            float my = (va.position.y + vb.position.y) * 0.5f;
            float mz = (va.position.z + vb.position.z) * 0.5f;
            float len = std::sqrt(mx*mx + my*my + mz*mz);
            Vertex3D_PBR v;
            v.position = {mx/len*radius, my/len*radius, mz/len*radius};
            v.normal = {mx/len, my/len, mz/len};
            v.texcoord = {0,0};
            v.tangent = {1,0,0,1};
            uint32_t idx = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back(v);
            midpointCache[key] = idx;
            return idx;
        };

        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            uint32_t v0 = mesh.indices[i], v1 = mesh.indices[i+1], v2 = mesh.indices[i+2];
            uint32_t m01 = getMidpoint(v0, v1);
            uint32_t m12 = getMidpoint(v1, v2);
            uint32_t m02 = getMidpoint(v0, v2);
            newIndices.push_back(v0);  newIndices.push_back(m01); newIndices.push_back(m02);
            newIndices.push_back(v1);  newIndices.push_back(m12); newIndices.push_back(m01);
            newIndices.push_back(v2);  newIndices.push_back(m02); newIndices.push_back(m12);
            newIndices.push_back(m01); newIndices.push_back(m12); newIndices.push_back(m02);
        }
        mesh.indices = std::move(newIndices);
    }

    return mesh;
}

MeshData MeshGenerator::CreateCone(float radius, float height, uint32_t sliceCount)
{
    constexpr float PI = 3.14159265358979323846f;
    MeshData mesh;
    float halfH = height * 0.5f;

    // Apex
    Vertex3D_PBR apex;
    apex.position = {0, halfH, 0};
    apex.normal = {0, 1, 0};
    apex.texcoord = {0.5f, 0};
    apex.tangent = {1,0,0,1};
    mesh.vertices.push_back(apex);

    // Base ring
    for (uint32_t i = 0; i <= sliceCount; ++i)
    {
        float theta = 2.0f * PI * static_cast<float>(i) / sliceCount;
        float c = std::cos(theta), s = std::sin(theta);
        float slope = radius / height;
        float ny = slope;
        float nxz = 1.0f / std::sqrt(1.0f + slope * slope);
        Vertex3D_PBR v;
        v.position = {radius * c, -halfH, radius * s};
        v.normal = {c * nxz, ny, s * nxz};
        float len = std::sqrt(v.normal.x*v.normal.x + v.normal.y*v.normal.y + v.normal.z*v.normal.z);
        if (len > 1e-6f) { v.normal.x /= len; v.normal.y /= len; v.normal.z /= len; }
        v.texcoord = {static_cast<float>(i) / sliceCount, 1};
        v.tangent = {-s, 0, c, 1};
        mesh.vertices.push_back(v);
    }

    // Side faces
    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i + 2);
    }

    // Bottom cap center
    uint32_t centerIdx = static_cast<uint32_t>(mesh.vertices.size());
    Vertex3D_PBR center;
    center.position = {0, -halfH, 0};
    center.normal = {0, -1, 0};
    center.texcoord = {0.5f, 0.5f};
    center.tangent = {1,0,0,1};
    mesh.vertices.push_back(center);

    uint32_t baseStart = static_cast<uint32_t>(mesh.vertices.size());
    for (uint32_t i = 0; i <= sliceCount; ++i)
    {
        float theta = 2.0f * PI * static_cast<float>(i) / sliceCount;
        Vertex3D_PBR v;
        v.position = {radius * std::cos(theta), -halfH, radius * std::sin(theta)};
        v.normal = {0, -1, 0};
        v.texcoord = {std::cos(theta)*0.5f+0.5f, std::sin(theta)*0.5f+0.5f};
        v.tangent = {1,0,0,1};
        mesh.vertices.push_back(v);
    }

    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        mesh.indices.push_back(centerIdx);
        mesh.indices.push_back(baseStart + i + 1);
        mesh.indices.push_back(baseStart + i);
    }

    return mesh;
}

MeshData MeshGenerator::CreateArrow(float shaftRadius, float shaftLength,
                                     float headRadius, float headLength, uint32_t sliceCount)
{
    MeshData shaft = CreateCylinder(shaftRadius, shaftRadius, shaftLength, sliceCount, 1);
    MeshData head = CreateCone(headRadius, headLength, sliceCount);

    // Offset head to top of shaft
    float headOffset = shaftLength * 0.5f + headLength * 0.5f;
    for (auto& v : head.vertices)
        v.position.y += headOffset;

    return Combine(shaft, head);
}

void MeshGenerator::ComputeNormals(MeshData& mesh)
{
    ProceduralMeshBuilder builder;
    for (auto& v : mesh.vertices)
        builder.AddVertex(v.position, v.normal, v.texcoord, v.tangent);
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        builder.AddTriangle(mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]);
    builder.ComputeNormals();
    mesh = builder.Build();
}

void MeshGenerator::ComputeTangents(MeshData& mesh)
{
    ProceduralMeshBuilder builder;
    for (auto& v : mesh.vertices)
        builder.AddVertex(v.position, v.normal, v.texcoord, v.tangent);
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        builder.AddTriangle(mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]);
    builder.ComputeTangents();
    mesh = builder.Build();
}

MeshData MeshGenerator::Combine(const MeshData& a, const MeshData& b)
{
    MeshData result;
    result.vertices = a.vertices;
    result.vertices.insert(result.vertices.end(), b.vertices.begin(), b.vertices.end());
    result.indices = a.indices;
    uint32_t offset = static_cast<uint32_t>(a.vertices.size());
    for (uint32_t idx : b.indices)
        result.indices.push_back(idx + offset);
    return result;
}

} // namespace gx
