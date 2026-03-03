/// @file test_AssetGUID.cpp
/// @brief GUID, AssetMeta, PPtr, AssetDatabase GUID統合テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/GUID.h"
#include "Core/AssetMeta.h"
#include "Core/PPtr.h"
#include "Core/AssetDatabase.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// ============================================================================
// GUID 基本テスト
// ============================================================================

TEST(GUID, GenerateIsValid)
{
    gx::GUID g = gx::GUID::Generate();
    EXPECT_TRUE(g.IsValid());
}

TEST(GUID, DefaultIsInvalid)
{
    gx::GUID g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_EQ(g.hi, 0u);
    EXPECT_EQ(g.lo, 0u);
}

TEST(GUID, GenerateUniqueness)
{
    gx::HashSet<gx::String> seen;
    constexpr int count = 1000;
    for (int i = 0; i < count; ++i)
    {
        gx::GUID g = gx::GUID::Generate();
        EXPECT_TRUE(g.IsValid());
        gx::String s = g.ToString();
        EXPECT_EQ(seen.count(s), 0u) << "Duplicate GUID at iteration " << i;
        seen.insert(s);
    }
    EXPECT_EQ(static_cast<int>(seen.size()), count);
}

TEST(GUID, ToStringFormat)
{
    gx::GUID g = gx::GUID::Generate();
    gx::String s = g.ToString();
    // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    EXPECT_EQ(s.size(), 36u);
    EXPECT_EQ(s[8], '-');
    EXPECT_EQ(s[13], '-');
    EXPECT_EQ(s[18], '-');
    EXPECT_EQ(s[23], '-');
}

TEST(GUID, StringRoundtrip)
{
    gx::GUID original = gx::GUID::Generate();
    gx::String str = original.ToString();
    gx::GUID parsed = gx::GUID::FromString(str);
    EXPECT_EQ(original, parsed);
}

TEST(GUID, StringRoundtripMultiple)
{
    for (int i = 0; i < 100; ++i)
    {
        gx::GUID g = gx::GUID::Generate();
        gx::GUID parsed = gx::GUID::FromString(g.ToString());
        EXPECT_EQ(g, parsed) << "Failed at iteration " << i;
    }
}

TEST(GUID, FromStringInvalid)
{
    // 空文字列
    EXPECT_FALSE(gx::GUID::FromString("").IsValid());
    // 長さ不足
    EXPECT_FALSE(gx::GUID::FromString("abcdef").IsValid());
    // ダッシュ位置不正
    EXPECT_FALSE(gx::GUID::FromString("12345678x1234x1234x1234x123456789abc").IsValid());
    // 非hex文字
    EXPECT_FALSE(gx::GUID::FromString("zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz").IsValid());
}

TEST(GUID, EqualityOperator)
{
    gx::GUID a = gx::GUID::Generate();
    gx::GUID b = a;
    EXPECT_EQ(a, b);
    EXPECT_FALSE(a != b);

    gx::GUID c = gx::GUID::Generate();
    EXPECT_NE(a, c);
}

TEST(GUID, LessThanOperator)
{
    gx::GUID a{1, 2};
    gx::GUID b{1, 3};
    gx::GUID c{2, 0};
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(b < c);
    EXPECT_FALSE(c < a);
}

TEST(GUID, HashConsistency)
{
    gx::GUID g = gx::GUID::Generate();
    gx::container::Hash<gx::GUID> hasher;
    size_t h1 = hasher(g);
    size_t h2 = hasher(g);
    EXPECT_EQ(h1, h2);
}

TEST(GUID, HashDifferentValues)
{
    gx::container::Hash<gx::GUID> hasher;
    gx::HashSet<size_t> hashes;
    constexpr int count = 100;
    for (int i = 0; i < count; ++i)
    {
        gx::GUID g = gx::GUID::Generate();
        hashes.insert(hasher(g));
    }
    // ハッシュ衝突は許容するが、少なくとも90%は異なるべき
    EXPECT_GT(static_cast<int>(hashes.size()), count * 9 / 10);
}

