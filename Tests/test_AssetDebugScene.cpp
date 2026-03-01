/// @file test_AssetDebugScene.cpp
/// @brief Phase 3: AssetDatabase, DebugDraw3D, AudioOcclusion, ScenePersistence, AtlasPacker テスト

#include <gtest/gtest.h>
#include "Core/AssetDatabase.h"
#include "Graphics/3D/DebugDraw3D.h"
#include "Audio/AudioOcclusion.h"
#include "Core/Scene/ScenePersistence.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include "Graphics/AtlasPacker.h"
#include <filesystem>
#include <fstream>

using namespace gx;
using namespace DirectX;

// ============================================================================
// AssetDatabase
// ============================================================================

class AssetDatabaseTest : public ::testing::Test
{
protected:
    std::string testDir;

    void SetUp() override
    {
        AssetDatabase::Instance().Clear();
        testDir = (std::filesystem::temp_directory_path() / "gx_asset_test").string();
        std::filesystem::create_directories(testDir);

        // テストファイルを作成
        CreateFile("texture.png");
        CreateFile("model.fbx");
        CreateFile("sound.wav");
        CreateFile("music.ogg");
        CreateFile("font.ttf");
        CreateFile("shader.hlsl");
        CreateFile("script.lua");
        CreateFile("readme.txt");
        std::filesystem::create_directories(testDir + "/sub");
        CreateFile("sub/nested.png");
    }

    void TearDown() override
    {
        AssetDatabase::Instance().Clear();
        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
    }

    void CreateFile(const std::string& name)
    {
        std::ofstream f(testDir + "/" + name);
        f << "test data for " << name;
    }
};

TEST_F(AssetDatabaseTest, ScanDirectory)
{
    EXPECT_TRUE(AssetDatabase::Instance().Scan(testDir));
    EXPECT_GE(AssetDatabase::Instance().GetTotalCount(), 8u);
}

TEST_F(AssetDatabaseTest, ScanNonExistent)
{
    EXPECT_FALSE(AssetDatabase::Instance().Scan("/nonexistent/path"));
}

TEST_F(AssetDatabaseTest, FindByPath)
{
    AssetDatabase::Instance().Scan(testDir);

    // 既知のファイルを検索
    auto results = AssetDatabase::Instance().Search("texture.png");
    EXPECT_GE(results.size(), 1u);
    EXPECT_EQ(results[0]->type, AssetType::Texture);
}

TEST_F(AssetDatabaseTest, FindByType)
{
    AssetDatabase::Instance().Scan(testDir);

    auto textures = AssetDatabase::Instance().FindByType(AssetType::Texture);
    EXPECT_GE(textures.size(), 2u); // texture.png + sub/nested.pngの2つ

    auto models = AssetDatabase::Instance().FindByType(AssetType::Model);
    EXPECT_EQ(models.size(), 1u);

    auto sounds = AssetDatabase::Instance().FindByType(AssetType::Sound);
    EXPECT_EQ(sounds.size(), 1u);
}

TEST_F(AssetDatabaseTest, Search)
{
    AssetDatabase::Instance().Scan(testDir);

    auto results = AssetDatabase::Instance().Search(".png");
    EXPECT_GE(results.size(), 2u);

    auto results2 = AssetDatabase::Instance().Search("nonexistent_xyz");
    EXPECT_EQ(results2.size(), 0u);
}

TEST_F(AssetDatabaseTest, SearchCaseInsensitive)
{
    AssetDatabase::Instance().Scan(testDir);

    auto results = AssetDatabase::Instance().Search("TEXTURE");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AssetDatabaseTest, Refresh)
{
    AssetDatabase::Instance().Scan(testDir);
    size_t count1 = AssetDatabase::Instance().GetTotalCount();

    CreateFile("new_file.wav");
    EXPECT_TRUE(AssetDatabase::Instance().Refresh());
    EXPECT_GT(AssetDatabase::Instance().GetTotalCount(), count1);
}

TEST_F(AssetDatabaseTest, Clear)
{
    AssetDatabase::Instance().Scan(testDir);
    EXPECT_GT(AssetDatabase::Instance().GetTotalCount(), 0u);

    AssetDatabase::Instance().Clear();
    EXPECT_EQ(AssetDatabase::Instance().GetTotalCount(), 0u);
}

