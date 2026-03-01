/// @file test_Integration.cpp
/// @brief Cross-system integration tests: SaveSystem, EventBus, SettingsManager,
///        GameStateMachine, Scene/Entity, MaterialManager interaction patterns

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/SaveSystem.h"
#include "Core/EventBus.h"
#include "Core/SettingsManager.h"
#include "Core/GameState.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/ShaderModelConstants.h"
#include <filesystem>

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// SaveSystem round-trip
// ============================================================================

class IntegrationSaveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_saveDir = "test_integration_saves_" + std::to_string(::GetCurrentProcessId());
    }
    void TearDown() override
    {
        std::filesystem::remove_all(m_saveDir);
    }
    std::string m_saveDir;
};

TEST_F(IntegrationSaveTest, SaveSystemRoundTrip)
{
    // Set data, save to slot, create new instance, load, verify
    {
        SaveSystem save(m_saveDir);
        save.SetInt("level", 10);
        save.SetFloat("hp", 75.5f);
        save.SetString("location", "Forest");
        save.SetBool("bossDefeated", true);
        EXPECT_TRUE(save.SaveToSlot(0, "Checkpoint", 1200.0f));
    }
    {
        SaveSystem save(m_saveDir);
        EXPECT_TRUE(save.LoadFromSlot(0));
        EXPECT_EQ(save.GetInt("level"), 10);
        EXPECT_FLOAT_EQ(save.GetFloat("hp"), 75.5f);
        EXPECT_EQ(save.GetString("location"), "Forest");
        EXPECT_TRUE(save.GetBool("bossDefeated"));
    }
}

TEST_F(IntegrationSaveTest, SaveSystemMultipleTypes)
{
    SaveSystem save(m_saveDir);
    save.SetInt("a", 1);
    save.SetFloat("b", 2.0f);
    save.SetString("c", "three");
    save.SetBool("d", false);

    EXPECT_EQ(save.GetInt("a"), 1);
    EXPECT_FLOAT_EQ(save.GetFloat("b"), 2.0f);
    EXPECT_EQ(save.GetString("c"), "three");
    EXPECT_FALSE(save.GetBool("d"));
}

TEST_F(IntegrationSaveTest, SaveSystemOverwrite)
{
    SaveSystem save(m_saveDir);
    save.SetInt("score", 100);
    EXPECT_EQ(save.GetInt("score"), 100);

    // Overwrite with new value
    save.SetInt("score", 999);
    EXPECT_EQ(save.GetInt("score"), 999);

    // Overwrite string
    save.SetString("name", "Alice");
    save.SetString("name", "Bob");
    EXPECT_EQ(save.GetString("name"), "Bob");
}

// ============================================================================
// EventBus typed event flow
// ============================================================================

namespace
{
    struct DamageEvent { int damage = 0; std::string target; };
    struct HealEvent { int amount = 0; };
    struct LevelUpEvent { int newLevel = 0; };
}

class IntegrationEventBusTest : public ::testing::Test
{
protected:
    void SetUp() override { EventBus::Instance().Clear(); }
    void TearDown() override { EventBus::Instance().Clear(); }
};

TEST_F(IntegrationEventBusTest, EventBusTypedEvent)
{
    int lastDamage = 0;
    std::string lastTarget;

    EventBus::Instance().Subscribe<DamageEvent>([&](const DamageEvent& e) {
        lastDamage = e.damage;
        lastTarget = e.target;
    });

    EventBus::Instance().Fire(DamageEvent{ 50, "Goblin" });
    EXPECT_EQ(lastDamage, 50);
    EXPECT_EQ(lastTarget, "Goblin");
}

TEST_F(IntegrationEventBusTest, EventBusMultipleSubscribers)
{
    int totalDamage = 0;
    int hitCount = 0;

    EventBus::Instance().Subscribe<DamageEvent>([&](const DamageEvent& e) {
        totalDamage += e.damage;
    });
    EventBus::Instance().Subscribe<DamageEvent>([&](const DamageEvent&) {
        hitCount++;
    });

    EventBus::Instance().Fire(DamageEvent{ 25, "Slime" });
    EventBus::Instance().Fire(DamageEvent{ 30, "Dragon" });

    EXPECT_EQ(totalDamage, 55);
    EXPECT_EQ(hitCount, 2);
}