TEST(GUID, UsableInHashMap)
{
    gx::HashMap<gx::GUID, int> map;
    gx::GUID g1 = gx::GUID::Generate();
    gx::GUID g2 = gx::GUID::Generate();
    map[g1] = 42;
    map[g2] = 99;
    EXPECT_EQ(map[g1], 42);
    EXPECT_EQ(map[g2], 99);
    EXPECT_EQ(map.size(), 2u);
}

// ============================================================================
// AssetMeta テスト
// ============================================================================

class AssetMetaTest : public ::testing::Test
{
protected:
    gx::String m_tempDir;

    void SetUp() override
    {
        char tmpBuf[MAX_PATH];
        GetTempPathA(MAX_PATH, tmpBuf);
        m_tempDir = gx::String(tmpBuf) + "gxlib_meta_test\\";
        fs::create_directories(fs::path(m_tempDir.c_str()));
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(fs::path(m_tempDir.c_str()), ec);
    }
};

TEST_F(AssetMetaTest, GetMetaPath)
{
    EXPECT_EQ(gx::AssetMeta::GetMetaPath("model.gxmdl"), gx::String("model.gxmdl.meta"));
    EXPECT_EQ(gx::AssetMeta::GetMetaPath("dir/texture.png"), gx::String("dir/texture.png.meta"));
}

TEST_F(AssetMetaTest, SaveLoadRoundtrip)
{
    gx::String metaPath = m_tempDir + "test.gxmdl.meta";
    gx::AssetMetaData data;
    data.guid = gx::GUID::Generate();
    data.importSettings["scale"] = "0.01";
    data.importSettings["compress"] = "true";

    EXPECT_TRUE(gx::AssetMeta::Save(metaPath, data));

    gx::AssetMetaData loaded;
    EXPECT_TRUE(gx::AssetMeta::Load(metaPath, loaded));
    EXPECT_EQ(loaded.guid, data.guid);
    EXPECT_EQ(loaded.importSettings.size(), 2u);
    EXPECT_EQ(loaded.importSettings["scale"], gx::String("0.01"));
    EXPECT_EQ(loaded.importSettings["compress"], gx::String("true"));
}

TEST_F(AssetMetaTest, LoadNonExistent)
{
    gx::AssetMetaData data;
    EXPECT_FALSE(gx::AssetMeta::Load(m_tempDir + "missing.meta", data));
}

TEST_F(AssetMetaTest, SaveEmptyImportSettings)
{
    gx::String metaPath = m_tempDir + "empty.meta";
    gx::AssetMetaData data;
    data.guid = gx::GUID::Generate();
    EXPECT_TRUE(gx::AssetMeta::Save(metaPath, data));

    gx::AssetMetaData loaded;
    EXPECT_TRUE(gx::AssetMeta::Load(metaPath, loaded));
    EXPECT_EQ(loaded.guid, data.guid);
    EXPECT_TRUE(loaded.importSettings.empty());
}

TEST_F(AssetMetaTest, HasMeta)
{
    gx::String assetPath = m_tempDir + "model.gxmdl";
    // ファイルなし
    EXPECT_FALSE(gx::AssetMeta::HasMeta(assetPath));

    // .meta ファイルを作成
    gx::String metaPath = gx::AssetMeta::GetMetaPath(assetPath);
    gx::AssetMetaData data;
    data.guid = gx::GUID::Generate();
    gx::AssetMeta::Save(metaPath, data);
    EXPECT_TRUE(gx::AssetMeta::HasMeta(assetPath));
}

// ============================================================================
// PPtr テスト
// ============================================================================

TEST(PPtr, DefaultIsInvalid)
{
    gx::PPtr<int> pp;
    EXPECT_FALSE(pp.IsValid());
    EXPECT_FALSE(static_cast<bool>(pp));
}

