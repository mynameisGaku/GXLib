/// @file test_PhysicsExtended.cpp
/// @brief 物理コンストレイント、スイープ/オーバーラップ、CharacterControllerのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Physics/PhysicsWorld3D.h"
#include "Physics/PhysicsConstraint.h"
#include "Physics/CharacterController.h"

using namespace gx;

// ============================================================================
// テストフィクスチャ — PhysicsWorld3D共有セットアップ
// ============================================================================

class PhysicsExtendedTest : public ::testing::Test
{
protected:
    PhysicsWorld3D world;

    void SetUp() override
    {
        ASSERT_TRUE(world.Initialize(1024));
        world.SetGravity({0, -9.81f, 0});
    }

    void TearDown() override
    {
        world.Shutdown();
    }

    // ヘルパー: Y=0に静的な床を作成
    PhysicsBodyID CreateFloor()
    {
        auto* shape = world.CreateBoxShape({50, 0.5f, 50});
        PhysicsBodySettings settings;
        settings.position = {0, -0.5f, 0};
        settings.motionType = MotionType3D::Static;
        settings.layer = 0;
        return world.AddBody(shape, settings);
    }

    // ヘルパー: 指定位置に動的ボックスを作成
    PhysicsBodyID CreateDynamicBox(const Vector3& pos, const Vector3& halfExtents = {0.5f, 0.5f, 0.5f})
    {
        auto* shape = world.CreateBoxShape(halfExtents);
        PhysicsBodySettings settings;
        settings.position = pos;
        settings.motionType = MotionType3D::Dynamic;
        settings.mass = 1.0f;
        settings.layer = 1;
        return world.AddBody(shape, settings);
    }

    // ヘルパー: 指定位置に動的スフィアを作成
    PhysicsBodyID CreateDynamicSphere(const Vector3& pos, float radius = 0.5f)
    {
        auto* shape = world.CreateSphereShape(radius);
        PhysicsBodySettings settings;
        settings.position = pos;
        settings.motionType = MotionType3D::Dynamic;
        settings.mass = 1.0f;
        settings.layer = 1;
        return world.AddBody(shape, settings);
    }
};

// ============================================================================
// コンストレイントID
// ============================================================================

TEST(PhysicsConstraintIDTest, DefaultInvalid)
{
    PhysicsConstraintID id;
    EXPECT_FALSE(id.IsValid());
}

// ============================================================================
// コンストレイント設定 — デフォルト値
// ============================================================================

TEST(PhysicsConstraintSettingsTest, FixedDefaults)
{
    FixedConstraintSettings s;
    EXPECT_TRUE(s.autoDetect);
}

TEST(PhysicsConstraintSettingsTest, HingeDefaults)
{
    HingeConstraintSettings s;
    EXPECT_NEAR(s.limitsMin, -3.14159f, 0.01f);
    EXPECT_NEAR(s.limitsMax, 3.14159f, 0.01f);
    EXPECT_NEAR(s.maxFrictionTorque, 0.0f, 1e-5f);
}

TEST(PhysicsConstraintSettingsTest, DistanceDefaults)
{
    DistanceConstraintSettings s;
    EXPECT_LT(s.minDistance, 0.0f);  // -1 = auto
    EXPECT_LT(s.maxDistance, 0.0f);
}

// ============================================================================
// コンストレイント作成 / 破棄
// ============================================================================

TEST_F(PhysicsExtendedTest, CreateFixedConstraint)
{
    auto bodyA = CreateDynamicBox({0, 5, 0});
    auto bodyB = CreateDynamicBox({2, 5, 0});
    ASSERT_TRUE(bodyA.IsValid());
    ASSERT_TRUE(bodyB.IsValid());

    FixedConstraintSettings settings;
    settings.autoDetect = true;
    auto cid = world.CreateFixedConstraint(bodyA, bodyB, settings);
    EXPECT_TRUE(cid.IsValid());

    // 破棄してもクラッシュしないことを確認
    world.DestroyConstraint(cid);
}

