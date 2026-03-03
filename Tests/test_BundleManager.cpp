/// @file test_BundleManager.cpp
/// @brief BundleManager テスト — GUID解決、依存関係、クロスバンドル解決

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/BundleManager.h"
#include "Core/GUID.h"

// ============================================================================
// テストフィクスチャ
// ============================================================================

class BundleManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        gx::BundleManager::Instance().Clear();
    }

    void TearDown() override
    {
        gx::BundleManager::Instance().Clear();
    }

    // テスト用GUID生成ヘルパー
    gx::GUID MakeGUID() { return gx::GUID::Generate(); }

    // テスト用バンドルマニフェストを作成
    gx::BundleManifest MakeManifest(const gx::String& path,
                                     const gx::HashMap<gx::GUID, gx::String>& assets,
                                     const gx::Vector<gx::GUID>& deps = {})
    {
        gx::BundleManifest m;
        m.pakPath = path;
        m.containedAssets = assets;
        m.externalDependencies = deps;
        return m;
    }
};

// ============================================================================
// RegisterBundleInfo 基本テスト
// ============================================================================

TEST_F(BundleManagerTest, RegisterBundleInfo)
{
    gx::GUID g1 = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g1] = "textures/hero.png";

    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle_a.gxpak", assets));

    EXPECT_EQ(gx::BundleManager::Instance().GetBundleCount(), 1u);
    EXPECT_EQ(gx::BundleManager::Instance().GetTotalAssetCount(), 1u);
}

TEST_F(BundleManagerTest, RegisterMultipleBundles)
{
    gx::GUID g1 = MakeGUID();
    gx::GUID g2 = MakeGUID();
    gx::GUID g3 = MakeGUID();

    gx::HashMap<gx::GUID, gx::String> assets_a;
    assets_a[g1] = "models/hero.gxmd";

    gx::HashMap<gx::GUID, gx::String> assets_b;
    assets_b[g2] = "textures/hero.png";
    assets_b[g3] = "textures/hero_n.png";

    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle_a.gxpak", assets_a));
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle_b.gxpak", assets_b));

    EXPECT_EQ(gx::BundleManager::Instance().GetBundleCount(), 2u);
    EXPECT_EQ(gx::BundleManager::Instance().GetTotalAssetCount(), 3u);
}

// ============================================================================
// Resolve テスト
// ============================================================================

TEST_F(BundleManagerTest, ResolveFound)
{
    gx::GUID g1 = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g1] = "textures/hero.png";

    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle_a.gxpak", assets));

    auto result = gx::BundleManager::Instance().Resolve(g1);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.bundlePath, gx::String("bundle_a.gxpak"));
    EXPECT_EQ(result.internalPath, gx::String("textures/hero.png"));
}

TEST_F(BundleManagerTest, ResolveNotFound)
{
    gx::GUID unknown = MakeGUID();
    auto result = gx::BundleManager::Instance().Resolve(unknown);
    EXPECT_FALSE(result.found);
}

TEST_F(BundleManagerTest, ResolveBoolConversion)
{
    gx::GUID g1 = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g1] = "data/config.json";

    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("config.gxpak", assets));

    EXPECT_TRUE(static_cast<bool>(gx::BundleManager::Instance().Resolve(g1)));
    EXPECT_FALSE(static_cast<bool>(gx::BundleManager::Instance().Resolve(MakeGUID())));
}

// ============================================================================
// クロスバンドル解決テスト
// ============================================================================

TEST_F(BundleManagerTest, CrossBundleResolve)
{
    gx::GUID g1 = MakeGUID();
    gx::GUID g2 = MakeGUID();

    gx::HashMap<gx::GUID, gx::String> assets_a;
    assets_a[g1] = "models/hero.gxmd";

    gx::HashMap<gx::GUID, gx::String> assets_b;
    assets_b[g2] = "textures/hero.png";

    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("models.gxpak", assets_a));
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("textures.gxpak", assets_b));

    auto r1 = gx::BundleManager::Instance().Resolve(g1);
    auto r2 = gx::BundleManager::Instance().Resolve(g2);

    EXPECT_TRUE(r1.found);
    EXPECT_EQ(r1.bundlePath, gx::String("models.gxpak"));

    EXPECT_TRUE(r2.found);
    EXPECT_EQ(r2.bundlePath, gx::String("textures.gxpak"));
}

// ============================================================================
// AreDependenciesMet テスト
// ============================================================================

TEST_F(BundleManagerTest, AreDependenciesMetAllResolved)
{
    gx::GUID texGuid = MakeGUID();

    // テクスチャバンドル（依存なし）
    gx::HashMap<gx::GUID, gx::String> texAssets;
    texAssets[texGuid] = "textures/hero.png";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("textures.gxpak", texAssets));

    // モデルバンドル（テクスチャに依存）
    gx::GUID modelGuid = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> modelAssets;
    modelAssets[modelGuid] = "models/hero.gxmd";
    gx::Vector<gx::GUID> deps;
    deps.push_back(texGuid);
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("models.gxpak", modelAssets, deps));

    EXPECT_TRUE(gx::BundleManager::Instance().AreDependenciesMet("models.gxpak"));
}

