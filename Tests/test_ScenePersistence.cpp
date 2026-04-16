/// @file test_ScenePersistence.cpp
/// @brief Phase 5: ScenePersistence専用テスト（テキスト/バイナリラウンドトリップ、ファイルI/O）

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/ScenePersistence.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/Components.h"
#include <filesystem>

using namespace gx;

// ============================================================================
// SceneFileFormat列挙型
// ============================================================================

TEST(ScenePersistencePhase5, FileFormatEnum)
{
    EXPECT_NE(static_cast<int>(SceneFileFormat::Text), static_cast<int>(SceneFileFormat::Binary));
}

// ============================================================================
// テキストシリアライゼーション
// ============================================================================

TEST(ScenePersistencePhase5, SerializeEmptySceneToString)
{
    Scene scene("EmptyTest");
    gx::String text = ScenePersistence::SerializeToString(scene);

    EXPECT_FALSE(text.empty());
    EXPECT_NE(text.find("EmptyTest"), gx::String::npos);
}

TEST(ScenePersistencePhase5, DeserializeEmptySceneFromString)
{
    Scene scene("RoundTrip");
    gx::String text = ScenePersistence::SerializeToString(scene);

    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "RoundTrip");
    EXPECT_EQ(loaded->GetEntityCount(), 0u);
}

TEST(ScenePersistencePhase5, TextRoundTripSingleEntity)
{
    Scene scene("SingleEntity");
    auto* entity = scene.CreateEntity("Hero");
    entity->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
    entity->GetTransform().SetScale(2.0f, 2.0f, 2.0f);

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 1u);
    auto* e = loaded->FindEntity("Hero");
    ASSERT_NE(e, nullptr);
    EXPECT_NEAR(e->GetTransform().GetPosition().x, 1.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetPosition().y, 2.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetPosition().z, 3.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetScale().x, 2.0f, 0.001f);
}

TEST(ScenePersistencePhase5, TextRoundTripMultipleEntities)
{
    Scene scene("MultiEntity");
    scene.CreateEntity("Player");
    scene.CreateEntity("Enemy");
    scene.CreateEntity("NPC");

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 3u);
    EXPECT_NE(loaded->FindEntity("Player"), nullptr);
    EXPECT_NE(loaded->FindEntity("Enemy"), nullptr);
    EXPECT_NE(loaded->FindEntity("NPC"), nullptr);
}

TEST(ScenePersistencePhase5, TextRoundTripWithHierarchy)
{
    Scene scene("Hierarchy");
    auto* parent = scene.CreateEntity("Parent");
    auto* child = scene.CreateEntity("Child");
    child->SetParent(parent);

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 2u);
    auto* loadedChild = loaded->FindEntity("Child");
    ASSERT_NE(loadedChild, nullptr);
    ASSERT_NE(loadedChild->GetParent(), nullptr);
    EXPECT_EQ(loadedChild->GetParent()->GetName(), "Parent");
}

TEST(ScenePersistencePhase5, TextRoundTripWithRotation)
{
    Scene scene("RotationTest");
    auto* entity = scene.CreateEntity("Rotated");
    entity->GetTransform().SetRotation(0.5f, 1.0f, 1.5f);

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    auto* e = loaded->FindEntity("Rotated");
    ASSERT_NE(e, nullptr);
    EXPECT_NEAR(e->GetTransform().GetRotation().x, 0.5f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetRotation().y, 1.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetRotation().z, 1.5f, 0.001f);
}

TEST(ScenePersistencePhase5, DeserializeInvalidText)
{
    auto loaded = ScenePersistence::DeserializeFromString("not a valid scene format");
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistencePhase5, DeserializeEmptyString)
{
    auto loaded = ScenePersistence::DeserializeFromString("");
    EXPECT_EQ(loaded, nullptr);
}

// ============================================================================
// バイナリシリアライゼーション
// ============================================================================

TEST(ScenePersistencePhase5, BinarySerializeEmpty)
{
    Scene scene("BinEmpty");
    auto bin = ScenePersistence::SerializeToBinary(scene);
    EXPECT_GT(bin.size(), 0u);
}

TEST(ScenePersistencePhase5, BinaryRoundTripEmpty)
{
    Scene scene("BinRoundTrip");
    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "BinRoundTrip");
    EXPECT_EQ(loaded->GetEntityCount(), 0u);
}

TEST(ScenePersistencePhase5, BinaryRoundTripWithEntities)
{
    Scene scene("BinEntities");
    auto* e = scene.CreateEntity("Warrior");
    e->GetTransform().SetPosition(10.0f, 20.0f, 30.0f);
    e->SetActive(false);

    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 1u);
    auto* warrior = loaded->FindEntity("Warrior");
    ASSERT_NE(warrior, nullptr);
    EXPECT_NEAR(warrior->GetTransform().GetPosition().x, 10.0f, 0.001f);
    EXPECT_FALSE(warrior->IsActive());
}