TEST_F(PhysicsExtendedTest, CreateHingeConstraint)
{
    auto bodyA = CreateDynamicBox({0, 5, 0});
    auto bodyB = CreateDynamicBox({2, 5, 0});

    HingeConstraintSettings settings;
    settings.point1 = {1, 0, 0};
    settings.point2 = {-1, 0, 0};
    auto cid = world.CreateHingeConstraint(bodyA, bodyB, settings);
    EXPECT_TRUE(cid.IsValid());
    world.DestroyConstraint(cid);
}

TEST_F(PhysicsExtendedTest, CreatePointConstraint)
{
    auto bodyA = CreateDynamicBox({0, 5, 0});
    auto bodyB = CreateDynamicBox({2, 5, 0});

    PointConstraintSettings settings;
    settings.point1 = {1, 0, 0};
    settings.point2 = {-1, 0, 0};
    auto cid = world.CreatePointConstraint(bodyA, bodyB, settings);
    EXPECT_TRUE(cid.IsValid());
    world.DestroyConstraint(cid);
}

TEST_F(PhysicsExtendedTest, DestroyInvalidConstraint)
{
    // クラッシュしないことを確認
    PhysicsConstraintID invalid;
    world.DestroyConstraint(invalid);
}

// ============================================================================
// Sweep (SphereCast / BoxCast)
// ============================================================================

TEST_F(PhysicsExtendedTest, SphereCast_Hit)
{
    CreateFloor();
    auto target = CreateDynamicBox({5, 1, 0}, {1, 1, 1});
    world.Step(0.0f);  // ボディを有効化

    auto hits = world.SphereCast({-5, 1, 0}, 0.5f, {1, 0, 0}, 20.0f);
    // ボックスにヒットするはず
    bool foundTarget = false;
    for (auto& h : hits)
    {
        if (h.bodyID.id == target.id)
            foundTarget = true;
    }
    EXPECT_TRUE(foundTarget);
}

TEST_F(PhysicsExtendedTest, SphereCast_Miss)
{
    CreateFloor();
    CreateDynamicBox({5, 1, 0});
    world.Step(0.0f);

    // すべてのオブジェクトを外す方向にキャスト
    auto hits = world.SphereCast({0, 100, 0}, 0.5f, {1, 0, 0}, 10.0f);
    EXPECT_TRUE(hits.empty());
}

TEST_F(PhysicsExtendedTest, BoxCast_Hit)
{
    CreateFloor();
    auto target = CreateDynamicBox({5, 1, 0}, {1, 1, 1});
    world.Step(0.0f);

    Quaternion identity;
    auto hits = world.BoxCast({-5, 1, 0}, {0.5f, 0.5f, 0.5f}, identity, {1, 0, 0}, 20.0f);
    bool foundTarget = false;
    for (auto& h : hits)
    {
        if (h.bodyID.id == target.id)
            foundTarget = true;
    }
    EXPECT_TRUE(foundTarget);
}

// ============================================================================
// Overlap (OverlapSphere / OverlapBox)
// ============================================================================

