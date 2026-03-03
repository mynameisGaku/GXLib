/// @file test_SceneSerializer.cpp
/// @brief SceneSerializer JSONラウンドトリップテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Scene.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Scene/Components.h"
#include "Graphics/3D/GraphicsComponents.h"

using namespace gx;

TEST(SceneSerializerTest, ToJsonString_EmptyScene)
{
    Scene scene("Empty");
    gx::String json = SceneSerializer::ToJsonString(scene);
    EXPECT_FALSE(json.empty());
    EXPECT_EQ(json[0], '{');
}

TEST(SceneSerializerTest, RoundTrip_EmptyScene)
{
    Scene scene("TestScene");
    gx::String json = SceneSerializer::ToJsonString(scene);

    Scene loaded;
    bool ok = SceneSerializer::FromJsonString(loaded, json);
    EXPECT_TRUE(ok);
    EXPECT_EQ(loaded.GetEntityCount(), 0u);
}

TEST(SceneSerializerTest, RoundTrip_SingleEntity)
{
    Scene scene;
    scene.CreateEntity("Box");
    gx::String json = SceneSerializer::ToJsonString(scene);

    Scene loaded;
    bool ok = SceneSerializer::FromJsonString(loaded, json);
    EXPECT_TRUE(ok);
    EXPECT_EQ(loaded.GetEntityCount(), 1u);
    Entity* e = loaded.FindEntity("Box");
    EXPECT_NE(e, nullptr);
}

TEST(SceneSerializerTest, RoundTrip_EntityTransform)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Moved");
    e->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
    gx::String json = SceneSerializer::ToJsonString(scene);

    Scene loaded;
    SceneSerializer::FromJsonString(loaded, json);
    Entity* le = loaded.FindEntity("Moved");
    ASSERT_NE(le, nullptr);
    const auto& pos = le->GetTransform().GetPosition();
    EXPECT_NEAR(pos.x, 1.0f, 1e-3f);
    EXPECT_NEAR(pos.y, 2.0f, 1e-3f);
    EXPECT_NEAR(pos.z, 3.0f, 1e-3f);
}

TEST(SceneSerializerTest, RoundTrip_Hierarchy)
{
    Scene scene;
    Entity* parent = scene.CreateEntity("Parent");
    Entity* child = scene.CreateEntity("Child");
    child->SetParent(parent);
    gx::String json = SceneSerializer::ToJsonString(scene);

    Scene loaded;
    SceneSerializer::FromJsonString(loaded, json);
    Entity* lChild = loaded.FindEntity("Child");
    ASSERT_NE(lChild, nullptr);
    ASSERT_NE(lChild->GetParent(), nullptr);
    EXPECT_EQ(lChild->GetParent()->GetName(), "Parent");
}

TEST(SceneSerializerTest, RoundTrip_CameraComponent)
{
    Scene scene;
    Entity* e = scene.CreateEntity("CamEntity");
    e->AddComponent<CameraComponent>();
    gx::String json = SceneSerializer::ToJsonString(scene);

    Scene loaded;
    SceneSerializer::FromJsonString(loaded, json);
    Entity* le = loaded.FindEntity("CamEntity");
    ASSERT_NE(le, nullptr);
    EXPECT_TRUE(le->HasComponent<CameraComponent>());
}

TEST(SceneSerializerTest, RoundTrip_LightComponent)
{
    Scene scene;
    Entity* e = scene.CreateEntity("LightEntity");
    e->AddComponent<LightComponent>();
    gx::String json = SceneSerializer::ToJsonString(scene);

    Scene loaded;
    SceneSerializer::FromJsonString(loaded, json);
    Entity* le = loaded.FindEntity("LightEntity");
    ASSERT_NE(le, nullptr);
    EXPECT_TRUE(le->HasComponent<LightComponent>());
}

TEST(SceneSerializerTest, FromJsonString_InvalidJson)
{
    Scene scene;
    bool ok = SceneSerializer::FromJsonString(scene, "this is not json {{{");
    EXPECT_FALSE(ok);
}