TEST_F(BundleManagerTest, AreDependenciesMetMissing)
{
    gx::GUID missingGuid = MakeGUID();

    gx::GUID modelGuid = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> modelAssets;
    modelAssets[modelGuid] = "models/hero.gxmd";
    gx::Vector<gx::GUID> deps;
    deps.push_back(missingGuid);
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("models.gxpak", modelAssets, deps));

    EXPECT_FALSE(gx::BundleManager::Instance().AreDependenciesMet("models.gxpak"));
}

TEST_F(BundleManagerTest, AreDependenciesMetNoDeps)
{
    gx::GUID g1 = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g1] = "data.bin";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("standalone.gxpak", assets));

    EXPECT_TRUE(gx::BundleManager::Instance().AreDependenciesMet("standalone.gxpak"));
}

TEST_F(BundleManagerTest, AreDependenciesMetUnknownBundle)
{
    EXPECT_FALSE(gx::BundleManager::Instance().AreDependenciesMet("nonexistent.gxpak"));
}

// ============================================================================
// GetMissingDependencies テスト
// ============================================================================

TEST_F(BundleManagerTest, GetMissingDependenciesNone)
{
    gx::GUID depGuid = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> depAssets;
    depAssets[depGuid] = "tex.png";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("dep.gxpak", depAssets));

    gx::GUID mainGuid = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> mainAssets;
    mainAssets[mainGuid] = "model.gxmd";
    gx::Vector<gx::GUID> deps;
    deps.push_back(depGuid);
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("main.gxpak", mainAssets, deps));

    auto missing = gx::BundleManager::Instance().GetMissingDependencies("main.gxpak");
    EXPECT_TRUE(missing.empty());
}

TEST_F(BundleManagerTest, GetMissingDependenciesSome)
{
    gx::GUID missing1 = MakeGUID();
    gx::GUID missing2 = MakeGUID();
    gx::GUID present = MakeGUID();

    // present は登録する
    gx::HashMap<gx::GUID, gx::String> presentAssets;
    presentAssets[present] = "tex.png";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("present.gxpak", presentAssets));

    // 3つの依存のうち2つが未解決
    gx::GUID mainGuid = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> mainAssets;
    mainAssets[mainGuid] = "model.gxmd";
    gx::Vector<gx::GUID> deps;
    deps.push_back(present);
    deps.push_back(missing1);
    deps.push_back(missing2);
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("main.gxpak", mainAssets, deps));

    auto missingList = gx::BundleManager::Instance().GetMissingDependencies("main.gxpak");
    EXPECT_EQ(missingList.size(), 2u);
}

TEST_F(BundleManagerTest, GetMissingDependenciesUnknownBundle)
{
    auto missing = gx::BundleManager::Instance().GetMissingDependencies("nonexistent.gxpak");
    EXPECT_TRUE(missing.empty());
}

// ============================================================================
// UnloadBundle テスト
// ============================================================================

TEST_F(BundleManagerTest, UnloadBundleRemovesGUIDs)
{
    gx::GUID g1 = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g1] = "tex.png";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle.gxpak", assets));

    EXPECT_TRUE(gx::BundleManager::Instance().Resolve(g1).found);

    gx::BundleManager::Instance().UnloadBundle("bundle.gxpak");
    EXPECT_FALSE(gx::BundleManager::Instance().Resolve(g1).found);
    EXPECT_EQ(gx::BundleManager::Instance().GetBundleCount(), 0u);
}

TEST_F(BundleManagerTest, UnloadBundleDoesNotAffectOthers)
{
    gx::GUID g1 = MakeGUID();
    gx::GUID g2 = MakeGUID();

    gx::HashMap<gx::GUID, gx::String> assets_a;
    assets_a[g1] = "a.dat";
    gx::HashMap<gx::GUID, gx::String> assets_b;
    assets_b[g2] = "b.dat";

    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("a.gxpak", assets_a));
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("b.gxpak", assets_b));

    gx::BundleManager::Instance().UnloadBundle("a.gxpak");
    EXPECT_FALSE(gx::BundleManager::Instance().Resolve(g1).found);
    EXPECT_TRUE(gx::BundleManager::Instance().Resolve(g2).found);
}

TEST_F(BundleManagerTest, UnloadNonExistent)
{
    // クラッシュしないことを確認
    gx::BundleManager::Instance().UnloadBundle("ghost.gxpak");
    EXPECT_EQ(gx::BundleManager::Instance().GetBundleCount(), 0u);
}