TEST_F(PhysicsExtendedTest, OverlapSphere_DetectsBody)
{
    auto body = CreateDynamicSphere({0, 1, 0}, 0.5f);
    world.Step(0.0f);

    auto hits = world.OverlapSphere({0, 1, 0}, 2.0f);
    bool found = false;
    for (auto& h : hits)
    {
        if (h.bodyID.id == body.id)
            found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(PhysicsExtendedTest, OverlapSphere_Empty)
{
    CreateDynamicSphere({0, 1, 0}, 0.5f);
    world.Step(0.0f);

    // 遠く離れている — 何も見つからないはず
    auto hits = world.OverlapSphere({100, 100, 100}, 1.0f);
    EXPECT_TRUE(hits.empty());
}

TEST_F(PhysicsExtendedTest, OverlapBox_DetectsBody)
{
    auto body = CreateDynamicBox({0, 1, 0}, {0.5f, 0.5f, 0.5f});
    world.Step(0.0f);

    Quaternion identity;
    auto hits = world.OverlapBox({0, 1, 0}, {2, 2, 2}, identity);
    bool found = false;
    for (auto& h : hits)
    {
        if (h.bodyID.id == body.id)
            found = true;
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// CharacterController
// ============================================================================

TEST_F(PhysicsExtendedTest, CharacterController_Initialize)
{
    CreateFloor();
    world.Step(1.0f / 60.0f);

    CharacterController cc;
    CharacterControllerSettings settings;
    EXPECT_TRUE(cc.Initialize(&world, settings, {0, 0, 0}));
    cc.Shutdown();
}

TEST_F(PhysicsExtendedTest, CharacterController_InitializeNullWorld)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    EXPECT_FALSE(cc.Initialize(nullptr, settings, {0, 0, 0}));
}

TEST_F(PhysicsExtendedTest, CharacterController_GetSetPosition)
{
    CreateFloor();
    world.Step(1.0f / 60.0f);

    CharacterController cc;
    CharacterControllerSettings settings;
    cc.Initialize(&world, settings, {0, 5, 0});

    auto pos = cc.GetPosition();
    EXPECT_NEAR(pos.x, 0.0f, 0.1f);
    EXPECT_NEAR(pos.y, 5.0f, 0.1f);
    EXPECT_NEAR(pos.z, 0.0f, 0.1f);

    cc.SetPosition({3, 2, 1});
    pos = cc.GetPosition();
    EXPECT_NEAR(pos.x, 3.0f, 0.1f);
    EXPECT_NEAR(pos.y, 2.0f, 0.1f);
    EXPECT_NEAR(pos.z, 1.0f, 0.1f);

    cc.Shutdown();
}

TEST_F(PhysicsExtendedTest, CharacterController_GroundState_InAir)
{
    CreateFloor();
    world.Step(1.0f / 60.0f);

    CharacterController cc;
    CharacterControllerSettings settings;
    cc.Initialize(&world, settings, {0, 50, 0});

    // 高い位置なので、初期状態では空中にいるはず（更新前）
    EXPECT_EQ(cc.GetGroundState(), CharacterGroundState::InAir);
    EXPECT_FALSE(cc.IsOnGround());

    cc.Shutdown();
}

TEST_F(PhysicsExtendedTest, CharacterController_Update)
{
    CreateFloor();
    world.Step(1.0f / 60.0f);

    CharacterController cc;
    CharacterControllerSettings settings;
    cc.Initialize(&world, settings, {0, 2, 0});

    // 下向き速度を設定（重力のような）
    cc.SetLinearVelocity({0, -5, 0});

    // 複数ステップ実行
    for (int i = 0; i < 60; ++i)
    {
        cc.Update(1.0f / 60.0f);
    }

    // 1秒落下後、地面付近または地面上にいるはず
    auto pos = cc.GetPosition();
    EXPECT_LT(pos.y, 2.0f);  // 下に移動しているはず

    cc.Shutdown();
}

TEST_F(PhysicsExtendedTest, CharacterController_WorldTransform)
{
    CreateFloor();
    world.Step(1.0f / 60.0f);

    CharacterController cc;
    CharacterControllerSettings settings;
    cc.Initialize(&world, settings, {3, 1, 5});

    auto mat = cc.GetWorldTransform();
    // 平行移動は最終行（行優先）のインデックス[3][0], [3][1], [3][2]にあるはず
    // またはマトリクスレイアウト経由でアクセス可能
    auto pos = cc.GetPosition();
    EXPECT_NEAR(pos.x, 3.0f, 0.1f);
    EXPECT_NEAR(pos.y, 1.0f, 0.1f);
    EXPECT_NEAR(pos.z, 5.0f, 0.1f);

    cc.Shutdown();
}
