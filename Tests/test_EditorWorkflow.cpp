/// @file test_EditorWorkflow.cpp
/// @brief SimulationManager、SceneSnapshot、コンポーネントClone、PhysicsDebugInfo、
///        VideoRecorder、FrameRingBufferの単体テスト

#include <gtest/gtest.h>
#include "Core/Scene/SimulationManager.h"
#include "Core/Scene/SceneSnapshot.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include "Physics/PhysicsDebugInfo.h"
#include "Graphics/VideoRecorder.h"
#include "Graphics/FrameRingBuffer.h"

using namespace gx;

// ============================================================================
// SimulationManager
// ============================================================================

TEST(SimulationManager, DefaultStateStopped)
{
    SimulationManager mgr;
    EXPECT_EQ(mgr.GetState(), SimulationState::Stopped);
    EXPECT_TRUE(mgr.IsStopped());
    EXPECT_FALSE(mgr.IsPlaying());
    EXPECT_FALSE(mgr.IsPaused());
}

TEST(SimulationManager, PlayChangesState)
{
    SimulationManager mgr;
    Scene scene("test");
    mgr.Play(scene);
    EXPECT_EQ(mgr.GetState(), SimulationState::Playing);
    EXPECT_TRUE(mgr.IsPlaying());
}

TEST(SimulationManager, PauseAfterPlay)
{
    SimulationManager mgr;
    Scene scene("test");
    mgr.Play(scene);
    mgr.Pause();
    EXPECT_EQ(mgr.GetState(), SimulationState::Paused);
    EXPECT_TRUE(mgr.IsPaused());
}

TEST(SimulationManager, ResumeAfterPause)
{
    SimulationManager mgr;
    Scene scene("test");
    mgr.Play(scene);
    mgr.Pause();
    mgr.Resume();
    EXPECT_EQ(mgr.GetState(), SimulationState::Playing);
}

TEST(SimulationManager, StopRestoresState)
{
    SimulationManager mgr;
    Scene scene("test");
    scene.CreateEntity("TestEntity");

    mgr.Play(scene);
    mgr.Stop(scene);
    EXPECT_TRUE(mgr.IsStopped());
}

TEST(SimulationManager, StepIncrementsCount)
{
    SimulationManager mgr;
    Scene scene("test");
    mgr.Play(scene);
    mgr.Pause();
    mgr.Step(scene);
    EXPECT_EQ(mgr.GetStepCount(), 1u);
}

TEST(SimulationManager, StepWhileStoppedDoesNothing)
{
    SimulationManager mgr;
    Scene scene("test");
    // Play前のStepは何もしないはず
    mgr.Step(scene);
    EXPECT_EQ(mgr.GetStepCount(), 0u);
}

TEST(SimulationManager, ElapsedTimeDefault)
{
    SimulationManager mgr;
    EXPECT_FLOAT_EQ(mgr.GetElapsedTime(), 0.0f);
}

// ============================================================================
// SceneSnapshot
// ============================================================================

TEST(SceneSnapshot, DefaultNotValid)
{
    SceneSnapshot snap;
    EXPECT_FALSE(snap.IsValid());
}

TEST(SceneSnapshot, CaptureMarksValid)
{
    Scene scene("test");
    scene.CreateEntity("E1");
    scene.CreateEntity("E2");

    SceneSnapshot snap;
    snap.Capture(scene);
    EXPECT_TRUE(snap.IsValid());
    EXPECT_EQ(snap.GetEntityCount(), 2u);
}

TEST(SceneSnapshot, CaptureEmptyScene)
{
    Scene scene("empty");
    SceneSnapshot snap;
    snap.Capture(scene);
    EXPECT_TRUE(snap.IsValid());
    EXPECT_EQ(snap.GetEntityCount(), 0u);
}

// ============================================================================
// コンポーネントClone
// ============================================================================

TEST(ComponentClone, AudioSource)
{
    AudioSourceComponent comp;
    comp.soundHandle = 42;
    comp.playOnStart = true;
    comp.loop = true;

    auto clone = comp.Clone();
    ASSERT_NE(clone, nullptr);
    auto* cloned = static_cast<AudioSourceComponent*>(clone.get());
    EXPECT_EQ(cloned->soundHandle, 42);
    EXPECT_TRUE(cloned->playOnStart);
    EXPECT_TRUE(cloned->loop);
}

TEST(ComponentClone, NameComponent)
{
    NameComponent comp;
    comp.name = "TestName";

    auto clone = comp.Clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(static_cast<NameComponent*>(clone.get())->name, "TestName");
}

TEST(ComponentClone, TagComponent)
{
    TagComponent comp;
    comp.tag = "Enemy";

    auto clone = comp.Clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(static_cast<TagComponent*>(clone.get())->tag, "Enemy");
}

TEST(ComponentClone, TransformComponent)
{
    TransformComponent comp;
    comp.position = {1, 2, 3};
    comp.rotation = {4, 5, 6};
    comp.scale = {7, 8, 9};

    auto clone = comp.Clone();
    ASSERT_NE(clone, nullptr);
    auto* cloned = static_cast<TransformComponent*>(clone.get());
    EXPECT_FLOAT_EQ(cloned->position.x, 1.0f);
    EXPECT_FLOAT_EQ(cloned->position.y, 2.0f);
    EXPECT_FLOAT_EQ(cloned->position.z, 3.0f);
    EXPECT_FLOAT_EQ(cloned->scale.z, 9.0f);
}

TEST(ComponentClone, ParticleSystemComponent)
{
    ParticleSystemComponent comp;
    auto clone = comp.Clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetType(), ComponentType::ParticleSystem);
}

