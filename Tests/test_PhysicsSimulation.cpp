/// @file test_PhysicsSimulation.cpp
/// @brief Phase 5: PhysicsWorld3Dシミュレーション、シェイプ、ボディ、重力、レイキャスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Physics/PhysicsWorld3D.h"

using namespace gx;

// ============================================================================
// 初期化
// ============================================================================

TEST(PhysicsSimulation, InitializeAndShutdown)
{
    PhysicsWorld3D world;
    EXPECT_TRUE(world.Initialize());
    world.Shutdown();
}

TEST(PhysicsSimulation, InitializeWithCustomMaxBodies)
{
    PhysicsWorld3D world;
    EXPECT_TRUE(world.Initialize(512));
    world.Shutdown();
}

// ============================================================================
// 重力
// ============================================================================

TEST(PhysicsSimulation, DefaultGravity)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    Vector3 gravity = world.GetGravity();
    EXPECT_FLOAT_EQ(gravity.x, 0.0f);
    EXPECT_LT(gravity.y, 0.0f); // デフォルト重力は下向き
    EXPECT_FLOAT_EQ(gravity.z, 0.0f);

    world.Shutdown();
}

TEST(PhysicsSimulation, SetGravity)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    world.SetGravity({ 0.0f, -20.0f, 0.0f });
    Vector3 gravity = world.GetGravity();
    EXPECT_FLOAT_EQ(gravity.y, -20.0f);

    world.Shutdown();
}

TEST(PhysicsSimulation, ZeroGravity)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    world.SetGravity({ 0.0f, 0.0f, 0.0f });
    Vector3 gravity = world.GetGravity();
    EXPECT_FLOAT_EQ(gravity.x, 0.0f);
    EXPECT_FLOAT_EQ(gravity.y, 0.0f);
    EXPECT_FLOAT_EQ(gravity.z, 0.0f);

    world.Shutdown();
}

// ============================================================================
// シェイプ生成
// ============================================================================

TEST(PhysicsSimulation, CreateBoxShape)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 1.0f, 1.0f, 1.0f });
    EXPECT_NE(shape, nullptr);

    world.DestroyShape(shape);
    world.Shutdown();
}

TEST(PhysicsSimulation, CreateSphereShape)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateSphereShape(0.5f);
    EXPECT_NE(shape, nullptr);

    world.DestroyShape(shape);
    world.Shutdown();
}

TEST(PhysicsSimulation, CreateCapsuleShape)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateCapsuleShape(0.5f, 0.3f);
    EXPECT_NE(shape, nullptr);

    world.DestroyShape(shape);
    world.Shutdown();
}

// ============================================================================
// ボディ管理
// ============================================================================

TEST(PhysicsSimulation, AddBody)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 10.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;
    settings.mass = 1.0f;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    EXPECT_TRUE(bodyId.IsValid());

    world.RemoveBody(bodyId);
    world.DestroyShape(shape);
    world.Shutdown();
}

TEST(PhysicsSimulation, GetPositionOfBody)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 5.0f, 10.0f, -3.0f };
    settings.motionType = MotionType3D::Static;
    settings.layer = 0;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    Vector3 pos = world.GetPosition(bodyId);

    EXPECT_NEAR(pos.x, 5.0f, 0.01f);
    EXPECT_NEAR(pos.y, 10.0f, 0.01f);
    EXPECT_NEAR(pos.z, -3.0f, 0.01f);

    world.RemoveBody(bodyId);
    world.DestroyShape(shape);
    world.Shutdown();
}

// ============================================================================
// PhysicsBodyID
// ============================================================================

TEST(PhysicsSimulation, BodyIDDefault)
{
    PhysicsBodyID id;
    EXPECT_FALSE(id.IsValid());
}

// ============================================================================
// シミュレーションステップ
// ============================================================================

TEST(PhysicsSimulation, StepUpdatesPosition)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());
    world.SetGravity({ 0.0f, -9.81f, 0.0f });

    // 床を作成
    auto* floorShape = world.CreateBoxShape({ 50.0f, 0.5f, 50.0f });
    PhysicsBodySettings floorSettings;
    floorSettings.position = { 0.0f, -0.5f, 0.0f };
    floorSettings.motionType = MotionType3D::Static;
    floorSettings.layer = 0;
    world.AddBody(floorShape, floorSettings);

    // 高い位置にダイナミックボックスを作成
    auto* boxShape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings boxSettings;
    boxSettings.position = { 0.0f, 20.0f, 0.0f };
    boxSettings.motionType = MotionType3D::Dynamic;
    boxSettings.mass = 1.0f;
    boxSettings.layer = 1;

    PhysicsBodyID boxId = world.AddBody(boxShape, boxSettings);
    Vector3 posBefore = world.GetPosition(boxId);

    // 複数回ステップ
    for (int i = 0; i < 60; ++i)
    {
        world.Step(1.0f / 60.0f);
    }

    Vector3 posAfter = world.GetPosition(boxId);

    // ボックスは重力により落下しているはず
    EXPECT_LT(posAfter.y, posBefore.y);

    world.Shutdown();
}

