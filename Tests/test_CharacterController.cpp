/// @file test_CharacterController.cpp
/// @brief Phase 5: CharacterControllerの設定、初期化、移動API

#include "pch.h"
#include <gtest/gtest.h>
#include "Physics/CharacterController.h"
#include "Physics/PhysicsWorld3D.h"
#include <set>

using namespace gx;

// ============================================================================
// CharacterControllerSettingsデフォルト値
// ============================================================================

TEST(CharacterControllerSettings, Defaults)
{
    CharacterControllerSettings settings;
    EXPECT_FLOAT_EQ(settings.capsuleHalfHeight, 0.9f);
    EXPECT_FLOAT_EQ(settings.capsuleRadius, 0.3f);
    EXPECT_FLOAT_EQ(settings.mass, 70.0f);
    EXPECT_FLOAT_EQ(settings.maxSlopeAngle, 50.0f);
    EXPECT_FLOAT_EQ(settings.stepHeight, 0.4f);
    EXPECT_FLOAT_EQ(settings.stepDownHeight, 0.5f);
    EXPECT_FLOAT_EQ(settings.walkSpeed, 4.0f);
    EXPECT_FLOAT_EQ(settings.runSpeed, 8.0f);
    EXPECT_FLOAT_EQ(settings.jumpForce, 5.0f);
    EXPECT_FLOAT_EQ(settings.gravity, -9.81f);
    EXPECT_FLOAT_EQ(settings.skinWidth, 0.02f);
    EXPECT_EQ(settings.layer, 1u);
}

TEST(CharacterControllerSettings, CustomValues)
{
    CharacterControllerSettings settings;
    settings.capsuleHalfHeight = 1.2f;
    settings.capsuleRadius = 0.5f;
    settings.mass = 80.0f;
    settings.walkSpeed = 5.0f;
    settings.runSpeed = 12.0f;
    settings.jumpForce = 8.0f;
    settings.gravity = -15.0f;

    EXPECT_FLOAT_EQ(settings.capsuleHalfHeight, 1.2f);
    EXPECT_FLOAT_EQ(settings.capsuleRadius, 0.5f);
    EXPECT_FLOAT_EQ(settings.mass, 80.0f);
    EXPECT_FLOAT_EQ(settings.walkSpeed, 5.0f);
    EXPECT_FLOAT_EQ(settings.runSpeed, 12.0f);
    EXPECT_FLOAT_EQ(settings.jumpForce, 8.0f);
    EXPECT_FLOAT_EQ(settings.gravity, -15.0f);
}

// ============================================================================
// GroundState列挙型
// ============================================================================

TEST(CharacterControllerTest, GroundStateValues)
{
    // すべてのground state値が存在し、互いに異なることを検証
    EXPECT_NE(static_cast<int>(CharacterGroundState::OnGround),
              static_cast<int>(CharacterGroundState::InAir));
    EXPECT_NE(static_cast<int>(CharacterGroundState::OnSteepGround),
              static_cast<int>(CharacterGroundState::NotSupported));
    EXPECT_NE(static_cast<int>(CharacterGroundState::Sliding),
              static_cast<int>(CharacterGroundState::OnGround));

    // GroundStateエイリアスが動作することを検証
    GroundState state = CharacterGroundState::OnGround;
    EXPECT_EQ(state, CharacterGroundState::OnGround);
}

// ============================================================================
// CharacterControllerとPhysicsWorld3D
// ============================================================================

class CharacterControllerFixture : public ::testing::Test
{
protected:
    PhysicsWorld3D world;

    void SetUp() override
    {
        ASSERT_TRUE(world.Initialize(1024));
        world.SetGravity({ 0.0f, -9.81f, 0.0f });

        // 床を作成
        auto* floorShape = world.CreateBoxShape({ 50.0f, 0.5f, 50.0f });
        PhysicsBodySettings floorSettings;
        floorSettings.position = { 0.0f, -0.5f, 0.0f };
        floorSettings.motionType = MotionType3D::Static;
        floorSettings.layer = 0;
        world.AddBody(floorShape, floorSettings);
    }