TEST(PPtr, ValidConstruction)
{
    gx::PPtr<int> pp;
    pp.guid = gx::GUID::Generate();
    pp.lfid = 5;
    EXPECT_TRUE(pp.IsValid());
    EXPECT_EQ(pp.lfid, 5u);
}

TEST(PPtr, EqualityComparison)
{
    gx::GUID g = gx::GUID::Generate();
    gx::PPtr<int> a;
    a.guid = g;
    a.lfid = 1;

    gx::PPtr<int> b;
    b.guid = g;
    b.lfid = 1;

    gx::PPtr<int> c;
    c.guid = g;
    c.lfid = 2;

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(PPtr, StringRoundtrip)
{
    gx::PPtr<int> original;
    original.guid = gx::GUID::Generate();
    original.lfid = 42;

    gx::String str = original.ToString();
    gx::PPtr<int> parsed = gx::PPtr<int>::FromString(str);
    EXPECT_EQ(original, parsed);
}

TEST(PPtr, ToStringFormat)
{
    gx::PPtr<float> pp;
    pp.guid = gx::GUID::Generate();
    pp.lfid = 0;
    gx::String s = pp.ToString();
    // Format: guid:lfid
    auto colonPos = s.rfind(':');
    EXPECT_NE(colonPos, gx::String::npos);
    EXPECT_EQ(s.substr(colonPos), gx::String(":0"));
}

TEST(PPtr, FromStringInvalid)
{
    gx::PPtr<int> pp = gx::PPtr<int>::FromString("");
    EXPECT_FALSE(pp.IsValid());

    gx::PPtr<int> pp2 = gx::PPtr<int>::FromString("nocolon");
    EXPECT_FALSE(pp2.IsValid());
}

// ============================================================================
// AssetDatabase GUID統合テスト
// ============================================================================

class AssetDatabaseGUIDTest : public ::testing::Test
{
protected:
    gx::String m_tempDir;

    void SetUp() override
    {
        char tmpBuf[MAX_PATH];
        GetTempPathA(MAX_PATH, tmpBuf);
        m_tempDir = gx::String(tmpBuf) + "gxlib_assetdb_test\\";
        std::error_code ec;
        fs::remove_all(fs::path(m_tempDir.c_str()), ec);
        fs::create_directories(fs::path(m_tempDir.c_str()));

        // テスト用アセットファイルを作成
        CreateTestFile("model.gxmdl", "model data");
        CreateTestFile("texture.png", "png data");
        CreateTestFile("script.lua", "lua data");

        gx::AssetDatabase::Instance().Shutdown();
    }

    void TearDown() override
    {
        gx::AssetDatabase::Instance().Shutdown();
        std::error_code ec;
        fs::remove_all(fs::path(m_tempDir.c_str()), ec);
    }

    void CreateTestFile(const gx::String& name, const gx::String& content)
    {
        gx::String path = m_tempDir + name;
        std::ofstream f(path.c_str());
        f << content;
    }
};

TEST_F(AssetDatabaseGUIDTest, ScanGeneratesMeta)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);

    // .meta ファイルが自動生成されたか確認
    EXPECT_TRUE(fs::exists(fs::path((m_tempDir + "model.gxmdl.meta").c_str())));
    EXPECT_TRUE(fs::exists(fs::path((m_tempDir + "texture.png.meta").c_str())));
    EXPECT_TRUE(fs::exists(fs::path((m_tempDir + "script.lua.meta").c_str())));
}

TEST_F(AssetDatabaseGUIDTest, ScanAssignsGUIDs)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);

    auto* model = gx::AssetDatabase::Instance().FindAsset("model.gxmdl");
    auto* texture = gx::AssetDatabase::Instance().FindAsset("texture.png");

    ASSERT_NE(model, nullptr);
    ASSERT_NE(texture, nullptr);
    EXPECT_TRUE(model->guid.IsValid());
    EXPECT_TRUE(texture->guid.IsValid());
    EXPECT_NE(model->guid, texture->guid);
}