TEST_F(AssetDatabaseTest, AssetTypeClassification)
{
    AssetDatabase::Instance().Scan(testDir);

    auto music = AssetDatabase::Instance().FindByType(AssetType::Music);
    EXPECT_EQ(music.size(), 1u);

    auto fonts = AssetDatabase::Instance().FindByType(AssetType::Font);
    EXPECT_EQ(fonts.size(), 1u);

    auto shaders = AssetDatabase::Instance().FindByType(AssetType::Shader);
    EXPECT_EQ(shaders.size(), 1u);

    auto scripts = AssetDatabase::Instance().FindByType(AssetType::Script);
    EXPECT_EQ(scripts.size(), 1u);

    auto unknown = AssetDatabase::Instance().FindByType(AssetType::Unknown);
    EXPECT_GE(unknown.size(), 1u); // readme.txtが該当
}

TEST_F(AssetDatabaseTest, NonRecursiveScan)
{
    EXPECT_TRUE(AssetDatabase::Instance().Scan(testDir, false));
    auto textures = AssetDatabase::Instance().FindByType(AssetType::Texture);
    EXPECT_EQ(textures.size(), 1u); // texture.pngのみ、sub/nested.pngは含まない
}

// ============================================================================
// DebugDraw3D
// ============================================================================

TEST(DebugDraw3DTest, DrawLine)
{
    auto& dd = DebugDraw3D::Instance();
    dd.SetEnabled(true);
    dd.Clear();

    dd.DrawLine({ 0,0,0 }, { 1,1,1 }, Color{ 1.0f,0.0f,0.0f,1.0f });
    EXPECT_EQ(dd.GetLineCount(), 1u);

    dd.Clear();
    EXPECT_EQ(dd.GetLineCount(), 0u);
}

TEST(DebugDraw3DTest, DrawWireBox)
{
    auto& dd = DebugDraw3D::Instance();
    dd.SetEnabled(true);
    dd.Clear();

    dd.DrawWireBox({ 0,0,0 }, { 1,1,1 }, Color{ 0.0f,1.0f,0.0f,1.0f });
    EXPECT_EQ(dd.GetLineCount(), 12u); // ボックスの12辺
}

TEST(DebugDraw3DTest, DrawWireSphere)
{
    auto& dd = DebugDraw3D::Instance();
    dd.SetEnabled(true);
    dd.Clear();

    dd.DrawWireSphere({ 0,0,0 }, 1.0f, Color{ 0.0f,0.0f,1.0f,1.0f }, 8);
    EXPECT_EQ(dd.GetLineCount(), 8u * 3u); // 8セグメントのリング3本
}

TEST(DebugDraw3DTest, DrawArrow)
{
    auto& dd = DebugDraw3D::Instance();
    dd.SetEnabled(true);
    dd.Clear();

    dd.DrawArrow({ 0,0,0 }, { 0,5,0 }, Color{ 1.0f,1.0f,0.0f,1.0f });
    EXPECT_EQ(dd.GetLineCount(), 3u); // 軸線 + 矢じり2線
}

TEST(DebugDraw3DTest, PersistentLine)
{
    auto& dd = DebugDraw3D::Instance();
    dd.SetEnabled(true);
    dd.Clear();

    dd.DrawLinePersistent({ 0,0,0 }, { 1,0,0 }, Color{ 1.0f,1.0f,1.0f,1.0f }, 1.0f);
    EXPECT_EQ(dd.GetPersistentLineCount(), 1u);
}

TEST(DebugDraw3DTest, PersistentBox)
{
    auto& dd = DebugDraw3D::Instance();
    dd.SetEnabled(true);
    dd.Clear();

    dd.DrawWireBoxPersistent({ 0,0,0 }, { 1,1,1 }, Color{ 1.0f,1.0f,1.0f,1.0f }, 2.0f);
    EXPECT_GE(dd.GetPersistentLineCount(), 12u); // 12 edges minimum
}

TEST(DebugDraw3DTest, DisabledDraws)
{
    auto& dd = DebugDraw3D::Instance();
    dd.Clear();
    dd.SetEnabled(false);

    dd.DrawLine({ 0,0,0 }, { 1,1,1 });
    EXPECT_EQ(dd.GetLineCount(), 0u); // 無効時は描画されない

    dd.SetEnabled(true);
}

// ============================================================================
// AudioOcclusion
// ============================================================================

TEST(AudioOcclusionTest, DefaultNoOcclusion)
{
    AudioOcclusion occ;
    auto params = occ.Calculate({ 10, 0, 0 });

    EXPECT_FLOAT_EQ(params.directLevel, 1.0f);
    EXPECT_FLOAT_EQ(params.reverbLevel, 1.0f);
    EXPECT_FLOAT_EQ(params.lowPassCutoff, 1.0f);
}

