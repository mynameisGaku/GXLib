/// @file test_PlayInEditor.cpp
/// @brief Play-in-Editor (PIE) システムのテスト

#include <gtest/gtest.h>
#include "Editor/PlayInEditor.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"

using namespace gx;

// ============================================================================
// 基本テスト
// ============================================================================

TEST(PIEBasic, ConstructDestruct)
{
    PlayInEditor pie;
    EXPECT_TRUE(pie.IsStopped());
}

TEST(PIEBasic, SetScene)
{
    PlayInEditor pie;
    Scene s("S");
    pie.SetScene(&s);
    EXPECT_EQ(pie.GetScene(), &s);
}

TEST(PIEBasic, EnterPlayNoScene)
{
    PlayInEditor pie;
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.IsPlaying());
}

TEST(PIEBasic, EnterPlayEmptyScene)
{
    Scene scene("Empty");
    PlayInEditor pie;
    pie.SetScene(&scene);
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.IsPlaying());
}

TEST(PIEBasic, EnterPlayWithEntities)
{
    Scene scene("Test");
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    PlayInEditor pie;
    pie.SetScene(&scene);
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.IsPlaying());
    EXPECT_TRUE(pie.HasSnapshot());
}

// ============================================================================
// PlayInEditor テスト (Fixture)
// ============================================================================

class PlayInEditorTest : public ::testing::Test
{
protected:
    Scene scene{"TestScene"};
    PlayInEditor pie;

    void SetUp() override
    {
        pie.SetScene(&scene);
    }
};

TEST_F(PlayInEditorTest, DefaultState)
{
    PlayInEditor fresh;
    EXPECT_EQ(fresh.GetState(), PIEState::Stopped);
    EXPECT_TRUE(fresh.IsStopped());
    EXPECT_FALSE(fresh.IsPlaying());
    EXPECT_FALSE(fresh.IsPaused());
}

TEST_F(PlayInEditorTest, ConfigDefault)
{
    const auto& cfg = pie.GetConfig();
    EXPECT_FALSE(cfg.startPaused);
    EXPECT_TRUE(cfg.captureInput);
    EXPECT_TRUE(cfg.showDebugOverlay);
    EXPECT_TRUE(cfg.simulatePhysics);
    EXPECT_TRUE(cfg.simulateAI);
    EXPECT_FALSE(cfg.simulateAudio);
    EXPECT_NEAR(cfg.fixedTimeStep, 1.0f / 60.0f, 1e-5f);
}

TEST_F(PlayInEditorTest, SetConfig)
{
    PIEConfig cfg;
    cfg.startPaused = true;
    cfg.captureInput = false;
    cfg.simulateAudio = true;
    pie.SetConfig(cfg);

    EXPECT_TRUE(pie.GetConfig().startPaused);
    EXPECT_FALSE(pie.GetConfig().captureInput);
    EXPECT_TRUE(pie.GetConfig().simulateAudio);
}

TEST_F(PlayInEditorTest, EnterPlayMode)
{
    pie.EnterPlayMode();
    EXPECT_EQ(pie.GetState(), PIEState::Playing);
    EXPECT_TRUE(pie.IsPlaying());
    EXPECT_FALSE(pie.IsStopped());
}

TEST_F(PlayInEditorTest, ExitPlayMode)
{
    pie.EnterPlayMode();
    pie.ExitPlayMode();
    EXPECT_EQ(pie.GetState(), PIEState::Stopped);
    EXPECT_TRUE(pie.IsStopped());
}

TEST_F(PlayInEditorTest, StateTransitions)
{
    EXPECT_TRUE(pie.IsStopped());
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.IsPlaying());
    pie.Pause();
    EXPECT_TRUE(pie.IsPaused());
    pie.Resume();
    EXPECT_TRUE(pie.IsPlaying());
    pie.ExitPlayMode();
    EXPECT_TRUE(pie.IsStopped());
}

TEST_F(PlayInEditorTest, PauseResume)
{
    pie.EnterPlayMode();
    pie.Pause();
    EXPECT_TRUE(pie.IsPaused());
    pie.Resume();
    EXPECT_TRUE(pie.IsPlaying());
}

TEST_F(PlayInEditorTest, PauseWhileStopped)
{
    pie.Pause();
    EXPECT_TRUE(pie.IsStopped());
}

TEST_F(PlayInEditorTest, ResumeWhileStopped)
{
    pie.Resume();
    EXPECT_TRUE(pie.IsStopped());
}

TEST_F(PlayInEditorTest, StepFrame)
{
    pie.EnterPlayMode();
    pie.Pause();
    EXPECT_EQ(pie.GetFrameCount(), 0u);

    pie.StepFrame();
    EXPECT_EQ(pie.GetFrameCount(), 1u);
    EXPECT_GT(pie.GetElapsedTime(), 0.0f);
    EXPECT_TRUE(pie.IsPaused());
}

TEST_F(PlayInEditorTest, StepFrameWhilePlaying)
{
    pie.EnterPlayMode();
    uint64_t before = pie.GetFrameCount();
    pie.StepFrame();
    EXPECT_EQ(pie.GetFrameCount(), before);
}

