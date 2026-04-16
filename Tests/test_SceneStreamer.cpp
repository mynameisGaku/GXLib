/// @file test_SceneStreamer.cpp
/// @brief SceneStreamer シーンストリーミングテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/SceneStreamer.h"
#include "Core/Scene/Scene.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ==================== VolumeManagement ====================

TEST(SceneStreamerTest, AddVolume_ReturnsHandle)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.scenePath = "level_01.scene";
    int handle = streamer.AddVolume(vol);
    EXPECT_GE(handle, 0);
}

TEST(SceneStreamerTest, GetVolumeCount_AfterAdd)
{
    SceneStreamer streamer;
    EXPECT_EQ(streamer.GetVolumeCount(), 0u);

    StreamingVolume vol;
    vol.scenePath = "test.scene";
    streamer.AddVolume(vol);
    EXPECT_EQ(streamer.GetVolumeCount(), 1u);

    streamer.AddVolume(vol);
    EXPECT_EQ(streamer.GetVolumeCount(), 2u);
}

TEST(SceneStreamerTest, RemoveVolume_DecreasesCount)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    int h0 = streamer.AddVolume(vol);
    streamer.AddVolume(vol);
    EXPECT_EQ(streamer.GetVolumeCount(), 2u);

    streamer.RemoveVolume(h0);
    EXPECT_EQ(streamer.GetVolumeCount(), 1u);
}

TEST(SceneStreamerTest, GetVolume_ValidHandle)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.scenePath = "myScene";
    vol.loadRadius = 50.0f;
    int handle = streamer.AddVolume(vol);

    const StreamingVolume* v = streamer.GetVolume(handle);
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->scenePath, "myScene");
    EXPECT_NEAR(v->loadRadius, 50.0f, kEps);
}

TEST(SceneStreamerTest, GetVolume_InvalidHandle)
{
    SceneStreamer streamer;
    EXPECT_EQ(streamer.GetVolume(-1), nullptr);
    EXPECT_EQ(streamer.GetVolume(999), nullptr);
}

// ==================== StreamingState ====================

TEST(SceneStreamerTest, DefaultState_Unloaded)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    int handle = streamer.AddVolume(vol);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Unloaded);
}

TEST(SceneStreamerTest, StateTransition_LoadOnProximity)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    // プレイヤーがloadRadius内に入る
    streamer.Update(5.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Loaded);
}

TEST(SceneStreamerTest, StateTransition_UnloadOnDistance)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    // まず読み込み
    streamer.Update(5.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Loaded);

    // unloadRadius外に出る
    streamer.Update(25.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Unloaded);
}

// ==================== Hysteresis ====================

TEST(SceneStreamerTest, Hysteresis_NoBouncing)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    // 読み込み
    streamer.Update(5.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Loaded);

    // loadRadiusとunloadRadiusの間 → 状態変化なし
    streamer.Update(15.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Loaded);
}

TEST(SceneStreamerTest, Hysteresis_UnloadedStaysUnloaded)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    // loadRadiusとunloadRadiusの間、未読み込み状態 → 読み込まない
    streamer.Update(15.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Unloaded);
}

// ==================== Callbacks ====================

TEST(SceneStreamerTest, Callback_OnLoadedFires)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    int firedHandle = -1;
    streamer.SetOnSceneLoaded([&](int h, Scene&) { firedHandle = h; });

    streamer.Update(5.0f, 0.0f, 0.0f);
    EXPECT_EQ(firedHandle, handle);
}

TEST(SceneStreamerTest, Callback_OnUnloadedFires)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    int unloadedHandle = -1;
    streamer.SetOnSceneUnloaded([&](int h) { unloadedHandle = h; });

    // 読み込み→解放
    streamer.Update(5.0f, 0.0f, 0.0f);
    streamer.Update(25.0f, 0.0f, 0.0f);
    EXPECT_EQ(unloadedHandle, handle);
}

// ==================== ForceLoad / ForceUnload ====================

TEST(SceneStreamerTest, ForceLoad_ImmediateTransition)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.position = { 100.0f, 0.0f, 0.0f }; // 遠い
    int handle = streamer.AddVolume(vol);

    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Unloaded);
    streamer.ForceLoad(handle);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Loaded);
}

TEST(SceneStreamerTest, ForceUnload_ImmediateTransition)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    vol.loadRadius = 10.0f;
    vol.unloadRadius = 20.0f;
    vol.position = { 0.0f, 0.0f, 0.0f };
    int handle = streamer.AddVolume(vol);

    streamer.ForceLoad(handle);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Loaded);

    streamer.ForceUnload(handle);
    EXPECT_EQ(streamer.GetVolumeState(handle), StreamingState::Unloaded);
}

TEST(SceneStreamerTest, ForceLoad_CallbackFires)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    int handle = streamer.AddVolume(vol);

    bool fired = false;
    streamer.SetOnSceneLoaded([&](int, Scene&) { fired = true; });
    streamer.ForceLoad(handle);
    EXPECT_TRUE(fired);
}