TEST(AudioOcclusionTest, FullOcclusion)
{
    AudioOcclusion occ;
    occ.SetListenerPosition({ 0, 0, 0 });
    occ.SetOcclusionCallback([](const XMFLOAT3&, const XMFLOAT3&) -> float {
        return 1.0f; // 完全遮蔽
    });

    auto params = occ.Calculate({ 10, 0, 0 });
    EXPECT_FLOAT_EQ(params.directLevel, 0.2f);   // 1.0 - 1.0*0.8
    EXPECT_FLOAT_EQ(params.reverbLevel, 0.5f);   // 1.0 - 1.0*0.5
    EXPECT_FLOAT_EQ(params.lowPassCutoff, 0.3f); // 1.0 - 1.0*0.7
}

TEST(AudioOcclusionTest, HalfOcclusion)
{
    AudioOcclusion occ;
    occ.SetListenerPosition({ 0, 0, 0 });
    occ.SetOcclusionCallback([](const XMFLOAT3&, const XMFLOAT3&) -> float {
        return 0.5f;
    });

    auto params = occ.Calculate({ 5, 0, 0 });
    EXPECT_FLOAT_EQ(params.directLevel, 0.6f);   // 1.0 - 0.5*0.8
    EXPECT_FLOAT_EQ(params.lowPassCutoff, 0.65f); // 1.0 - 0.5*0.7
}

TEST(AudioOcclusionTest, Disabled)
{
    AudioOcclusion occ;
    occ.SetEnabled(false);
    occ.SetOcclusionCallback([](const XMFLOAT3&, const XMFLOAT3&) -> float {
        return 1.0f;
    });

    auto params = occ.Calculate({ 10, 0, 0 });
    EXPECT_FLOAT_EQ(params.directLevel, 1.0f); // 無効 = 遮蔽なし
}

TEST(AudioOcclusionTest, OcclusionClamped)
{
    AudioOcclusion occ;
    occ.SetOcclusionCallback([](const XMFLOAT3&, const XMFLOAT3&) -> float {
        return 5.0f; // 1.0を超過
    });

    auto params = occ.Calculate({ 1, 0, 0 });
    EXPECT_LE(params.directLevel, 1.0f);
    EXPECT_GE(params.directLevel, 0.0f);
}

// ============================================================================
// ScenePersistence
// ============================================================================

TEST(ScenePersistenceTest, SerializeEmptyScene)
{
    Scene scene("TestScene");

    std::string text = ScenePersistence::SerializeToString(scene);
    EXPECT_FALSE(text.empty());
    EXPECT_NE(text.find("GXSCENE 1"), std::string::npos);
    EXPECT_NE(text.find("scene_name: TestScene"), std::string::npos);
    EXPECT_NE(text.find("entity_count: 0"), std::string::npos);
}

TEST(ScenePersistenceTest, SerializeWithEntities)
{
    Scene scene("Test");
    scene.CreateEntity("Player");
    scene.CreateEntity("Enemy");

    std::string text = ScenePersistence::SerializeToString(scene);
    EXPECT_NE(text.find("Player"), std::string::npos);
    EXPECT_NE(text.find("Enemy"), std::string::npos);
    EXPECT_NE(text.find("entity_count: 2"), std::string::npos);
}

TEST(ScenePersistenceTest, TextRoundTrip)
{
    Scene scene("RoundTrip");
    auto* player = scene.CreateEntity("Player");
    player->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
    player->GetTransform().SetRotation(0.1f, 0.2f, 0.3f);
    player->GetTransform().SetScale(1.5f, 2.0f, 0.5f);
    auto* tag = player->AddComponent<TagComponent>();
    tag->tag = "hero";

    auto* enemy = scene.CreateEntity("Enemy");
    enemy->GetTransform().SetPosition(10.0f, 0.0f, -5.0f);

    std::string text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetName(), "RoundTrip");
    EXPECT_EQ(loaded->GetEntityCount(), 2u);

    auto* e0 = loaded->FindEntity("Player");
    ASSERT_NE(e0, nullptr);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetPosition().x, 1.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetPosition().y, 2.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetPosition().z, 3.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetRotation().x, 0.1f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetRotation().y, 0.2f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetRotation().z, 0.3f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetScale().x, 1.5f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetScale().y, 2.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetScale().z, 0.5f);

    // TagComponent の検証（コンポーネント走査）
    const TagComponent* tagResult = nullptr;
    for (const auto& comp : e0->GetComponents())
    {
        if (auto* t = dynamic_cast<const TagComponent*>(comp.get()))
        {
            tagResult = t;
            break;
        }
    }
    ASSERT_NE(tagResult, nullptr);
    EXPECT_EQ(tagResult->tag, "hero");

    auto* e1 = loaded->FindEntity("Enemy");
    ASSERT_NE(e1, nullptr);
    EXPECT_FLOAT_EQ(e1->GetTransform().GetPosition().x, 10.0f);
}