TEST_F(PlayInEditorTest, FrameCount)
{
    pie.EnterPlayMode();
    EXPECT_EQ(pie.GetFrameCount(), 0u);
    pie.Update(1.0f / 60.0f);
    EXPECT_EQ(pie.GetFrameCount(), 1u);
    pie.Update(1.0f / 60.0f);
    EXPECT_EQ(pie.GetFrameCount(), 2u);
}

TEST_F(PlayInEditorTest, ElapsedTime)
{
    pie.EnterPlayMode();
    EXPECT_FLOAT_EQ(pie.GetElapsedTime(), 0.0f);
    pie.Update(0.5f);
    EXPECT_FLOAT_EQ(pie.GetElapsedTime(), 0.5f);
    pie.Update(0.3f);
    EXPECT_NEAR(pie.GetElapsedTime(), 0.8f, 1e-5f);
}

TEST_F(PlayInEditorTest, UpdatePlaying)
{
    pie.EnterPlayMode();
    pie.Update(0.1f);
    EXPECT_EQ(pie.GetFrameCount(), 1u);
    EXPECT_NEAR(pie.GetElapsedTime(), 0.1f, 1e-5f);
}

TEST_F(PlayInEditorTest, UpdatePaused)
{
    pie.EnterPlayMode();
    pie.Pause();
    pie.Update(0.1f);
    EXPECT_EQ(pie.GetFrameCount(), 0u);
    EXPECT_FLOAT_EQ(pie.GetElapsedTime(), 0.0f);
}

TEST_F(PlayInEditorTest, UpdateStopped)
{
    pie.Update(1.0f);
    EXPECT_EQ(pie.GetFrameCount(), 0u);
    EXPECT_FLOAT_EQ(pie.GetElapsedTime(), 0.0f);
}

TEST_F(PlayInEditorTest, TakeSnapshot)
{
    scene.CreateEntity("Player");
    scene.CreateEntity("Enemy");

    auto snap = pie.TakeSnapshot(scene);
    EXPECT_EQ(snap.entityCount, 2u);
    EXPECT_TRUE(snap.hasData);
}

TEST_F(PlayInEditorTest, RestoreSnapshot)
{
    auto* entity = scene.CreateEntity("Player");
    entity->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);

    pie.EnterPlayMode();
    EXPECT_TRUE(pie.HasSnapshot());

    entity->GetTransform().SetPosition(10.0f, 20.0f, 30.0f);

    pie.ExitPlayMode();

    Entity* restored = scene.FindEntityByID(entity->GetID());
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->GetName(), "Player");
    EXPECT_FLOAT_EQ(restored->GetTransform().GetPosition().x, 1.0f);
    EXPECT_FLOAT_EQ(restored->GetTransform().GetPosition().y, 2.0f);
    EXPECT_FLOAT_EQ(restored->GetTransform().GetPosition().z, 3.0f);
}

TEST_F(PlayInEditorTest, HasSnapshotFalse)
{
    EXPECT_FALSE(pie.HasSnapshot());
}

TEST_F(PlayInEditorTest, HasSnapshotAfterPlay)
{
    scene.CreateEntity("A");
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.HasSnapshot());
}

TEST_F(PlayInEditorTest, IsPlayingQuery)
{
    EXPECT_FALSE(pie.IsPlaying());
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.IsPlaying());
}

TEST_F(PlayInEditorTest, IsPausedQuery)
{
    EXPECT_FALSE(pie.IsPaused());
    pie.EnterPlayMode();
    pie.Pause();
    EXPECT_TRUE(pie.IsPaused());
}

TEST_F(PlayInEditorTest, SetScene)
{
    PlayInEditor p;
    EXPECT_EQ(p.GetScene(), nullptr);
    Scene s("S");
    p.SetScene(&s);
    EXPECT_EQ(p.GetScene(), &s);
}

TEST_F(PlayInEditorTest, CallbackOnEnter)
{
    bool called = false;
    pie.OnEnterPlay = [&]() { called = true; };
    pie.EnterPlayMode();
    EXPECT_TRUE(called);
}

TEST_F(PlayInEditorTest, CallbackOnExit)
{
    bool called = false;
    pie.OnExitPlay = [&]() { called = true; };
    pie.EnterPlayMode();
    pie.ExitPlayMode();
    EXPECT_TRUE(called);
}

TEST_F(PlayInEditorTest, DoubleEnterIgnored)
{
    pie.EnterPlayMode();
    uint64_t frame1 = pie.GetFrameCount();
    pie.Update(0.1f);

    pie.EnterPlayMode();
    EXPECT_EQ(pie.GetFrameCount(), frame1 + 1);
}

TEST_F(PlayInEditorTest, ExitWithoutEnterSafe)
{
    pie.ExitPlayMode();
    EXPECT_TRUE(pie.IsStopped());
}

TEST_F(PlayInEditorTest, StartPausedConfig)
{
    PIEConfig cfg;
    cfg.startPaused = true;
    pie.SetConfig(cfg);
    pie.EnterPlayMode();
    EXPECT_TRUE(pie.IsPaused());
}