TEST(ScenePersistencePhase5, BinaryDeserializeInvalid)
{
    gx::Vector<uint8_t> garbage = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03 };
    auto loaded = ScenePersistence::DeserializeFromBinary(garbage);
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistencePhase5, BinaryDeserializeEmpty)
{
    gx::Vector<uint8_t> empty;
    auto loaded = ScenePersistence::DeserializeFromBinary(empty);
    EXPECT_EQ(loaded, nullptr);
}

// ============================================================================
// ファイルI/O
// ============================================================================

TEST(ScenePersistencePhase5, SaveAndLoadTextFile)
{
    auto tempPath = (std::filesystem::temp_directory_path() / "gx_p5_scene_text.gxscene").string();

    Scene scene("FileTest");
    auto* entity = scene.CreateEntity("FileEntity");
    entity->GetTransform().SetPosition(7.0f, 8.0f, 9.0f);

    EXPECT_TRUE(ScenePersistence::SaveToFile(scene, tempPath, SceneFileFormat::Text));

    auto loaded = ScenePersistence::LoadFromFile(tempPath, SceneFileFormat::Text);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "FileTest");
    EXPECT_EQ(loaded->GetEntityCount(), 1u);

    auto* e = loaded->FindEntity("FileEntity");
    ASSERT_NE(e, nullptr);
    EXPECT_NEAR(e->GetTransform().GetPosition().x, 7.0f, 0.001f);

    std::filesystem::remove(tempPath);
}

TEST(ScenePersistencePhase5, SaveAndLoadBinaryFile)
{
    auto tempPath = (std::filesystem::temp_directory_path() / "gx_p5_scene_bin.gxscbin").string();

    Scene scene("BinFileTest");
    scene.CreateEntity("Alpha");
    scene.CreateEntity("Beta");

    EXPECT_TRUE(ScenePersistence::SaveToFile(scene, tempPath, SceneFileFormat::Binary));

    auto loaded = ScenePersistence::LoadFromFile(tempPath, SceneFileFormat::Binary);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "BinFileTest");
    EXPECT_EQ(loaded->GetEntityCount(), 2u);

    std::filesystem::remove(tempPath);
}

TEST(ScenePersistencePhase5, LoadNonExistentTextFile)
{
    auto loaded = ScenePersistence::LoadFromFile("C:/nonexistent/path/scene.gxscene", SceneFileFormat::Text);
    EXPECT_EQ(loaded, nullptr);
}

TEST(ScenePersistencePhase5, LoadNonExistentBinaryFile)
{
    auto loaded = ScenePersistence::LoadFromFile("C:/nonexistent/path/scene.gxscbin", SceneFileFormat::Binary);
    EXPECT_EQ(loaded, nullptr);
}

// ============================================================================
// 追加テスト: テキストシリアライゼーション
// ============================================================================

TEST(ScenePersistencePhase5, TextRoundTripEntityActive)
{
    Scene scene("ActiveTest");
    auto* e1 = scene.CreateEntity("Active");
    e1->SetActive(true);
    auto* e2 = scene.CreateEntity("Inactive");
    e2->SetActive(false);

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    auto* la = loaded->FindEntity("Active");
    auto* li = loaded->FindEntity("Inactive");
    ASSERT_NE(la, nullptr);
    ASSERT_NE(li, nullptr);
    EXPECT_TRUE(la->IsActive());
    EXPECT_FALSE(li->IsActive());
}

TEST(ScenePersistencePhase5, TextRoundTripLargeScene)
{
    Scene scene("LargeScene");
    for (int i = 0; i < 50; ++i)
    {
        auto* e = scene.CreateEntity("Entity_" + std::to_string(i));
        e->GetTransform().SetPosition(
            static_cast<float>(i),
            static_cast<float>(i * 2),
            static_cast<float>(i * 3));
    }

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 50u);

    auto* e25 = loaded->FindEntity("Entity_25");
    ASSERT_NE(e25, nullptr);
    EXPECT_NEAR(e25->GetTransform().GetPosition().x, 25.0f, 0.001f);
    EXPECT_NEAR(e25->GetTransform().GetPosition().y, 50.0f, 0.001f);
    EXPECT_NEAR(e25->GetTransform().GetPosition().z, 75.0f, 0.001f);
}

