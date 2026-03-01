/// @file test_ProceduralMesh.cpp
/// @brief ProceduralMeshBuilder + MeshGenerator拡張形状テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/ProceduralMesh.h"
#include "Graphics/3D/MeshData.h"

using namespace gx;

static constexpr float kEps = 1e-4f;

// ==================== ProceduralMeshBuilder ====================

TEST(ProceduralMeshBuilderTest, AddVertex_IncrementsCount)
{
    ProceduralMeshBuilder builder;
    EXPECT_EQ(builder.GetVertexCount(), 0u);
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    EXPECT_EQ(builder.GetVertexCount(), 1u);
}

TEST(ProceduralMeshBuilderTest, AddVertex_ReturnsIndex)
{
    ProceduralMeshBuilder builder;
    uint32_t i0 = builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    uint32_t i1 = builder.AddVertex({1,0,0}, {0,1,0}, {1,0});
    EXPECT_EQ(i0, 0u);
    EXPECT_EQ(i1, 1u);
}

TEST(ProceduralMeshBuilderTest, AddTriangle_AddsThreeIndices)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    builder.AddVertex({1,0,0}, {0,1,0}, {1,0});
    builder.AddVertex({0,0,1}, {0,1,0}, {0,1});
    builder.AddTriangle(0, 1, 2);
    EXPECT_EQ(builder.GetIndexCount(), 3u);
}

TEST(ProceduralMeshBuilderTest, Build_ReturnsValidMeshData)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    builder.AddVertex({1,0,0}, {0,1,0}, {1,0});
    builder.AddVertex({0,0,1}, {0,1,0}, {0,1});
    builder.AddTriangle(0, 1, 2);
    MeshData mesh = builder.Build();
    EXPECT_EQ(mesh.vertices.size(), 3u);
    EXPECT_EQ(mesh.indices.size(), 3u);
}

TEST(ProceduralMeshBuilderTest, Clear_ResetsToEmpty)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    builder.AddTriangle(0, 0, 0);
    builder.Clear();
    EXPECT_EQ(builder.GetVertexCount(), 0u);
    EXPECT_EQ(builder.GetIndexCount(), 0u);
}

TEST(ProceduralMeshBuilderTest, Append_MergesMeshData)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});

    MeshData other;
    Vertex3D_PBR v; v.position = {1,0,0}; v.normal = {0,1,0}; v.texcoord = {1,0}; v.tangent = {1,0,0,1};
    other.vertices.push_back(v);
    other.indices.push_back(0);

    builder.Append(other);
    EXPECT_EQ(builder.GetVertexCount(), 2u);
    EXPECT_EQ(builder.GetIndexCount(), 1u);
}

TEST(ProceduralMeshBuilderTest, Append_OffsetsIndices)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    builder.AddVertex({1,0,0}, {0,1,0}, {1,0});

    MeshData other;
    Vertex3D_PBR v; v.position = {2,0,0}; v.normal = {0,1,0}; v.texcoord = {0,0}; v.tangent = {1,0,0,1};
    other.vertices.push_back(v);
    other.indices = {0};

    builder.Append(other);
    MeshData result = builder.Build();
    // The appended index 0 should have become index 2 (offset by 2 existing verts)
    EXPECT_EQ(result.indices.back(), 2u);
}

TEST(ProceduralMeshBuilderTest, ComputeNormals_FlatPlane)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,0,0}, {0,0});
    builder.AddVertex({1,0,0}, {0,0,0}, {1,0});
    builder.AddVertex({0,0,1}, {0,0,0}, {0,1});
    builder.AddTriangle(0, 1, 2);
    builder.ComputeNormals();
    MeshData mesh = builder.Build();
    for (auto& v : mesh.vertices)
    {
        EXPECT_NEAR(v.normal.y, 1.0f, 0.1f); // Approximately up
    }
}

TEST(ProceduralMeshBuilderTest, ComputeTangents_OrthogonalToNormals)
{
    ProceduralMeshBuilder builder;
    builder.AddVertex({0,0,0}, {0,1,0}, {0,0});
    builder.AddVertex({1,0,0}, {0,1,0}, {1,0});
    builder.AddVertex({0,0,1}, {0,1,0}, {0,1});
    builder.AddTriangle(0, 1, 2);
    builder.ComputeTangents();
    MeshData mesh = builder.Build();
    for (auto& v : mesh.vertices)
    {
        float dot = v.normal.x * v.tangent.x + v.normal.y * v.tangent.y + v.normal.z * v.tangent.z;
        EXPECT_NEAR(dot, 0.0f, 0.1f);
    }
}

// ==================== MeshGenerator::Combine ====================

TEST(MeshCombineTest, CombinedVertexCount)
{
    MeshData a = MeshGenerator::CreateBox(1, 1, 1);
    MeshData b = MeshGenerator::CreateSphere(0.5f, 8, 8);
    MeshData combined = MeshGenerator::Combine(a, b);
    EXPECT_EQ(combined.vertices.size(), a.vertices.size() + b.vertices.size());
}

TEST(MeshCombineTest, CombinedIndexCount)
{
    MeshData a = MeshGenerator::CreateBox(1, 1, 1);
    MeshData b = MeshGenerator::CreateSphere(0.5f, 8, 8);
    MeshData combined = MeshGenerator::Combine(a, b);
    EXPECT_EQ(combined.indices.size(), a.indices.size() + b.indices.size());
}

// ==================== Torus ====================

