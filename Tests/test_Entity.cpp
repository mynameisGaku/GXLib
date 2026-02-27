/// @file test_Entity.cpp
/// @brief Entity / Scene unit tests — component management, parent-child hierarchy, scene operations

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include "Graphics/3D/GraphicsComponents.h"

using namespace gx;

TEST(SceneTest, CreateEntity_NotNull)
{
    Scene scene;
    Entity* e = scene.CreateEntity("TestEntity");
    EXPECT_NE(e, nullptr);
}

TEST(SceneTest, CreateEntity_Name)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Player");
    EXPECT_EQ(e->GetName(), "Player");
}

TEST(SceneTest, CreateEntity_UniqueID)
{
    Scene scene;
    Entity* e1 = scene.CreateEntity("A");
    Entity* e2 = scene.CreateEntity("B");
    EXPECT_NE(e1->GetID(), e2->GetID());
}

TEST(SceneTest, GetEntityCount)
{
    Scene scene;
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    scene.CreateEntity("C");
    EXPECT_EQ(scene.GetEntityCount(), 3u);
}

TEST(SceneTest, FindEntityByName)
{
    Scene scene;
    Entity* player = scene.CreateEntity("Player");
    Entity* found = scene.FindEntity("Player");
    EXPECT_EQ(found, player);
}

TEST(SceneTest, FindEntityByName_NotFound)
{
    Scene scene;
    scene.CreateEntity("Player");
    Entity* found = scene.FindEntity("NoExist");
    EXPECT_EQ(found, nullptr);
}

TEST(SceneTest, FindEntityByID)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Test");
    uint32_t id = e->GetID();
    Entity* found = scene.FindEntityByID(id);
    EXPECT_EQ(found, e);
}

TEST(SceneTest, DestroyEntity)
{
    Scene scene;
    Entity* e = scene.CreateEntity("ToDestroy");
    scene.DestroyEntity(e);
    scene.Update(0.0f);  // deferred destruction
    EXPECT_EQ(scene.FindEntity("ToDestroy"), nullptr);
}

TEST(SceneTest, RootEntities)
{
    Scene scene;
    Entity* root = scene.CreateEntity("Root");
    Entity* child = scene.CreateEntity("Child");
    // Both initially in root list (CreateEntity adds to m_rootEntities)
    EXPECT_EQ(scene.GetRootEntities().size(), 2u);
    // SetParent establishes hierarchy on Entity side
    child->SetParent(root);
    EXPECT_EQ(child->GetParent(), root);
    EXPECT_EQ(root->GetChildren().size(), 1u);
    // Root should still be in root entities list
    bool foundRoot = false;
    for (auto* r : scene.GetRootEntities())
        if (r == root) foundRoot = true;
    EXPECT_TRUE(foundRoot);
}

TEST(SceneTest, SetName)
{
    Scene scene("OldName");
    EXPECT_EQ(scene.GetName(), "OldName");
    scene.SetName("NewName");
    EXPECT_EQ(scene.GetName(), "NewName");
}

TEST(SceneTest, Update_NoCrash)
{
    Scene scene;
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    EXPECT_NO_THROW(scene.Update(0.016f));
}

TEST(EntityTest, DefaultTransform)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Test");
    const auto& pos = e->GetTransform().GetPosition();
    EXPECT_NEAR(pos.x, 0.0f, 1e-5f);
    EXPECT_NEAR(pos.y, 0.0f, 1e-5f);
    EXPECT_NEAR(pos.z, 0.0f, 1e-5f);
}

TEST(EntityTest, AddComponent_Camera)
{
    Scene scene;
    Entity* e = scene.CreateEntity("CamEntity");
    auto* cam = e->AddComponent<CameraComponent>();
    EXPECT_NE(cam, nullptr);
}

TEST(EntityTest, GetComponent_Camera)
{
    Scene scene;
    Entity* e = scene.CreateEntity("CamEntity");
    auto* added = e->AddComponent<CameraComponent>();
    auto* got = e->GetComponent<CameraComponent>();
    EXPECT_EQ(added, got);
}

TEST(EntityTest, GetComponent_NotAdded)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Empty");
    auto* light = e->GetComponent<LightComponent>();
    EXPECT_EQ(light, nullptr);
}

TEST(EntityTest, HasComponent)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Test");
    EXPECT_FALSE(e->HasComponent<CameraComponent>());
    e->AddComponent<CameraComponent>();
    EXPECT_TRUE(e->HasComponent<CameraComponent>());
}

TEST(EntityTest, RemoveComponent)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Test");
    e->AddComponent<CameraComponent>();
    EXPECT_TRUE(e->HasComponent<CameraComponent>());
    e->RemoveComponent<CameraComponent>();
    EXPECT_FALSE(e->HasComponent<CameraComponent>());
}

TEST(EntityTest, SetParent)
{
    Scene scene;
    Entity* parent = scene.CreateEntity("Parent");
    Entity* child = scene.CreateEntity("Child");
    child->SetParent(parent);
    EXPECT_EQ(child->GetParent(), parent);
    const auto& children = parent->GetChildren();
    bool found = false;
    for (auto* c : children)
        if (c == child) found = true;
    EXPECT_TRUE(found);
}

TEST(EntityTest, RemoveParent)
{
    Scene scene;
    Entity* parent = scene.CreateEntity("Parent");
    Entity* child = scene.CreateEntity("Child");
    child->SetParent(parent);
    child->SetParent(nullptr);
    EXPECT_EQ(child->GetParent(), nullptr);
}

TEST(EntityTest, ActiveState)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Test");
    EXPECT_TRUE(e->IsActive());
    e->SetActive(false);
    EXPECT_FALSE(e->IsActive());
    e->SetActive(true);
    EXPECT_TRUE(e->IsActive());
}
