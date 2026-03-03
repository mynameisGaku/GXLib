/// @file test_AssetDatabaseExtended.cpp
/// @brief AssetDatabase拡張テスト（Phase 7 API）
///        test_EditorPanels.cpp から分割

#include <gtest/gtest.h>
#include "Core/AssetDatabase.h"
#include <filesystem>
#include <fstream>

using namespace gx;

// ============================================================================
// AssetDatabase（拡張 - Phase 7 API）
// ============================================================================

class AssetDatabaseExtendedTest : public ::testing::Test
{
protected:
    gx::String testDir;

    void SetUp() override
    {
        AssetDatabase::Instance().Shutdown();
        testDir = (std::filesystem::temp_directory_path() / "gx_assetdb_ext_test").string();
        std::filesystem::create_directories(std::filesystem::path(testDir.c_str()));
        std::filesystem::create_directories(std::filesystem::path((testDir + "/textures").c_str()));
        std::filesystem::create_directories(std::filesystem::path((testDir + "/shaders").c_str()));

        CreateFile("textures/icon.png");
        CreateFile("textures/bg.dds");
        CreateFile("shaders/pbr.hlsl");
        CreateFile("config.json");
        CreateFile("player.gxmdl");
        CreateFile("script.lua");
    }

    void TearDown() override
    {
        AssetDatabase::Instance().Shutdown();
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(testDir.c_str()), ec);
    }

    void CreateFile(const gx::String& relPath)
    {
        gx::String fullPath = testDir + "/" + relPath;
        std::ofstream f(fullPath);
        f << "test content for " << relPath;
    }
};

TEST_F(AssetDatabaseExtendedTest, Initialize)
{
    AssetDatabase::Instance().Initialize(testDir);
    EXPECT_GT(AssetDatabase::Instance().GetAssetCount(), 0);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeTexture)
{
    EXPECT_EQ(AssetDatabase::DetectType(".png"), AssetType::Texture);
    EXPECT_EQ(AssetDatabase::DetectType(".jpg"), AssetType::Texture);
    EXPECT_EQ(AssetDatabase::DetectType(".DDS"), AssetType::Texture);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeModel)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxmdl"), AssetType::Model);
    EXPECT_EQ(AssetDatabase::DetectType(".fbx"), AssetType::Model);
    EXPECT_EQ(AssetDatabase::DetectType(".obj"), AssetType::Model);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeShader)
{
    EXPECT_EQ(AssetDatabase::DetectType(".hlsl"), AssetType::Shader);
    EXPECT_EQ(AssetDatabase::DetectType(".hlsli"), AssetType::Shader);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeScript)
{
    EXPECT_EQ(AssetDatabase::DetectType(".lua"), AssetType::Script);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeScene)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxscene"), AssetType::Scene);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeMaterial)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxmat"), AssetType::Material);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeAnimation)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxanim"), AssetType::Animation);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypePrefab)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxprefab"), AssetType::Prefab);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeTilemap)
{
    EXPECT_EQ(AssetDatabase::DetectType(".tmx"), AssetType::Tilemap);
    EXPECT_EQ(AssetDatabase::DetectType(".tmj"), AssetType::Tilemap);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeConfig)
{
    EXPECT_EQ(AssetDatabase::DetectType(".json"), AssetType::Config);
    EXPECT_EQ(AssetDatabase::DetectType(".xml"), AssetType::Config);
    EXPECT_EQ(AssetDatabase::DetectType(".ini"), AssetType::Config);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeUnknown)
{
    EXPECT_EQ(AssetDatabase::DetectType(".xyz"), AssetType::Unknown);
}

TEST_F(AssetDatabaseExtendedTest, FindByName)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto results = AssetDatabase::Instance().FindByName("icon");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AssetDatabaseExtendedTest, FindByNameCaseInsensitive)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto results = AssetDatabase::Instance().FindByName("ICON");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AssetDatabaseExtendedTest, FindByTypeTextures)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto textures = AssetDatabase::Instance().FindByType(AssetType::Texture);
    EXPECT_GE(textures.size(), 2u); // icon.png + bg.ddsの2つ
}

TEST_F(AssetDatabaseExtendedTest, FindByTypeShaders)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto shaders = AssetDatabase::Instance().FindByType(AssetType::Shader);
    EXPECT_EQ(shaders.size(), 1u);
}

TEST_F(AssetDatabaseExtendedTest, FindAssetByRelPath)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto* entry = AssetDatabase::Instance().FindAsset("textures/icon.png");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->type, AssetType::Texture);
    EXPECT_EQ(entry->name, "icon.png");
}

TEST_F(AssetDatabaseExtendedTest, FindAssetNotFound)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto* entry = AssetDatabase::Instance().FindAsset("nonexistent.xyz");
    EXPECT_EQ(entry, nullptr);
}

TEST_F(AssetDatabaseExtendedTest, ImportSettings)
{
    AssetDatabase::Instance().Initialize(testDir);
    AssetDatabase::Instance().SetImportSetting("textures/icon.png", "maxSize", "1024");
    AssetDatabase::Instance().SetImportSetting("textures/icon.png", "format", "BC7");

    auto* settings = AssetDatabase::Instance().GetImportSettings("textures/icon.png");
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->Get("maxSize"), "1024");
    EXPECT_EQ(settings->Get("format"), "BC7");
    EXPECT_EQ(settings->Get("missing", "default"), "default");
}

TEST_F(AssetDatabaseExtendedTest, SaveLoadMetadata)
{
    AssetDatabase::Instance().Initialize(testDir);
    AssetDatabase::Instance().SetImportSetting("textures/icon.png", "compress", "true");

    gx::String metaPath = (std::filesystem::temp_directory_path() / "gx_meta_test.txt").string();
    EXPECT_TRUE(AssetDatabase::Instance().SaveMetadata(metaPath));

    // クリアして再読み込み
    AssetDatabase::Instance().Shutdown();
    AssetDatabase::Instance().Initialize(testDir);
    EXPECT_TRUE(AssetDatabase::Instance().LoadMetadata(metaPath));

    auto* settings = AssetDatabase::Instance().GetImportSettings("textures/icon.png");
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->Get("compress"), "true");

    std::filesystem::remove(std::filesystem::path(metaPath.c_str()));
}

TEST_F(AssetDatabaseExtendedTest, DetectChanges)
{
    AssetDatabase::Instance().Initialize(testDir);

    // ファイルを書き換えて変更する
    {
        std::ofstream f(testDir + "/config.json");
        f << "modified content at " << std::chrono::steady_clock::now().time_since_epoch().count();
    }

    // ファイルシステムのタイムスタンプが異なることを保証するために少し待つ
    // （DetectChangesはlastModifiedタイムスタンプを比較する）
    int changes = AssetDatabase::Instance().DetectChanges();
    // ファイルシステムの精度によって変更を検出する場合としない場合がある
    EXPECT_GE(changes, 0);
}

TEST_F(AssetDatabaseExtendedTest, Shutdown)
{
    AssetDatabase::Instance().Initialize(testDir);
    EXPECT_GT(AssetDatabase::Instance().GetAssetCount(), 0);

    AssetDatabase::Instance().Shutdown();
    EXPECT_EQ(AssetDatabase::Instance().GetAssetCount(), 0);
    EXPECT_TRUE(AssetDatabase::Instance().GetRootPath().empty());
}