TEST_F(AssetDatabaseGUIDTest, RescanPreservesGUIDs)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);
    auto* model1 = gx::AssetDatabase::Instance().FindAsset("model.gxmdl");
    ASSERT_NE(model1, nullptr);
    gx::GUID firstGuid = model1->guid;

    // 再スキャン
    gx::AssetDatabase::Instance().ScanAll();
    auto* model2 = gx::AssetDatabase::Instance().FindAsset("model.gxmdl");
    ASSERT_NE(model2, nullptr);
    EXPECT_EQ(model2->guid, firstGuid);
}

TEST_F(AssetDatabaseGUIDTest, FindByGUID)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);

    auto* model = gx::AssetDatabase::Instance().FindAsset("model.gxmdl");
    ASSERT_NE(model, nullptr);
    gx::GUID guid = model->guid;

    const gx::AssetEntry* found = gx::AssetDatabase::Instance().FindByGUID(guid);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, gx::String("model.gxmdl"));
}

TEST_F(AssetDatabaseGUIDTest, FindByGUIDNotFound)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);
    gx::GUID fake = gx::GUID::Generate();
    EXPECT_EQ(gx::AssetDatabase::Instance().FindByGUID(fake), nullptr);
}

TEST_F(AssetDatabaseGUIDTest, GetOrCreateGUIDIdempotent)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);

    gx::GUID g1 = gx::AssetDatabase::Instance().GetOrCreateGUID("model.gxmdl");
    gx::GUID g2 = gx::AssetDatabase::Instance().GetOrCreateGUID("model.gxmdl");
    EXPECT_TRUE(g1.IsValid());
    EXPECT_EQ(g1, g2);
}

TEST_F(AssetDatabaseGUIDTest, OnAssetMovedGUIDMaintained)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);

    auto* model = gx::AssetDatabase::Instance().FindAsset("model.gxmdl");
    ASSERT_NE(model, nullptr);
    gx::GUID originalGuid = model->guid;

    // 仮想的に移動
    bool moved = gx::AssetDatabase::Instance().OnAssetMoved("model.gxmdl", "renamed.gxmdl");
    EXPECT_TRUE(moved);

    // 旧パスでは見つからない
    EXPECT_EQ(gx::AssetDatabase::Instance().FindAsset("model.gxmdl"), nullptr);

    // GUIDで見つかる（GUIDは維持）
    const gx::AssetEntry* found = gx::AssetDatabase::Instance().FindByGUID(originalGuid);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->relativePath, gx::String("renamed.gxmdl"));
}

TEST_F(AssetDatabaseGUIDTest, OnAssetMovedNonExistent)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);
    EXPECT_FALSE(gx::AssetDatabase::Instance().OnAssetMoved("nonexistent.fbx", "new.fbx"));
}

TEST_F(AssetDatabaseGUIDTest, GUIDMapPopulated)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);
    const auto& guidMap = gx::AssetDatabase::Instance().GetGUIDMap();
    EXPECT_GE(guidMap.size(), 3u); // model, texture, script
}

TEST_F(AssetDatabaseGUIDTest, MetaFilesNotInEntries)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);

    // .meta ファイルはアセットとして登録されていないこと
    for (const auto& [key, entry] : gx::AssetDatabase::Instance().GetAllAssets())
    {
        EXPECT_NE(entry.extension, gx::String(".meta"))
            << "Meta file found in entries: " << entry.name;
    }
}

TEST_F(AssetDatabaseGUIDTest, ShutdownClearsGUIDMap)
{
    gx::AssetDatabase::Instance().Initialize(m_tempDir);
    EXPECT_FALSE(gx::AssetDatabase::Instance().GetGUIDMap().empty());
    gx::AssetDatabase::Instance().Shutdown();
    EXPECT_TRUE(gx::AssetDatabase::Instance().GetGUIDMap().empty());
}