TEST_F(IntegrationEventBusTest, EventBusQueueDispatch)
{
    int received = 0;
    EventBus::Instance().Subscribe<LevelUpEvent>([&](const LevelUpEvent& e) {
        received = e.newLevel;
    });

    // Queue but don't dispatch yet
    EventBus::Instance().Queue(LevelUpEvent{ 5 });
    EXPECT_EQ(received, 0);

    // Dispatch queued events
    EventBus::Instance().DispatchQueued();
    EXPECT_EQ(received, 5);
}

// ============================================================================
// GameStateMachine lifecycle
// ============================================================================

TEST(IntegrationTest, GameStateLifecycle)
{
    GameStateMachine gsm;
    std::vector<std::string> log;

    auto titleId = gsm.RegisterState({
        "Title",
        [&]() { log.push_back("title_enter"); },
        [&](float dt) { log.push_back("title_update"); },
        [&]() { log.push_back("title_draw"); },
        [&]() { log.push_back("title_exit"); }
    });

    gsm.ChangeState(titleId);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Title");

    gsm.Update(0.016f);
    gsm.Draw();

    ASSERT_GE(log.size(), 3u);
    EXPECT_EQ(log[0], "title_enter");
    EXPECT_EQ(log[1], "title_update");
    EXPECT_EQ(log[2], "title_draw");
}

TEST(IntegrationTest, GameStatePushPop)
{
    GameStateMachine gsm;
    std::string lastState;

    auto gameId = gsm.RegisterState({
        "Game",
        [&]() { lastState = "Game"; },
        nullptr, nullptr, nullptr
    });
    auto menuId = gsm.RegisterState({
        "Menu",
        [&]() { lastState = "Menu"; },
        nullptr, nullptr, nullptr
    });

    gsm.ChangeState(gameId);
    EXPECT_EQ(lastState, "Game");
    EXPECT_EQ(gsm.GetStackDepth(), 1u);

    gsm.PushState(menuId);
    EXPECT_EQ(lastState, "Menu");
    EXPECT_EQ(gsm.GetStackDepth(), 2u);

    gsm.PopState();
    EXPECT_EQ(gsm.GetCurrentStateName(), "Game");
    EXPECT_EQ(gsm.GetStackDepth(), 1u);
}

TEST(IntegrationTest, GameStateChange)
{
    GameStateMachine gsm;
    bool gameExited = false;

    auto gameId = gsm.RegisterState({
        "Game", nullptr, nullptr, nullptr,
        [&]() { gameExited = true; }
    });
    auto resultId = gsm.RegisterState({
        "Result", nullptr, nullptr, nullptr, nullptr
    });

    gsm.ChangeState(gameId);
    EXPECT_FALSE(gameExited);

    gsm.ChangeState(resultId);
    EXPECT_TRUE(gameExited);
    EXPECT_EQ(gsm.GetCurrentStateName(), "Result");
}

// ============================================================================
// Scene + Entity lifecycle
// ============================================================================

TEST(IntegrationTest, SceneCreateEntity)
{
    Scene scene("TestScene");
    Entity* e = scene.CreateEntity("Player");
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->GetName(), "Player");
    EXPECT_TRUE(e->IsActive());
}

TEST(IntegrationTest, SceneDestroyEntity)
{
    Scene scene;
    Entity* e = scene.CreateEntity("Temp");
    EXPECT_EQ(scene.GetEntityCount(), 1u);

    scene.DestroyEntity(e);
    scene.Update(0.0f); // Process pending destroys
    EXPECT_EQ(scene.FindEntity("Temp"), nullptr);
}

TEST(IntegrationTest, SceneEntityCount)
{
    Scene scene;
    EXPECT_EQ(scene.GetEntityCount(), 0u);

    scene.CreateEntity("A");
    scene.CreateEntity("B");
    scene.CreateEntity("C");
    EXPECT_EQ(scene.GetEntityCount(), 3u);
}

