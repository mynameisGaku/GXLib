/// @file test_Prefab.cpp
/// @brief Prefabキャプチャ/インスタンス化ラウンドトリップのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Prefab.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/Components.h"

using namespace gx;

// ============================================================================
// 基本的な有効性
// ============================================================================

TEST(PrefabTest, DefaultInvalid)
{
    Prefab prefab;
    EXPECT_FALSE(prefab.IsValid());
    EXPECT_TRUE(prefab.GetJson().empty());
}

// ============================================================================
// CaptureFromEntity + Instantiate ラウンドトリップ
// ============================================================================

TEST(PrefabTest, CaptureFromEntity_Valid)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Template");
    e->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);

    Prefab prefab;
    bool ok = prefab.CaptureFromEntity(*e);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(prefab.IsValid());
    EXPECT_FALSE(prefab.GetJson().empty());
}

TEST(PrefabTest, Instantiate_CreatesEntity)
{
    Scene srcScene;
    Entity* src = srcScene.CreateEntity("Soldier");
    src->GetTransform().SetPosition(5.0f, 0.0f, 10.0f);

    Prefab prefab;
    prefab.CaptureFromEntity(*src);

    Scene dstScene;
    Entity* inst = prefab.Instantiate(dstScene);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(dstScene.GetEntityCount(), 1u);
}

TEST(PrefabTest, Instantiate_PreservesName)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Tree");

    Prefab prefab;
    prefab.CaptureFromEntity(*e);

    Scene dstScene;
    Entity* inst = prefab.Instantiate(dstScene);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->GetName(), "Tree");
}

TEST(PrefabTest, Instantiate_OverrideName)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Tree");

    Prefab prefab;
    prefab.CaptureFromEntity(*e);

    Scene dstScene;
    Entity* inst = prefab.Instantiate(dstScene, "CustomTree");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->GetName(), "CustomTree");
}

TEST(PrefabTest, Instantiate_PreservesPosition)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Box");
    e->GetTransform().SetPosition(10.0f, 20.0f, 30.0f);

    Prefab prefab;
    prefab.CaptureFromEntity(*e);

    Scene dstScene;
    Entity* inst = prefab.Instantiate(dstScene);
    ASSERT_NE(inst, nullptr);

    const auto& pos = inst->GetTransform().GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, 1e-3f);
    EXPECT_NEAR(pos.y, 20.0f, 1e-3f);
    EXPECT_NEAR(pos.z, 30.0f, 1e-3f);
}

TEST(PrefabTest, Instantiate_MultipleInstances)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Enemy");

    Prefab prefab;
    prefab.CaptureFromEntity(*e);

    Scene dstScene;
    Entity* inst1 = prefab.Instantiate(dstScene, "Enemy1");
    Entity* inst2 = prefab.Instantiate(dstScene, "Enemy2");
    Entity* inst3 = prefab.Instantiate(dstScene, "Enemy3");
    EXPECT_EQ(dstScene.GetEntityCount(), 3u);
    EXPECT_NE(inst1->GetID(), inst2->GetID());
    EXPECT_NE(inst2->GetID(), inst3->GetID());
}

TEST(PrefabTest, InvalidPrefab_InstantiateReturnsNull)
{
    Prefab prefab;
    Scene scene;
    Entity* inst = prefab.Instantiate(scene);
    EXPECT_EQ(inst, nullptr);
}

// ============================================================================
// SaveToFile / LoadFromFile roundtrip
// ============================================================================

TEST(PrefabTest, SaveAndLoad_Roundtrip)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Barrel");
    e->GetTransform().SetPosition(7.0f, 3.0f, -2.0f);

    Prefab original;
    original.CaptureFromEntity(*e);
    ASSERT_TRUE(original.IsValid());

    // 一時ファイルに保存
    gx::String tmpPath = "test_prefab_tmp.json";
    ASSERT_TRUE(original.SaveToFile(tmpPath));

    // 再読み込み
    Prefab loaded;
    ASSERT_TRUE(loaded.LoadFromFile(tmpPath));
    EXPECT_TRUE(loaded.IsValid());

    // 読み込んだプレハブからインスタンス化
    Scene dstScene;
    Entity* inst = loaded.Instantiate(dstScene);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->GetName(), "Barrel");

    const auto& pos = inst->GetTransform().GetPosition();
    EXPECT_NEAR(pos.x, 7.0f, 1e-3f);
    EXPECT_NEAR(pos.y, 3.0f, 1e-3f);
    EXPECT_NEAR(pos.z, -2.0f, 1e-3f);

    // クリーンアップ
    std::remove(tmpPath.c_str());
}

TEST(PrefabTest, LoadFromFile_NonExistent)
{
    Prefab prefab;
    EXPECT_FALSE(prefab.LoadFromFile("nonexistent_file_12345.json"));
    EXPECT_FALSE(prefab.IsValid());
}

// ============================================================================
// コンポーネント保持
// ============================================================================

TEST(PrefabTest, CaptureWithComponents)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Scripted");
    e->GetTransform().SetPosition(1.0f, 1.0f, 1.0f);
    e->GetTransform().SetScale(2.0f, 2.0f, 2.0f);

    Prefab prefab;
    EXPECT_TRUE(prefab.CaptureFromEntity(*e));

    Scene dstScene;
    Entity* inst = prefab.Instantiate(dstScene);
    ASSERT_NE(inst, nullptr);

    const auto& scale = inst->GetTransform().GetScale();
    EXPECT_NEAR(scale.x, 2.0f, 1e-3f);
    EXPECT_NEAR(scale.y, 2.0f, 1e-3f);
    EXPECT_NEAR(scale.z, 2.0f, 1e-3f);
}