TEST(ScenePersistencePhase5, TextRoundTripScale)
{
    Scene scene("ScaleTest");
    auto* entity = scene.CreateEntity("Scaled");
    entity->GetTransform().SetScale(3.0f, 4.0f, 5.0f);

    gx::String text = ScenePersistence::SerializeToString(scene);
    auto loaded = ScenePersistence::DeserializeFromString(text);
    ASSERT_NE(loaded, nullptr);

    auto* e = loaded->FindEntity("Scaled");
    ASSERT_NE(e, nullptr);
    EXPECT_NEAR(e->GetTransform().GetScale().x, 3.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetScale().y, 4.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetScale().z, 5.0f, 0.001f);
}

TEST(ScenePersistencePhase5, SerializeContainsSceneName)
{
    Scene scene("NameCheckScene");
    gx::String text = ScenePersistence::SerializeToString(scene);
    EXPECT_NE(text.find("NameCheckScene"), gx::String::npos);
}

// ============================================================================
// 追加テスト: バイナリシリアライゼーション
// ============================================================================

TEST(ScenePersistencePhase5, BinaryRoundTripWithHierarchy)
{
    Scene scene("BinHierarchy");
    auto* parent = scene.CreateEntity("Parent");
    auto* child = scene.CreateEntity("Child");
    child->SetParent(parent);

    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 2u);
    auto* loadedChild = loaded->FindEntity("Child");
    ASSERT_NE(loadedChild, nullptr);
    ASSERT_NE(loadedChild->GetParent(), nullptr);
    EXPECT_EQ(loadedChild->GetParent()->GetName(), "Parent");
}

TEST(ScenePersistencePhase5, BinaryRoundTripWithRotation)
{
    Scene scene("BinRotation");
    auto* entity = scene.CreateEntity("Rotated");
    entity->GetTransform().SetRotation(0.3f, 0.6f, 0.9f);

    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    auto* e = loaded->FindEntity("Rotated");
    ASSERT_NE(e, nullptr);
    EXPECT_NEAR(e->GetTransform().GetRotation().x, 0.3f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetRotation().y, 0.6f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetRotation().z, 0.9f, 0.001f);
}

TEST(ScenePersistencePhase5, BinaryRoundTripWithScale)
{
    Scene scene("BinScale");
    auto* entity = scene.CreateEntity("Scaled");
    entity->GetTransform().SetScale(2.0f, 3.0f, 4.0f);

    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    auto* e = loaded->FindEntity("Scaled");
    ASSERT_NE(e, nullptr);
    EXPECT_NEAR(e->GetTransform().GetScale().x, 2.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetScale().y, 3.0f, 0.001f);
    EXPECT_NEAR(e->GetTransform().GetScale().z, 4.0f, 0.001f);
}

TEST(ScenePersistencePhase5, BinaryRoundTripMultipleEntities)
{
    Scene scene("BinMulti");
    scene.CreateEntity("Alpha");
    scene.CreateEntity("Beta");
    scene.CreateEntity("Gamma");
    scene.CreateEntity("Delta");

    auto bin = ScenePersistence::SerializeToBinary(scene);
    auto loaded = ScenePersistence::DeserializeFromBinary(bin);
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->GetEntityCount(), 4u);
    EXPECT_NE(loaded->FindEntity("Alpha"), nullptr);
    EXPECT_NE(loaded->FindEntity("Beta"), nullptr);
    EXPECT_NE(loaded->FindEntity("Gamma"), nullptr);
    EXPECT_NE(loaded->FindEntity("Delta"), nullptr);
}

TEST(ScenePersistencePhase5, BinarySerializeSize)
{
    Scene scene("SizeCheck");
    scene.CreateEntity("E1");
    auto bin1 = ScenePersistence::SerializeToBinary(scene);

    scene.CreateEntity("E2");
    scene.CreateEntity("E3");
    auto bin2 = ScenePersistence::SerializeToBinary(scene);

    // エンティティが多い方がバイナリデータも大きいはず
    EXPECT_GT(bin2.size(), bin1.size());
}

// ============================================================================
// 追加テスト: ファイルI/O
// ============================================================================

TEST(ScenePersistencePhase5, SaveAndLoadTextFileWithHierarchy)
{
    auto tempPath = (std::filesystem::temp_directory_path() / "gx_p5_hier_text.gxscene").string();

    Scene scene("HierFile");
    auto* parent = scene.CreateEntity("P");
    auto* child = scene.CreateEntity("C");
    child->SetParent(parent);

    EXPECT_TRUE(ScenePersistence::SaveToFile(scene, tempPath, SceneFileFormat::Text));

    auto loaded = ScenePersistence::LoadFromFile(tempPath, SceneFileFormat::Text);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetEntityCount(), 2u);

    auto* lc = loaded->FindEntity("C");
    ASSERT_NE(lc, nullptr);
    ASSERT_NE(lc->GetParent(), nullptr);
    EXPECT_EQ(lc->GetParent()->GetName(), "P");

    std::filesystem::remove(tempPath);
}