TEST(IntegrationTest, SceneMultipleEntities)
{
    Scene scene;
    Entity* player = scene.CreateEntity("Player");
    Entity* enemy1 = scene.CreateEntity("Enemy1");
    Entity* enemy2 = scene.CreateEntity("Enemy2");

    // All unique IDs
    EXPECT_NE(player->GetID(), enemy1->GetID());
    EXPECT_NE(enemy1->GetID(), enemy2->GetID());

    // Find by name
    EXPECT_EQ(scene.FindEntity("Player"), player);
    EXPECT_EQ(scene.FindEntity("Enemy1"), enemy1);
    EXPECT_EQ(scene.FindEntity("Enemy2"), enemy2);
    EXPECT_EQ(scene.FindEntity("NonExistent"), nullptr);

    // Parent-child relationships
    enemy1->SetParent(player);
    EXPECT_EQ(enemy1->GetParent(), player);
    EXPECT_EQ(player->GetChildren().size(), 1u);
}

// ============================================================================
// MaterialManager lifecycle and freelist
// ============================================================================

TEST(IntegrationTest, MaterialManagerLifecycle)
{
    MaterialManager mgr;

    // Create a material with specific values
    Material mat;
    mat.constants.metallicFactor = 0.8f;
    mat.constants.roughnessFactor = 0.2f;
    mat.shaderModel = gxfmt::ShaderModel::Standard;

    int h = mgr.CreateMaterial(mat);
    EXPECT_GE(h, 0);

    // Retrieve and verify
    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_NEAR(got->constants.metallicFactor, 0.8f, kEps);
    EXPECT_NEAR(got->constants.roughnessFactor, 0.2f, kEps);

    // Modify through pointer
    got->constants.metallicFactor = 0.5f;
    Material* verify = mgr.GetMaterial(h);
    EXPECT_NEAR(verify->constants.metallicFactor, 0.5f, kEps);

    // Release
    mgr.ReleaseMaterial(h);
    EXPECT_EQ(mgr.GetMaterial(h), nullptr);
}

TEST(IntegrationTest, MaterialManagerFreelist)
{
    MaterialManager mgr;
    Material mat;

    // Create three materials
    int h0 = mgr.CreateMaterial(mat);
    int h1 = mgr.CreateMaterial(mat);
    int h2 = mgr.CreateMaterial(mat);

    EXPECT_NE(h0, h1);
    EXPECT_NE(h1, h2);

    // Release middle one
    mgr.ReleaseMaterial(h1);
    EXPECT_EQ(mgr.GetMaterial(h1), nullptr);

    // New allocation should reuse h1
    int h3 = mgr.CreateMaterial(mat);
    EXPECT_EQ(h3, h1);

    // h0 and h2 should still be valid
    EXPECT_NE(mgr.GetMaterial(h0), nullptr);
    EXPECT_NE(mgr.GetMaterial(h2), nullptr);
}

TEST(IntegrationTest, MaterialManagerMultiple)
{
    MaterialManager mgr;

    // Create many materials and verify they all have unique handles
    std::vector<int> handles;
    for (int i = 0; i < 50; ++i)
    {
        Material mat;
        mat.constants.metallicFactor = static_cast<float>(i) / 50.0f;
        int h = mgr.CreateMaterial(mat);
        EXPECT_GE(h, 0);
        handles.push_back(h);
    }

    // Verify uniqueness
    for (size_t i = 0; i < handles.size(); ++i)
    {
        for (size_t j = i + 1; j < handles.size(); ++j)
        {
            EXPECT_NE(handles[i], handles[j]);
        }
    }

    // Verify data integrity
    for (int i = 0; i < 50; ++i)
    {
        Material* got = mgr.GetMaterial(handles[i]);
        ASSERT_NE(got, nullptr);
        EXPECT_NEAR(got->constants.metallicFactor, static_cast<float>(i) / 50.0f, kEps);
    }
}

// ============================================================================
// Settings round-trip
// ============================================================================

class IntegrationSettingsTest : public ::testing::Test
{
protected:
    void SetUp() override { SettingsManager::Instance().Clear(); }
    void TearDown() override
    {
        SettingsManager::Instance().Clear();
        std::filesystem::remove("test_integration_settings.json");
    }
};