    void TearDown() override
    {
        world.Shutdown();
    }
};

TEST_F(CharacterControllerFixture, Initialize)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    EXPECT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));
    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, GetPosition)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 5.0f, 2.0f, 3.0f }));

    Vector3 pos = cc.GetPosition();
    EXPECT_NEAR(pos.x, 5.0f, 0.1f);
    EXPECT_NEAR(pos.z, 3.0f, 0.1f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, SetPosition)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    cc.SetPosition({ 10.0f, 5.0f, 10.0f });
    Vector3 pos = cc.GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, 0.1f);
    EXPECT_NEAR(pos.y, 5.0f, 0.1f);
    EXPECT_NEAR(pos.z, 10.0f, 0.1f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, GetHeightAndRadius)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    settings.capsuleHalfHeight = 0.9f;
    settings.capsuleRadius = 0.3f;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    EXPECT_FLOAT_EQ(cc.GetHeight(), 0.9f);
    EXPECT_FLOAT_EQ(cc.GetRadius(), 0.3f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, SetWalkAndRunSpeed)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    cc.SetWalkSpeed(6.0f);
    cc.SetRunSpeed(14.0f);
    cc.SetJumpForce(10.0f);
    cc.SetGravity(-15.0f);

    // クラッシュしないこと
    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, SetMovementInput)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // 移動入力を設定して更新
    cc.SetMovementInput({ 1.0f, 0.0f, 0.0f });

    for (int i = 0; i < 10; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    // キャラクターは+X方向に移動しているはず
    Vector3 pos = cc.GetPosition();
    EXPECT_GT(pos.x, 0.0f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, SetRunning)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    cc.SetRunning(true);
    cc.SetMovementInput({ 0.0f, 0.0f, 1.0f });

    for (int i = 0; i < 10; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    // キャラクターは移動しているはず
    Vector3 pos = cc.GetPosition();
    EXPECT_GT(pos.z, 0.0f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, Jump)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // キャラクターを地面に着地させる
    for (int i = 0; i < 120; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    float yBeforeJump = cc.GetPosition().y;

    // ジャンプを試行
    cc.Jump();

    for (int i = 0; i < 10; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    float yAfterJump = cc.GetPosition().y;
    // ジャンプ後、キャラクターはより高い位置か同じ高さにあるはず
    EXPECT_GE(yAfterJump, yBeforeJump - 0.01f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, GetWorldTransform)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    Matrix4x4 transform = cc.GetWorldTransform();
    // 有効なトランスフォーム行列であるべき
    // おおまかに位置が含まれていることを確認
    EXPECT_NEAR(transform.m[3][0], 0.0f, 0.1f);
    EXPECT_NEAR(transform.m[3][2], 0.0f, 0.1f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, GetGroundNormal)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    Vector3 normal = cc.GetGroundNormal();
    // 着地前は{0,1,0}がデフォルトの可能性
    // 平面に着地後は約{0,1,0}であるべき
    EXPECT_GE(normal.y, 0.0f);

    cc.Shutdown();
}

// ============================================================================
// 追加テスト: 設定値の検証
// ============================================================================

TEST(CharacterControllerSettings, MaxSlopeAngle)
{
    CharacterControllerSettings settings;
    settings.maxSlopeAngle = 75.0f;
    EXPECT_FLOAT_EQ(settings.maxSlopeAngle, 75.0f);
}

TEST(CharacterControllerSettings, StepHeightValues)
{
    CharacterControllerSettings settings;
    settings.stepHeight = 0.8f;
    settings.stepDownHeight = 1.0f;
    EXPECT_FLOAT_EQ(settings.stepHeight, 0.8f);
    EXPECT_FLOAT_EQ(settings.stepDownHeight, 1.0f);
}

TEST(CharacterControllerSettings, SkinWidth)
{
    CharacterControllerSettings settings;
    settings.skinWidth = 0.05f;
    EXPECT_FLOAT_EQ(settings.skinWidth, 0.05f);
}

TEST(CharacterControllerSettings, LayerDefault)
{
    CharacterControllerSettings settings;
    EXPECT_EQ(settings.layer, 1u);
    settings.layer = 3;
    EXPECT_EQ(settings.layer, 3u);
}

// ============================================================================
// 追加テスト: GroundState列挙型の全値
// ============================================================================

TEST(CharacterControllerTest, AllGroundStatesDistinct)
{
    // すべてのground stateが互いに異なる値を持つことを検証
    std::set<int> values;
    values.insert(static_cast<int>(CharacterGroundState::OnGround));
    values.insert(static_cast<int>(CharacterGroundState::OnSteepGround));
    values.insert(static_cast<int>(CharacterGroundState::NotSupported));
    values.insert(static_cast<int>(CharacterGroundState::InAir));
    values.insert(static_cast<int>(CharacterGroundState::Sliding));
    EXPECT_EQ(values.size(), 5u);
}

// ============================================================================
// 追加テスト: CharacterController with PhysicsWorld3D
// ============================================================================

TEST_F(CharacterControllerFixture, InitializeAndShutdownMultipleTimes)
{
    CharacterController cc;
    CharacterControllerSettings settings;

    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));
    cc.Shutdown();

    // 再初期化が正常に動作すること
    ASSERT_TRUE(cc.Initialize(&world, settings, { 5.0f, 3.0f, 5.0f }));
    Vector3 pos = cc.GetPosition();
    EXPECT_NEAR(pos.x, 5.0f, 0.1f);
    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, GetRotation)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    Quaternion rot = cc.GetRotation();
    // デフォルトの回転は単位クォータニオンに近いはず
    EXPECT_NEAR(rot.w, 1.0f, 0.1f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, SetRotation)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // 90度Y軸回転のクォータニオン
    Quaternion rot = { 0.0f, 0.7071f, 0.0f, 0.7071f };
    cc.SetRotation(rot);

    // クラッシュしないことを確認
    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, GetLinearVelocity)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    Vector3 vel = cc.GetLinearVelocity();
    // 初期状態では速度はほぼゼロか重力による降下速度
    // クラッシュしないことを確認
    (void)vel;

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, SetLinearVelocity)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    cc.SetLinearVelocity({ 5.0f, 0.0f, 0.0f });
    // クラッシュしないことを確認
    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, IsOnGroundInitial)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // 初期位置が地面より上なので空中にいる可能性
    // この時点ではUpdate前なのでground stateが未確定の場合がある
    CharacterGroundState state = cc.GetGroundState();
    (void)state;  // クラッシュしないことを確認

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, FallToGround)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // 重力により地面に着地させる
    for (int i = 0; i < 120; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    // 地面付近にいるはず
    Vector3 pos = cc.GetPosition();
    EXPECT_LT(pos.y, 2.0f); // 初期位置より下がっている
    EXPECT_GT(pos.y, -1.0f); // 地面より下に突き抜けていない

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, MoveInZDirection)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // 着地させる
    for (int i = 0; i < 120; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    float zBefore = cc.GetPosition().z;
    cc.SetMovementInput({ 0.0f, 0.0f, 1.0f });

    for (int i = 0; i < 30; ++i)
    {
        world.Step(1.0f / 60.0f);
        cc.Update(1.0f / 60.0f);
    }

    float zAfter = cc.GetPosition().z;
    EXPECT_GT(zAfter, zBefore);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, UpdateWithZeroDeltaTime)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 2.0f, 0.0f }));

    // ゼロのデルタタイムでクラッシュしないこと
    cc.Update(0.0f);

    cc.Shutdown();
}

TEST_F(CharacterControllerFixture, CustomCapsuleDimensions)
{
    CharacterController cc;
    CharacterControllerSettings settings;
    settings.capsuleHalfHeight = 1.5f;
    settings.capsuleRadius = 0.5f;
    ASSERT_TRUE(cc.Initialize(&world, settings, { 0.0f, 3.0f, 0.0f }));

    EXPECT_FLOAT_EQ(cc.GetHeight(), 1.5f);
    EXPECT_FLOAT_EQ(cc.GetRadius(), 0.5f);

    cc.Shutdown();
}