// ============================================================================
// Clear テスト
// ============================================================================

TEST_F(BundleManagerTest, ClearRemovesEverything)
{
    gx::GUID g1 = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g1] = "data.bin";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle.gxpak", assets));

    gx::BundleManager::Instance().Clear();
    EXPECT_EQ(gx::BundleManager::Instance().GetBundleCount(), 0u);
    EXPECT_EQ(gx::BundleManager::Instance().GetTotalAssetCount(), 0u);
}

// ============================================================================
// 依存解決後のロードテスト
// ============================================================================

TEST_F(BundleManagerTest, DependencyResolvedAfterLateLoad)
{
    gx::GUID texGuid = MakeGUID();
    gx::GUID modelGuid = MakeGUID();

    // モデルバンドル先にロード（依存未解決）
    gx::HashMap<gx::GUID, gx::String> modelAssets;
    modelAssets[modelGuid] = "hero.gxmd";
    gx::Vector<gx::GUID> deps;
    deps.push_back(texGuid);
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("models.gxpak", modelAssets, deps));
    EXPECT_FALSE(gx::BundleManager::Instance().AreDependenciesMet("models.gxpak"));

    // テクスチャバンドル後からロード
    gx::HashMap<gx::GUID, gx::String> texAssets;
    texAssets[texGuid] = "hero.png";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("textures.gxpak", texAssets));

    // 依存解決
    EXPECT_TRUE(gx::BundleManager::Instance().AreDependenciesMet("models.gxpak"));
    EXPECT_TRUE(gx::BundleManager::Instance().GetMissingDependencies("models.gxpak").empty());
}

// ============================================================================
// 複数依存チェーンテスト
// ============================================================================

TEST_F(BundleManagerTest, MultiDependencyChain)
{
    gx::GUID shaderGuid = MakeGUID();
    gx::GUID texGuid = MakeGUID();
    gx::GUID modelGuid = MakeGUID();

    // シェーダーバンドル（依存なし）
    gx::HashMap<gx::GUID, gx::String> shaderAssets;
    shaderAssets[shaderGuid] = "pbr.hlsl";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("shaders.gxpak", shaderAssets));

    // テクスチャバンドル（依存なし）
    gx::HashMap<gx::GUID, gx::String> texAssets;
    texAssets[texGuid] = "hero.png";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("textures.gxpak", texAssets));

    // モデルバンドル（シェーダー+テクスチャに依存）
    gx::HashMap<gx::GUID, gx::String> modelAssets;
    modelAssets[modelGuid] = "hero.gxmd";
    gx::Vector<gx::GUID> deps;
    deps.push_back(shaderGuid);
    deps.push_back(texGuid);
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("models.gxpak", modelAssets, deps));

    EXPECT_TRUE(gx::BundleManager::Instance().AreDependenciesMet("models.gxpak"));
}

// ============================================================================
// ReadByGUID テスト（ファイルなしでは失敗を確認）
// ============================================================================

TEST_F(BundleManagerTest, ReadByGUIDUnknown)
{
    auto data = gx::BundleManager::Instance().ReadByGUID(MakeGUID());
    EXPECT_FALSE(data.IsValid());
}

TEST_F(BundleManagerTest, ReadByGUIDNoFile)
{
    gx::GUID g = MakeGUID();
    gx::HashMap<gx::GUID, gx::String> assets;
    assets[g] = "data.bin";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("nonexistent_bundle.gxpak", assets));

    // ファイルが存在しないのでReadは失敗する
    auto data = gx::BundleManager::Instance().ReadByGUID(g);
    EXPECT_FALSE(data.IsValid());
}

// ============================================================================
// ResolveResult 構造体テスト
// ============================================================================

TEST_F(BundleManagerTest, ResolveResultDefault)
{
    gx::ResolveResult r;
    EXPECT_FALSE(r.found);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_TRUE(r.bundlePath.empty());
    EXPECT_TRUE(r.internalPath.empty());
}

// ============================================================================
// バンドル再登録テスト
// ============================================================================

TEST_F(BundleManagerTest, OverwriteRegistration)
{
    gx::GUID g1 = MakeGUID();
    gx::GUID g2 = MakeGUID();

    gx::HashMap<gx::GUID, gx::String> assets1;
    assets1[g1] = "old.dat";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle.gxpak", assets1));

    // 同じバンドルを再登録（アセット差し替え）
    gx::HashMap<gx::GUID, gx::String> assets2;
    assets2[g2] = "new.dat";
    gx::BundleManager::Instance().RegisterBundleInfo(
        MakeManifest("bundle.gxpak", assets2));

    EXPECT_EQ(gx::BundleManager::Instance().GetBundleCount(), 1u);
    EXPECT_TRUE(gx::BundleManager::Instance().Resolve(g2).found);
}