TEST(MeshGeneratorExtTest, Torus_VertexCount)
{
    uint32_t major = 16, minor = 8;
    MeshData mesh = MeshGenerator::CreateTorus(1.0f, 0.3f, major, minor);
    EXPECT_EQ(mesh.vertices.size(), (major + 1) * (minor + 1));
}

TEST(MeshGeneratorExtTest, Torus_IndexCount)
{
    uint32_t major = 16, minor = 8;
    MeshData mesh = MeshGenerator::CreateTorus(1.0f, 0.3f, major, minor);
    EXPECT_EQ(mesh.indices.size(), major * minor * 6);
}

TEST(MeshGeneratorExtTest, Torus_NonEmpty)
{
    MeshData mesh = MeshGenerator::CreateTorus(2.0f, 0.5f, 32, 16);
    EXPECT_GT(mesh.vertices.size(), 0u);
    EXPECT_GT(mesh.indices.size(), 0u);
}

// ==================== Capsule ====================

TEST(MeshGeneratorExtTest, Capsule_HasVertices)
{
    MeshData mesh = MeshGenerator::CreateCapsule(0.5f, 2.0f, 16);
    EXPECT_GT(mesh.vertices.size(), 0u);
    EXPECT_GT(mesh.indices.size(), 0u);
}

TEST(MeshGeneratorExtTest, Capsule_TopAndBottomExist)
{
    MeshData mesh = MeshGenerator::CreateCapsule(0.5f, 2.0f, 16);
    float maxY = -1e10f, minY = 1e10f;
    for (auto& v : mesh.vertices) {
        maxY = std::max(maxY, v.position.y);
        minY = std::min(minY, v.position.y);
    }
    EXPECT_GT(maxY, 0.5f);  // Top hemisphere
    EXPECT_LT(minY, -0.5f); // Bottom hemisphere
}

// ==================== Icosphere ====================

TEST(MeshGeneratorExtTest, Icosphere_Sub0_Has12Verts)
{
    MeshData mesh = MeshGenerator::CreateIcosphere(1.0f, 0);
    EXPECT_EQ(mesh.vertices.size(), 12u);
    EXPECT_EQ(mesh.indices.size(), 60u); // 20 faces * 3
}

TEST(MeshGeneratorExtTest, Icosphere_Sub1_HasMoreVerts)
{
    MeshData mesh0 = MeshGenerator::CreateIcosphere(1.0f, 0);
    MeshData mesh1 = MeshGenerator::CreateIcosphere(1.0f, 1);
    EXPECT_GT(mesh1.vertices.size(), mesh0.vertices.size());
    EXPECT_GT(mesh1.indices.size(), mesh0.indices.size());
}

TEST(MeshGeneratorExtTest, Icosphere_VerticesOnSphere)
{
    MeshData mesh = MeshGenerator::CreateIcosphere(2.0f, 1);
    for (auto& v : mesh.vertices)
    {
        float dist = std::sqrt(v.position.x * v.position.x + v.position.y * v.position.y + v.position.z * v.position.z);
        EXPECT_NEAR(dist, 2.0f, 0.01f);
    }
}

// ==================== Cone ====================

TEST(MeshGeneratorExtTest, Cone_HasVertices)
{
    MeshData mesh = MeshGenerator::CreateCone(1.0f, 2.0f, 16);
    EXPECT_GT(mesh.vertices.size(), 0u);
    EXPECT_GT(mesh.indices.size(), 0u);
}

TEST(MeshGeneratorExtTest, Cone_ApexAtTop)
{
    MeshData mesh = MeshGenerator::CreateCone(1.0f, 2.0f, 16);
    // First vertex should be the apex
    EXPECT_NEAR(mesh.vertices[0].position.y, 1.0f, kEps); // halfHeight
}

// ==================== Arrow ====================

TEST(MeshGeneratorExtTest, Arrow_NonEmpty)
{
    MeshData mesh = MeshGenerator::CreateArrow(0.1f, 1.0f, 0.2f, 0.3f, 16);
    EXPECT_GT(mesh.vertices.size(), 0u);
    EXPECT_GT(mesh.indices.size(), 0u);
}

TEST(MeshGeneratorExtTest, Arrow_LargerThanShaftAlone)
{
    MeshData shaft = MeshGenerator::CreateCylinder(0.1f, 0.1f, 1.0f, 16, 1);
    MeshData arrow = MeshGenerator::CreateArrow(0.1f, 1.0f, 0.2f, 0.3f, 16);
    EXPECT_GT(arrow.vertices.size(), shaft.vertices.size());
}

// ==================== ComputeNormals/Tangents on MeshData ====================

TEST(MeshGeneratorExtTest, ComputeNormals_OnBox)
{
    MeshData mesh = MeshGenerator::CreateBox(1, 1, 1);
    MeshGenerator::ComputeNormals(mesh);
    for (auto& v : mesh.vertices)
    {
        float len = std::sqrt(v.normal.x*v.normal.x + v.normal.y*v.normal.y + v.normal.z*v.normal.z);
        EXPECT_NEAR(len, 1.0f, 0.1f);
    }
}

TEST(MeshGeneratorExtTest, ComputeTangents_OnBox)
{
    MeshData mesh = MeshGenerator::CreateBox(1, 1, 1);
    MeshGenerator::ComputeTangents(mesh);
    // Just verify no NaN
    for (auto& v : mesh.vertices)
    {
        EXPECT_FALSE(std::isnan(v.tangent.x));
        EXPECT_FALSE(std::isnan(v.tangent.y));
        EXPECT_FALSE(std::isnan(v.tangent.z));
    }
}