TEST(ScenePersistenceTest, TextRoundTripWithHierarchy)
{
    Scene scene("HierarchyTest");
    auto* parent = scene.CreateEntity("Parent");
    auto* child = scene.CreateEntity("Child");
    child->SetParent(parent);

    std::string text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetEntityCount(), 2u);

    auto* loadedChild = loaded->FindEntity("Child");
    ASSERT_NE(loadedChild, nullptr);
    ASSERT_NE(loadedChild->GetParent(), nullptr);
    EXPECT_EQ(loadedChild->GetParent()->GetName(), "Parent");
}

TEST(ScenePersistenceTest, BinaryRoundTrip)
{
    Scene scene("BinaryTest");
    auto* player = scene.CreateEntity("Player");
    player->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
    player->GetTransform().SetRotation(0.1f, 0.2f, 0.3f);
    player->GetTransform().SetScale(1.5f, 2.0f, 0.5f);
    auto* tag = player->AddComponent<TagComponent>();
    tag->tag = "hero";

    auto* enemy = scene.CreateEntity("Enemy");
    enemy->GetTransform().SetPosition(10.0f, 0.0f, -5.0f);
    enemy->SetActive(false);

    auto bin = ScenePersistence::SerializeToBinary(scene);
    EXPECT_GT(bin.size(), 8u); // 少なくともヘッダー分

    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetName(), "BinaryTest");
    EXPECT_EQ(loaded->GetEntityCount(), 2u);

    auto* e0 = loaded->FindEntity("Player");
    ASSERT_NE(e0, nullptr);
    EXPECT_TRUE(e0->IsActive());
    EXPECT_FLOAT_EQ(e0->GetTransform().GetPosition().x, 1.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetPosition().y, 2.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetPosition().z, 3.0f);
    EXPECT_FLOAT_EQ(e0->GetTransform().GetScale().x, 1.5f);

    auto* e1 = loaded->FindEntity("Enemy");
    ASSERT_NE(e1, nullptr);
    EXPECT_FALSE(e1->IsActive());
    EXPECT_FLOAT_EQ(e1->GetTransform().GetPosition().x, 10.0f);
}

TEST(ScenePersistenceTest, BinaryRoundTripWithHierarchy)
{
    Scene scene("BinHierarchy");
    auto* parent = scene.CreateEntity("Parent");
    auto* child = scene.CreateEntity("Child");
    child->SetParent(parent);

    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    auto* loadedChild = loaded->FindEntity("Child");
    ASSERT_NE(loadedChild, nullptr);
    ASSERT_NE(loadedChild->GetParent(), nullptr);
    EXPECT_EQ(loadedChild->GetParent()->GetName(), "Parent");
}

TEST(ScenePersistenceTest, SaveAndLoadTextFile)
{
    auto tempPath = (std::filesystem::temp_directory_path() / "gx_scene_test.gxscene").string();

    Scene scene("SaveTest");
    auto* entity = scene.CreateEntity("TestEntity");
    entity->GetTransform().SetPosition(5.0f, 10.0f, 15.0f);

    EXPECT_TRUE(ScenePersistence::SaveToFile(scene, tempPath, SceneFileFormat::Text));

    auto loaded = ScenePersistence::LoadFromFile(tempPath, SceneFileFormat::Text);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "SaveTest");
    EXPECT_EQ(loaded->GetEntityCount(), 1u);

    auto* e = loaded->FindEntity("TestEntity");
    ASSERT_NE(e, nullptr);
    EXPECT_FLOAT_EQ(e->GetTransform().GetPosition().x, 5.0f);

    std::filesystem::remove(tempPath);
}

TEST(ScenePersistenceTest, SaveAndLoadBinaryFile)
{
    auto tempPath = (std::filesystem::temp_directory_path() / "gx_scene_test.gxscbin").string();

    Scene scene("BinSaveTest");
    auto* entity = scene.CreateEntity("TestEntity");
    entity->GetTransform().SetPosition(5.0f, 10.0f, 15.0f);

    EXPECT_TRUE(ScenePersistence::SaveToFile(scene, tempPath, SceneFileFormat::Binary));

    auto loaded = ScenePersistence::LoadFromFile(tempPath, SceneFileFormat::Binary);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "BinSaveTest");
    EXPECT_EQ(loaded->GetEntityCount(), 1u);

    std::filesystem::remove(tempPath);
}