TEST_F(IntegrationSettingsTest, SettingsRoundTrip)
{
    auto& sm = SettingsManager::Instance();
    sm.SetInt("display", "width", 2560);
    sm.SetInt("display", "height", 1440);
    sm.SetFloat("audio", "sfxVolume", 0.6f);
    sm.SetBool("gameplay", "invertY", true);
    sm.SetString("player", "nickname", "TestUser");

    // Save to file
    EXPECT_TRUE(sm.SaveToFile("test_integration_settings.json"));

    // Clear and reload
    sm.Clear();
    EXPECT_EQ(sm.GetInt("display", "width"), 0);

    EXPECT_TRUE(sm.LoadFromFile("test_integration_settings.json"));

    // Verify values survived the round-trip (values stored as strings internally)
    EXPECT_EQ(sm.GetString("display", "width"), "2560");
    EXPECT_EQ(sm.GetString("display", "height"), "1440");
    EXPECT_EQ(sm.GetString("player", "nickname"), "TestUser");
}

// ============================================================================
// Material shader model and texture slots
// ============================================================================

TEST(IntegrationTest, MaterialShaderModelSet)
{
    MaterialManager mgr;
    Material mat;
    mat.shaderModel = gxfmt::ShaderModel::Toon;
    mat.shaderParams = gxfmt::DefaultShaderModelParams(gxfmt::ShaderModel::Toon);

    int h = mgr.CreateMaterial(mat);
    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->shaderModel, gxfmt::ShaderModel::Toon);
    EXPECT_NEAR(got->shaderParams.outlineWidth, 0.002f, kEps);
}

TEST(IntegrationTest, MaterialTextureSlots)
{
    MaterialManager mgr;
    Material mat;
    int h = mgr.CreateMaterial(mat);

    // Set all available texture slots
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Albedo, 10));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Normal, 11));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::MetalRoughness, 12));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::AO, 13));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Emissive, 14));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::ToonRamp, 15));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::SubsurfaceMap, 16));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::ClearCoatMask, 17));

    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->albedoMapHandle, 10);
    EXPECT_EQ(got->normalMapHandle, 11);
    EXPECT_EQ(got->metRoughMapHandle, 12);
    EXPECT_EQ(got->aoMapHandle, 13);
    EXPECT_EQ(got->emissiveMapHandle, 14);
    EXPECT_EQ(got->toonRampMapHandle, 15);
    EXPECT_EQ(got->subsurfaceMapHandle, 16);
    EXPECT_EQ(got->clearCoatMaskMapHandle, 17);

    // Clear a slot by setting to -1
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Albedo, -1));
    EXPECT_EQ(got->albedoMapHandle, -1);
}

// ============================================================================
// Integration: Settings + SaveSystem coordination
// ============================================================================

TEST_F(IntegrationSettingsTest, IntegrationSettingsSave)
{
    auto& sm = SettingsManager::Instance();

    // Set game settings
    sm.SetInt("video", "resolutionX", 1920);
    sm.SetInt("video", "resolutionY", 1080);
    sm.SetFloat("audio", "master", 0.8f);
    sm.SetBool("video", "fullscreen", false);

    // Verify settings are accessible before save
    EXPECT_EQ(sm.GetInt("video", "resolutionX"), 1920);
    EXPECT_EQ(sm.GetInt("video", "resolutionY"), 1080);
    EXPECT_FLOAT_EQ(sm.GetFloat("audio", "master"), 0.8f);
    EXPECT_FALSE(sm.GetBool("video", "fullscreen"));

    // Modify and verify update works
    sm.SetInt("video", "resolutionX", 2560);
    EXPECT_EQ(sm.GetInt("video", "resolutionX"), 2560);

    // Multiple sections
    sm.SetString("input", "bindJump", "Space");
    sm.SetString("input", "bindShoot", "LeftClick");
    EXPECT_EQ(sm.GetString("input", "bindJump"), "Space");
    EXPECT_EQ(sm.GetString("input", "bindShoot"), "LeftClick");
}