TEST(PhysicsSimulation, SetLinearVelocity)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());
    world.SetGravity({ 0.0f, 0.0f, 0.0f }); // 重力なし

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 0.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;
    settings.mass = 1.0f;
    settings.linearDamping = 0.0f;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    world.SetLinearVelocity(bodyId, { 10.0f, 0.0f, 0.0f });

    for (int i = 0; i < 30; ++i)
    {
        world.Step(1.0f / 60.0f);
    }

    Vector3 pos = world.GetPosition(bodyId);
    EXPECT_GT(pos.x, 0.0f); // +X方向に移動しているはず

    world.Shutdown();
}

// ============================================================================
// Raycast
// ============================================================================

TEST(PhysicsSimulation, RaycastHitsBody)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    // 静的ボックスを作成
    auto* shape = world.CreateBoxShape({ 1.0f, 1.0f, 1.0f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 0.0f, 10.0f };
    settings.motionType = MotionType3D::Static;
    settings.layer = 0;
    PhysicsBodyID bodyId = world.AddBody(shape, settings);

    // 物理処理の完了を待つ
    world.Step(1.0f / 60.0f);

    // ボックスに向けてレイキャスト
    auto result = world.Raycast({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 100.0f);
    EXPECT_TRUE(result.hit);
    EXPECT_TRUE(result.bodyID.IsValid());
    EXPECT_GT(result.fraction, 0.0f);
    EXPECT_LE(result.fraction, 1.0f);

    world.Shutdown();
}

TEST(PhysicsSimulation, RaycastMissesEmpty)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    // 空の空間にレイキャスト
    auto result = world.Raycast({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 100.0f);
    EXPECT_FALSE(result.hit);

    world.Shutdown();
}

// ============================================================================
// ボディ操作
// ============================================================================

TEST(PhysicsSimulation, SetPosition)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 0.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;
    settings.mass = 1.0f;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    world.SetPosition(bodyId, { 100.0f, 200.0f, 300.0f });

    Vector3 pos = world.GetPosition(bodyId);
    EXPECT_NEAR(pos.x, 100.0f, 0.01f);
    EXPECT_NEAR(pos.y, 200.0f, 0.01f);
    EXPECT_NEAR(pos.z, 300.0f, 0.01f);

    world.Shutdown();
}

TEST(PhysicsSimulation, ApplyImpulse)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());
    world.SetGravity({ 0.0f, 0.0f, 0.0f });

    auto* shape = world.CreateSphereShape(0.5f);
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 0.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;
    settings.mass = 1.0f;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    world.ApplyImpulse(bodyId, { 0.0f, 100.0f, 0.0f });

    world.Step(1.0f / 60.0f);

    Vector3 vel = world.GetLinearVelocity(bodyId);
    EXPECT_GT(vel.y, 0.0f);

    world.Shutdown();
}

// ============================================================================
// 追加テスト: BodySettings
// ============================================================================

TEST(PhysicsSimulation, BodySettingsDefaults)
{
    PhysicsBodySettings settings;
    EXPECT_EQ(settings.motionType, MotionType3D::Dynamic);
    EXPECT_FLOAT_EQ(settings.mass, 1.0f);
    EXPECT_FLOAT_EQ(settings.friction, 0.5f);
    EXPECT_FLOAT_EQ(settings.restitution, 0.3f);
    EXPECT_FLOAT_EQ(settings.linearDamping, 0.05f);
    EXPECT_FLOAT_EQ(settings.angularDamping, 0.05f);
    EXPECT_EQ(settings.layer, 1u);
    EXPECT_EQ(settings.userData, nullptr);
}

TEST(PhysicsSimulation, BodySettingsCustom)
{
    PhysicsBodySettings settings;
    settings.mass = 100.0f;
    settings.friction = 0.8f;
    settings.restitution = 0.1f;
    settings.linearDamping = 0.5f;
    settings.angularDamping = 0.2f;
    settings.layer = 3;

    EXPECT_FLOAT_EQ(settings.mass, 100.0f);
    EXPECT_FLOAT_EQ(settings.friction, 0.8f);
    EXPECT_FLOAT_EQ(settings.restitution, 0.1f);
    EXPECT_FLOAT_EQ(settings.linearDamping, 0.5f);
    EXPECT_FLOAT_EQ(settings.angularDamping, 0.2f);
    EXPECT_EQ(settings.layer, 3u);
}