TEST(ScenePersistenceTest, LoadNonExistentText)
{
    auto loaded = ScenePersistence::LoadFromFile("/nonexistent/path.gxscene", SceneFileFormat::Text);
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistenceTest, LoadNonExistentBinary)
{
    auto loaded = ScenePersistence::LoadFromFile("/nonexistent/path.gxscbin", SceneFileFormat::Binary);
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistenceTest, DeserializeInvalidText)
{
    auto loaded = ScenePersistence::DeserializeFromString("garbage data");
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistenceTest, DeserializeInvalidBinary)
{
    std::vector<uint8_t> garbage = { 0, 1, 2, 3, 4, 5, 6, 7 };
    auto loaded = ScenePersistence::DeserializeFromBinary(garbage);
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistenceTest, DeserializeEmptyBinary)
{
    std::vector<uint8_t> empty;
    auto loaded = ScenePersistence::DeserializeFromBinary(empty);
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistenceTest, EmptySceneTextRoundTrip)
{
    Scene scene("Empty");
    std::string text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "Empty");
    EXPECT_EQ(loaded->GetEntityCount(), 0u);
}

TEST(ScenePersistenceTest, EmptySceneBinaryRoundTrip)
{
    Scene scene("Empty");
    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "Empty");
    EXPECT_EQ(loaded->GetEntityCount(), 0u);
}

// ============================================================================
// AtlasPacker
// ============================================================================

TEST(AtlasPackerTest, EmptyInput)
{
    AtlasPacker packer;
    auto result = packer.Pack({});
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries.size(), 0u);
}

TEST(AtlasPackerTest, SingleRect)
{
    AtlasPacker packer;
    std::vector<AtlasInput> inputs = { { "A", 64, 64 } };
    auto result = packer.Pack(inputs);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries.size(), 1u);
    EXPECT_EQ(result.entries[0].name, "A");
    EXPECT_EQ(result.entries[0].width, 64u);
    EXPECT_EQ(result.entries[0].height, 64u);
    // アトラスサイズは2の累乗であるべき
    EXPECT_EQ(result.atlasWidth & (result.atlasWidth - 1), 0u);
}

TEST(AtlasPackerTest, MultipleRects)
{
    AtlasPacker packer;
    std::vector<AtlasInput> inputs = {
        { "A", 32, 32 },
        { "B", 64, 64 },
        { "C", 16, 48 },
        { "D", 128, 32 },
    };
    auto result = packer.Pack(inputs);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries.size(), 4u);
}

TEST(AtlasPackerTest, WithPadding)
{
    AtlasPacker packer;
    packer.SetPadding(2);

    std::vector<AtlasInput> inputs = {
        { "A", 30, 30 },
        { "B", 30, 30 },
    };
    auto result = packer.Pack(inputs);
    EXPECT_TRUE(result.success);

    // エントリが重ならないことを検証（パディング考慮）
    if (result.entries.size() == 2)
    {
        const auto& a = result.entries[0];
        const auto& b = result.entries[1];
        // aがbの左、bがaの左、などの少なくとも1つ
        bool separated = (a.x + a.width + 4 <= b.x) || (b.x + b.width + 4 <= a.x) ||
                         (a.y + a.height + 4 <= b.y) || (b.y + b.height + 4 <= a.y);
        EXPECT_TRUE(separated);
    }
}

TEST(AtlasPackerTest, TooLargeForMaxSize)
{
    AtlasPacker packer;
    packer.SetMaxSize(32, 32);

    std::vector<AtlasInput> inputs = {
        { "A", 64, 64 }, // 最大アトラスサイズより大きい
    };
    auto result = packer.Pack(inputs);
    EXPECT_FALSE(result.success);
}

TEST(AtlasPackerTest, PowerOf2Output)
{
    AtlasPacker packer;
    std::vector<AtlasInput> inputs = {
        { "A", 100, 100 },
    };
    auto result = packer.Pack(inputs);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.atlasWidth & (result.atlasWidth - 1), 0u);
    EXPECT_EQ(result.atlasHeight & (result.atlasHeight - 1), 0u);
    EXPECT_GE(result.atlasWidth, 100u);
    EXPECT_GE(result.atlasHeight, 100u);
}

TEST(AtlasPackerTest, ManySmallRects)
{
    AtlasPacker packer;
    packer.SetMaxSize(1024, 1024);

    std::vector<AtlasInput> inputs;
    for (int i = 0; i < 50; ++i)
    {
        inputs.push_back({ "R" + std::to_string(i), 16, 16 });
    }

    auto result = packer.Pack(inputs);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries.size(), 50u);
}