TEST(SceneStreamerTest, ForceUnload_CallbackFires)
{
    SceneStreamer streamer;
    StreamingVolume vol;
    int handle = streamer.AddVolume(vol);

    streamer.ForceLoad(handle);

    bool fired = false;
    streamer.SetOnSceneUnloaded([&](int) { fired = true; });
    streamer.ForceUnload(handle);
    EXPECT_TRUE(fired);
}

// ==================== MultiVolume ====================

TEST(SceneStreamerTest, MultiVolume_IndependentTracking)
{
    SceneStreamer streamer;

    StreamingVolume vol0;
    vol0.loadRadius = 10.0f;
    vol0.unloadRadius = 20.0f;
    vol0.position = { 0.0f, 0.0f, 0.0f };

    StreamingVolume vol1;
    vol1.loadRadius = 10.0f;
    vol1.unloadRadius = 20.0f;
    vol1.position = { 100.0f, 0.0f, 0.0f };

    int h0 = streamer.AddVolume(vol0);
    int h1 = streamer.AddVolume(vol1);

    // プレイヤーはvol0の近く
    streamer.Update(5.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(h0), StreamingState::Loaded);
    EXPECT_EQ(streamer.GetVolumeState(h1), StreamingState::Unloaded);
}

TEST(SceneStreamerTest, MultiVolume_BothLoaded)
{
    SceneStreamer streamer;

    StreamingVolume vol0;
    vol0.loadRadius = 10.0f;
    vol0.unloadRadius = 20.0f;
    vol0.position = { 0.0f, 0.0f, 0.0f };

    StreamingVolume vol1;
    vol1.loadRadius = 10.0f;
    vol1.unloadRadius = 20.0f;
    vol1.position = { 5.0f, 0.0f, 0.0f };

    int h0 = streamer.AddVolume(vol0);
    int h1 = streamer.AddVolume(vol1);

    // プレイヤーは両方の近く
    streamer.Update(2.0f, 0.0f, 0.0f);
    EXPECT_EQ(streamer.GetVolumeState(h0), StreamingState::Loaded);
    EXPECT_EQ(streamer.GetVolumeState(h1), StreamingState::Loaded);
}

// ==================== VolumeDefaults ====================

TEST(SceneStreamerTest, VolumeDefaults_Radii)
{
    StreamingVolume vol;
    EXPECT_NEAR(vol.loadRadius, 100.0f, kEps);
    EXPECT_NEAR(vol.unloadRadius, 150.0f, kEps);
}

TEST(SceneStreamerTest, VolumeDefaults_Position)
{
    StreamingVolume vol;
    EXPECT_NEAR(vol.position.x, 0.0f, kEps);
    EXPECT_NEAR(vol.position.y, 0.0f, kEps);
    EXPECT_NEAR(vol.position.z, 0.0f, kEps);
}

TEST(SceneStreamerTest, VolumeDefaults_State)
{
    StreamingVolume vol;
    EXPECT_EQ(vol.state, StreamingState::Unloaded);
}

// ==================== Scene::MergeFrom ====================

TEST(SceneStreamerTest, SceneMergeFrom_EntitiesMove)
{
    Scene target("Target");
    Scene source("Source");

    source.CreateEntity("A");
    source.CreateEntity("B");
    EXPECT_EQ(source.GetEntityCount(), 2u);

    target.MergeFrom(source);
    EXPECT_EQ(target.GetEntityCount(), 2u);
    EXPECT_EQ(source.GetEntityCount(), 0u);
    EXPECT_NE(target.FindEntity("A"), nullptr);
    EXPECT_NE(target.FindEntity("B"), nullptr);
}

TEST(SceneStreamerTest, SceneMergeFrom_AccumulatesEntities)
{
    Scene target("Target");
    target.CreateEntity("Existing");

    Scene source("Source");
    source.CreateEntity("New");

    target.MergeFrom(source);
    EXPECT_EQ(target.GetEntityCount(), 2u);
    EXPECT_NE(target.FindEntity("Existing"), nullptr);
    EXPECT_NE(target.FindEntity("New"), nullptr);
}

// ==================== Scene::ClearAllEntities ====================

TEST(SceneStreamerTest, SceneClearAllEntities_RemovesAll)
{
    Scene scene("Test");
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    scene.CreateEntity("C");
    EXPECT_EQ(scene.GetEntityCount(), 3u);

    scene.ClearAllEntities();
    EXPECT_EQ(scene.GetEntityCount(), 0u);
    EXPECT_EQ(scene.GetRootEntities().size(), 0u);
}

TEST(SceneStreamerTest, SceneClearAllEntities_EmptyScene)
{
    Scene scene("Empty");
    scene.ClearAllEntities();
    EXPECT_EQ(scene.GetEntityCount(), 0u);
}