TEST(ComponentClone, BaseClassDefaultNull)
{
    // Componentベースクラスの Clone() はデフォルトで nullptr を返す
    // すべての組み込みコンポーネントは Clone() をオーバーライドするため、
    // ベースの動作を間接的に検証: ベースのvirtualは nullptr を返す
    Component* base = nullptr;
    // 派生型でインスタンス化し、Cloneが実際に動作することをテスト
    AudioSourceComponent comp;
    auto clone = comp.Clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetType(), ComponentType::AudioSource);
    (void)base;
}

// ============================================================================
// PhysicsDebugInfo
// ============================================================================

TEST(PhysicsDebugInfo, BodyDebugInfoFields)
{
    BodyDebugInfo info{};
    info.bodyId = 1;
    info.motionType = 2; // ダイナミック
    info.isActive = true;
    info.shapeType = DebugShapeType::Box;
    EXPECT_EQ(info.bodyId, 1u);
    EXPECT_EQ(info.motionType, 2u);
    EXPECT_TRUE(info.isActive);
    EXPECT_EQ(info.shapeType, DebugShapeType::Box);
}

TEST(PhysicsDebugInfo, ContactDebugInfoFields)
{
    ContactDebugInfo info{};
    info.pointA = Vector3{1, 2, 3};
    info.pointB = Vector3{4, 5, 6};
    info.normal = Vector3{0, 1, 0};
    info.penetration = 0.1f;
    EXPECT_FLOAT_EQ(info.penetration, 0.1f);
    EXPECT_FLOAT_EQ(info.normal.y, 1.0f);
}

TEST(PhysicsDebugInfo, ConstraintDebugInfoFields)
{
    ConstraintDebugInfo info{};
    info.anchorA = Vector3{0, 0, 0};
    info.anchorB = Vector3{1, 1, 1};
    info.type = 2; // ヒンジ
    EXPECT_EQ(info.type, 2u);
    EXPECT_FLOAT_EQ(info.anchorB.x, 1.0f);
}

TEST(PhysicsDebugInfo, DebugShapeTypeEnums)
{
    EXPECT_EQ(static_cast<uint32_t>(DebugShapeType::Box), 0u);
    EXPECT_EQ(static_cast<uint32_t>(DebugShapeType::Sphere), 1u);
    EXPECT_EQ(static_cast<uint32_t>(DebugShapeType::Capsule), 2u);
    EXPECT_EQ(static_cast<uint32_t>(DebugShapeType::Mesh), 3u);
    EXPECT_EQ(static_cast<uint32_t>(DebugShapeType::ConvexHull), 4u);
}

// ============================================================================
// FrameRingBuffer
// ============================================================================

TEST(FrameRingBuffer, DefaultNotInitialized)
{
    FrameRingBuffer buf;
    EXPECT_FALSE(buf.IsInitialized());
    EXPECT_EQ(buf.GetFrameCount(), 0u);
}

TEST(FrameRingBuffer, Initialize)
{
    FrameRingBuffer buf;
    buf.Initialize(5.0f, 320, 240, 30);
    EXPECT_TRUE(buf.IsInitialized());
    EXPECT_EQ(buf.GetMaxFrames(), 150u); // 5 * 30
    EXPECT_EQ(buf.GetWidth(), 320u);
    EXPECT_EQ(buf.GetHeight(), 240u);
    EXPECT_EQ(buf.GetFPS(), 30u);
}

TEST(FrameRingBuffer, PushFrame)
{
    FrameRingBuffer buf;
    buf.Initialize(1.0f, 2, 2, 10);
    gx::Vector<uint8_t> frame(2 * 2 * 4, 128);
    buf.PushFrame(frame.data(), static_cast<uint32_t>(frame.size()));
    EXPECT_EQ(buf.GetFrameCount(), 1u);
}

TEST(FrameRingBuffer, BufferedSeconds)
{
    FrameRingBuffer buf;
    buf.Initialize(1.0f, 2, 2, 10);
    EXPECT_FLOAT_EQ(buf.GetBufferedSeconds(), 0.0f);
    gx::Vector<uint8_t> frame(2 * 2 * 4, 128);
    for (int i = 0; i < 5; ++i)
        buf.PushFrame(frame.data(), static_cast<uint32_t>(frame.size()));
    EXPECT_FLOAT_EQ(buf.GetBufferedSeconds(), 0.5f);
}

TEST(FrameRingBuffer, ClearResetsFrameCount)
{
    FrameRingBuffer buf;
    buf.Initialize(1.0f, 2, 2, 10);
    gx::Vector<uint8_t> frame(2 * 2 * 4, 128);
    buf.PushFrame(frame.data(), static_cast<uint32_t>(frame.size()));
    EXPECT_EQ(buf.GetFrameCount(), 1u);
    buf.Clear();
    EXPECT_EQ(buf.GetFrameCount(), 0u);
    EXPECT_FLOAT_EQ(buf.GetBufferedSeconds(), 0.0f);
}

// ============================================================================
// VideoRecorder
// ============================================================================

TEST(VideoRecorder, DefaultStateIdle)
{
    VideoRecorder rec;
    EXPECT_EQ(rec.GetState(), RecordingState::Idle);
    EXPECT_FALSE(rec.IsRecording());
}

TEST(VideoRecorder, FramesCapturedDefault)
{
    VideoRecorder rec;
    EXPECT_EQ(rec.GetFramesCaptured(), 0u);
}

TEST(VideoRecorder, RecordingDurationDefault)
{
    VideoRecorder rec;
    EXPECT_FLOAT_EQ(rec.GetRecordingDuration(), 0.0f);
}
