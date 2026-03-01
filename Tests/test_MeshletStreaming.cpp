/// @file test_MeshletStreaming.cpp
/// @brief MeshletStreaming のCPU側ロジックテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/MeshletStreaming.h"

using namespace gx;

// ヘルパー: テスト用メッシュデータ作成
static StreamableMesh MakeTestMesh(const std::string& path, uint32_t meshletCount, int lodCount)
{
    StreamableMesh mesh;
    mesh.assetPath = path;
    mesh.totalMeshletCount = meshletCount;
    mesh.boundingRadius = 5.0f;

    uint32_t offset = 0;
    uint32_t perLod = meshletCount / std::max(lodCount, 1);
    for (int i = 0; i < lodCount; ++i)
    {
        MeshletLOD lod;
        lod.meshletOffset = offset;
        lod.meshletCount = (i == lodCount - 1) ? (meshletCount - offset) : perLod;
        lod.maxScreenSize = 1.0f - (static_cast<float>(i) / static_cast<float>(lodCount));
        offset += lod.meshletCount;
        mesh.lods.push_back(lod);
    }

    return mesh;
}

// ============================================================================
// ConstructDestruct: 構築と破壊が安全に行えること
// ============================================================================
TEST(MeshletStreamingTest, ConstructDestruct)
{
    MeshletStreaming ms;
    EXPECT_EQ(ms.GetRegisteredMeshCount(), 0u);
    EXPECT_EQ(ms.GetResidentMeshletCount(), 0u);
}

// ============================================================================
// RegisterMesh: メッシュ登録
// ============================================================================
TEST(MeshletStreamingTest, RegisterMesh)
{
    MeshletStreaming ms;
    auto mesh = MakeTestMesh("test_mesh.gxmd", 256, 3);
    uint32_t id = ms.RegisterMesh(mesh);

    EXPECT_EQ(ms.GetRegisteredMeshCount(), 1u);
    EXPECT_EQ(id, 0u);
}

// ============================================================================
// UnregisterMesh: メッシュ登録解除
// ============================================================================
TEST(MeshletStreamingTest, UnregisterMesh)
{
    MeshletStreaming ms;
    auto mesh = MakeTestMesh("test_mesh.gxmd", 256, 3);
    uint32_t id = ms.RegisterMesh(mesh);

    ms.UnregisterMesh(id);
    EXPECT_EQ(ms.GetRegisteredMeshCount(), 0u);
}

// ============================================================================
// SetConfig: 設定変更
// ============================================================================
TEST(MeshletStreamingTest, SetConfig)
{
    MeshletStreaming ms;
    MeshletStreamingConfig config;
    config.lodBias = 1.5f;
    config.maxResidentMeshlets = 32768;
    config.loadBudgetPerFrame = 128;
    config.fadeDistance = 10.0f;

    ms.SetConfig(config);
    const auto& got = ms.GetConfig();

    EXPECT_FLOAT_EQ(got.lodBias, 1.5f);
    EXPECT_EQ(got.maxResidentMeshlets, 32768u);
    EXPECT_EQ(got.loadBudgetPerFrame, 128u);
    EXPECT_FLOAT_EQ(got.fadeDistance, 10.0f);
}

// ============================================================================
// GetConfig: デフォルト設定取得
// ============================================================================
TEST(MeshletStreamingTest, GetConfig)
{
    MeshletStreaming ms;
    const auto& config = ms.GetConfig();

    EXPECT_FLOAT_EQ(config.lodBias, 0.0f);
    EXPECT_EQ(config.maxResidentMeshlets, 65536u);
    EXPECT_EQ(config.loadBudgetPerFrame, 64u);
    EXPECT_FLOAT_EQ(config.fadeDistance, 5.0f);
}

// ============================================================================
// DefaultConfig: デフォルトコンフィグの各フィールド
// ============================================================================
TEST(MeshletStreamingTest, DefaultConfig)
{
    MeshletStreamingConfig config;
    EXPECT_FLOAT_EQ(config.lodBias, 0.0f);
    EXPECT_EQ(config.maxResidentMeshlets, 65536u);
    EXPECT_EQ(config.loadBudgetPerFrame, 64u);
    EXPECT_FLOAT_EQ(config.fadeDistance, 5.0f);
}

// ============================================================================
// RegisterMultiple: 複数メッシュの登録
// ============================================================================
TEST(MeshletStreamingTest, RegisterMultiple)
{
    MeshletStreaming ms;
    auto mesh1 = MakeTestMesh("mesh1.gxmd", 128, 2);
    auto mesh2 = MakeTestMesh("mesh2.gxmd", 256, 3);
    auto mesh3 = MakeTestMesh("mesh3.gxmd", 512, 4);

    uint32_t id1 = ms.RegisterMesh(mesh1);
    uint32_t id2 = ms.RegisterMesh(mesh2);
    uint32_t id3 = ms.RegisterMesh(mesh3);

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_EQ(ms.GetRegisteredMeshCount(), 3u);
}

// ============================================================================
// Stats: 統計情報の取得
// ============================================================================
TEST(MeshletStreamingTest, Stats)
{
    MeshletStreaming ms;
    auto mesh1 = MakeTestMesh("mesh1.gxmd", 128, 2);
    auto mesh2 = MakeTestMesh("mesh2.gxmd", 256, 3);

    ms.RegisterMesh(mesh1);
    ms.RegisterMesh(mesh2);

    auto stats = ms.GetStats();
    EXPECT_EQ(stats.registeredMeshes, 2u);
    // LOD 0のメッシュレット数が常駐としてカウントされる
    EXPECT_GT(stats.residentMeshlets, 0u);
}

// ============================================================================
// GetActiveLOD: LODレベル取得
// ============================================================================
TEST(MeshletStreamingTest, GetActiveLOD)
{
    MeshletStreaming ms;
    auto mesh = MakeTestMesh("test.gxmd", 256, 3);
    uint32_t id = ms.RegisterMesh(mesh);

    // 初期LODは0
    EXPECT_EQ(ms.GetActiveLOD(id), 0);

    // 無効なIDは-1
    EXPECT_EQ(ms.GetActiveLOD(999), -1);
}

// ============================================================================
// UnregisterReuse: 解除後のID再利用
// ============================================================================
TEST(MeshletStreamingTest, UnregisterReuse)
{
    MeshletStreaming ms;
    auto mesh1 = MakeTestMesh("mesh1.gxmd", 128, 2);
    auto mesh2 = MakeTestMesh("mesh2.gxmd", 256, 3);

    uint32_t id1 = ms.RegisterMesh(mesh1);
    ms.UnregisterMesh(id1);

    // 解除後のIDは再利用される
    uint32_t id2 = ms.RegisterMesh(mesh2);
    EXPECT_EQ(id1, id2);
    EXPECT_EQ(ms.GetRegisteredMeshCount(), 1u);

    // 解除されたIDのLODは-1
    ms.UnregisterMesh(id2);
    EXPECT_EQ(ms.GetActiveLOD(id2), -1);
}