// ============================================================================
// 追加テスト: MotionType3D列挙型
// ============================================================================

TEST(PhysicsSimulation, MotionTypeEnumValues)
{
    EXPECT_NE(static_cast<int>(MotionType3D::Static), static_cast<int>(MotionType3D::Dynamic));
    EXPECT_NE(static_cast<int>(MotionType3D::Kinematic), static_cast<int>(MotionType3D::Dynamic));
    EXPECT_NE(static_cast<int>(MotionType3D::Static), static_cast<int>(MotionType3D::Kinematic));
}

// ============================================================================
// 追加テスト: ボディ操作
// ============================================================================

TEST(PhysicsSimulation, SetRotation)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 5.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);

    constexpr float s = 0.70710678118f; // sin(45°) = cos(45°)
    Quaternion rot = { 0.0f, s, 0.0f, s };
    world.SetRotation(bodyId, rot);

    Quaternion readBack = world.GetRotation(bodyId);
    EXPECT_NEAR(readBack.y, s, 0.01f);
    EXPECT_NEAR(readBack.w, s, 0.01f);

    world.Shutdown();
}

TEST(PhysicsSimulation, GetWorldTransform)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 3.0f, 4.0f, 5.0f };
    settings.motionType = MotionType3D::Static;
    settings.layer = 0;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    Matrix4x4 transform = world.GetWorldTransform(bodyId);

    EXPECT_NEAR(transform.m[3][0], 3.0f, 0.01f);
    EXPECT_NEAR(transform.m[3][1], 4.0f, 0.01f);
    EXPECT_NEAR(transform.m[3][2], 5.0f, 0.01f);

    world.Shutdown();
}

TEST(PhysicsSimulation, ApplyForce)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());
    world.SetGravity({ 0.0f, 0.0f, 0.0f });

    auto* shape = world.CreateSphereShape(0.5f);
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 0.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;
    settings.mass = 1.0f;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    world.ApplyForce(bodyId, { 100.0f, 0.0f, 0.0f });

    world.Step(1.0f / 60.0f);

    Vector3 vel = world.GetLinearVelocity(bodyId);
    EXPECT_GT(vel.x, 0.0f);

    world.Shutdown();
}

TEST(PhysicsSimulation, SetAngularVelocity)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());
    world.SetGravity({ 0.0f, 0.0f, 0.0f });

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 5.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;
    settings.mass = 1.0f;
    settings.angularDamping = 0.0f;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    world.SetAngularVelocity(bodyId, { 0.0f, 10.0f, 0.0f });

    world.Step(1.0f / 60.0f);

    // 回転が変化しているはず
    Quaternion rot = world.GetRotation(bodyId);
    // 完全な単位クォータニオンではなくなっているはず
    float rotDiff = std::abs(rot.w - 1.0f);
    EXPECT_GT(rotDiff, 0.001f);

    world.Shutdown();
}

TEST(PhysicsSimulation, IsActiveAfterCreation)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 10.0f, 0.0f };
    settings.motionType = MotionType3D::Dynamic;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    EXPECT_TRUE(world.IsActive(bodyId));

    world.Shutdown();
}

TEST(PhysicsSimulation, RemoveBody)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto* shape = world.CreateBoxShape({ 0.5f, 0.5f, 0.5f });
    PhysicsBodySettings settings;
    settings.position = { 0.0f, 0.0f, 0.0f };
    settings.motionType = MotionType3D::Static;
    settings.layer = 0;

    PhysicsBodyID bodyId = world.AddBody(shape, settings);
    EXPECT_TRUE(bodyId.IsValid());

    // 削除がクラッシュしないこと
    world.RemoveBody(bodyId);

    world.DestroyShape(shape);
    world.Shutdown();
}

TEST(PhysicsSimulation, MultipleSteps)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());
    world.SetGravity({ 0.0f, -9.81f, 0.0f });

    // 複数ステップがクラッシュしないこと
    for (int i = 0; i < 100; ++i)
    {
        world.Step(1.0f / 60.0f);
    }

    world.Shutdown();
}

TEST(PhysicsSimulation, SphereCastEmpty)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto hits = world.SphereCast({ 0.0f, 0.0f, 0.0f }, 0.5f, { 0.0f, 1.0f, 0.0f }, 100.0f);
    EXPECT_TRUE(hits.empty());

    world.Shutdown();
}

TEST(PhysicsSimulation, OverlapSphereEmpty)
{
    PhysicsWorld3D world;
    ASSERT_TRUE(world.Initialize());

    auto hits = world.OverlapSphere({ 0.0f, 0.0f, 0.0f }, 1.0f);
    EXPECT_TRUE(hits.empty());

    world.Shutdown();
}
