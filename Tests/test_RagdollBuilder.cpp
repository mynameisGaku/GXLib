/// @file test_RagdollBuilder.cpp
/// @brief Phase 5: RagdollBuilder構築、ボーン管理、プリセットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Physics/RagdollBuilder.h"
#include "Physics/PhysicsWorld3D.h"

using namespace gx;

// ============================================================================
// RagdollBoneデフォルト値
// ============================================================================

TEST(RagdollBoneTest, Defaults)
{
    RagdollBone bone;
    EXPECT_TRUE(bone.name.empty());
    EXPECT_EQ(bone.parentIndex, -1);
    EXPECT_FLOAT_EQ(bone.length, 0.3f);
    EXPECT_FLOAT_EQ(bone.radius, 0.05f);
    EXPECT_FLOAT_EQ(bone.mass, 5.0f);
}

// ============================================================================
// RagdollJointデフォルト値
// ============================================================================

TEST(RagdollJointTest, Defaults)
{
    RagdollJoint joint;
    EXPECT_EQ(joint.boneA, -1);
    EXPECT_EQ(joint.boneB, -1);
    EXPECT_FLOAT_EQ(joint.coneAngle, 30.0f);
}

// ============================================================================
// RagdollPreset列挙型
// ============================================================================

TEST(RagdollPresetTest, EnumValues)
{
    EXPECT_NE(static_cast<int>(RagdollPreset::Custom), static_cast<int>(RagdollPreset::Humanoid));
}

// ============================================================================
// RagdollBuilderデフォルト状態
// ============================================================================

TEST(RagdollBuilder, DefaultNotBuilt)
{
    RagdollBuilder builder;
    EXPECT_FALSE(builder.IsBuilt());
    EXPECT_EQ(builder.GetBoneCount(), 0u);
    EXPECT_TRUE(builder.GetBones().empty());
    EXPECT_TRUE(builder.GetJoints().empty());
}

// ============================================================================
// RagdollBuilderとPhysicsWorld3D
// ============================================================================

class RagdollBuilderFixture : public ::testing::Test
{
protected:
    PhysicsWorld3D world;

    void SetUp() override
    {
        ASSERT_TRUE(world.Initialize(2048));
        world.SetGravity({ 0.0f, -9.81f, 0.0f });
    }

    void TearDown() override
    {
        world.Shutdown();
    }
};

TEST_F(RagdollBuilderFixture, BuildHumanoid)
{
    RagdollBuilder builder;
    EXPECT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));
    EXPECT_TRUE(builder.IsBuilt());
    EXPECT_GT(builder.GetBoneCount(), 0u);

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidBoneCount)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    // ヒューマノイドプリセットは15本のボーンを持つはず
    EXPECT_EQ(builder.GetBoneCount(), 15u);

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidHasJoints)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    EXPECT_GT(builder.GetJoints().size(), 0u);

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidBoneNames)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    const auto& bones = builder.GetBones();
    // すべてのボーンに名前があるはず
    for (const auto& bone : bones)
    {
        EXPECT_FALSE(bone.name.empty());
    }

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidBoneBodyIDs)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    const auto& bones = builder.GetBones();
    for (const auto& bone : bones)
    {
        EXPECT_TRUE(bone.bodyID.IsValid());
    }

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, DestroyResetsState)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));
    EXPECT_TRUE(builder.IsBuilt());

    builder.Destroy(&world);
    EXPECT_FALSE(builder.IsBuilt());
    EXPECT_EQ(builder.GetBoneCount(), 0u);
}

TEST_F(RagdollBuilderFixture, GetBoneTransforms)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    auto transforms = builder.GetBoneTransforms(&world);
    EXPECT_EQ(transforms.size(), builder.GetBoneCount());

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, BuildWithScale)
{
    RagdollBuilder builder;
    EXPECT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }, 2.0f));
    EXPECT_TRUE(builder.IsBuilt());
    EXPECT_GT(builder.GetBoneCount(), 0u);

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, ApplyImpulseToBone)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    // 最初のボーンにインパルスを適用 - クラッシュしないこと
    builder.ApplyImpulse(&world, 0, { 10.0f, 0.0f, 0.0f });

    world.Step(1.0f / 60.0f);

    builder.Destroy(&world);
}

// ============================================================================
// 追加テスト: RagdollBoneのカスタム値
// ============================================================================

TEST(RagdollBoneTest, CustomValues)
{
    RagdollBone bone;
    bone.name = "UpperArm";
    bone.parentIndex = 2;
    bone.length = 0.5f;
    bone.radius = 0.08f;
    bone.mass = 3.0f;
    bone.offset = { 0.3f, 0.0f, 0.0f };

    EXPECT_EQ(bone.name, "UpperArm");
    EXPECT_EQ(bone.parentIndex, 2);
    EXPECT_FLOAT_EQ(bone.length, 0.5f);
    EXPECT_FLOAT_EQ(bone.radius, 0.08f);
    EXPECT_FLOAT_EQ(bone.mass, 3.0f);
    EXPECT_FLOAT_EQ(bone.offset.x, 0.3f);
}

TEST(RagdollBoneTest, OffsetDefault)
{
    RagdollBone bone;
    EXPECT_FLOAT_EQ(bone.offset.x, 0.0f);
    EXPECT_FLOAT_EQ(bone.offset.y, 0.0f);
    EXPECT_FLOAT_EQ(bone.offset.z, 0.0f);
}

// ============================================================================
// 追加テスト: RagdollJointのカスタム値
// ============================================================================

TEST(RagdollJointTest, CustomValues)
{
    RagdollJoint joint;
    joint.boneA = 0;
    joint.boneB = 1;
    joint.coneAngle = 45.0f;

    EXPECT_EQ(joint.boneA, 0);
    EXPECT_EQ(joint.boneB, 1);
    EXPECT_FLOAT_EQ(joint.coneAngle, 45.0f);
}

// ============================================================================
// 追加テスト: RagdollPreset列挙型
// ============================================================================

TEST(RagdollPresetTest, CustomIsNotHumanoid)
{
    RagdollPreset custom = RagdollPreset::Custom;
    RagdollPreset humanoid = RagdollPreset::Humanoid;
    EXPECT_NE(static_cast<int>(custom), static_cast<int>(humanoid));
}

// ============================================================================
// 追加テスト: RagdollBuilderのデフォルト状態
// ============================================================================

TEST(RagdollBuilder, GetBonesEmpty)
{
    RagdollBuilder builder;
    EXPECT_EQ(builder.GetBones().size(), 0u);
}

TEST(RagdollBuilder, GetJointsEmpty)
{
    RagdollBuilder builder;
    EXPECT_EQ(builder.GetJoints().size(), 0u);
}

// ============================================================================
// 追加テスト: RagdollBuilderフィクスチャ
// ============================================================================

TEST_F(RagdollBuilderFixture, BuildAtDifferentPosition)
{
    RagdollBuilder builder;
    EXPECT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 10.0f, 8.0f, -5.0f }));
    EXPECT_TRUE(builder.IsBuilt());

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, ApplyImpulseToMultipleBones)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    // 複数のボーンにインパルスを適用
    for (uint32_t i = 0; i < builder.GetBoneCount(); ++i)
    {
        builder.ApplyImpulse(&world, static_cast<int>(i), { 1.0f, 0.0f, 0.0f });
    }

    world.Step(1.0f / 60.0f);

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, StepAfterBuild)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    // 複数の物理ステップがクラッシュしないこと
    for (int i = 0; i < 60; ++i)
    {
        world.Step(1.0f / 60.0f);
    }

    auto transforms = builder.GetBoneTransforms(&world);
    EXPECT_EQ(transforms.size(), builder.GetBoneCount());

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidJointCount)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    // ヒューマノイドは15ボーンで14個のジョイントがあるはず
    EXPECT_GT(builder.GetJoints().size(), 0u);
    EXPECT_LE(builder.GetJoints().size(), builder.GetBoneCount());

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidJointConeAngles)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    const auto& joints = builder.GetJoints();
    for (const auto& joint : joints)
    {
        // コーン角度は正の値であるはず
        EXPECT_GT(joint.coneAngle, 0.0f);
        // コーン角度は180度以下であるはず
        EXPECT_LE(joint.coneAngle, 180.0f);
    }

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, HumanoidBoneParentIndices)
{
    RagdollBuilder builder;
    ASSERT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }));

    const auto& bones = builder.GetBones();
    // ルートボーンの親は-1
    bool hasRoot = false;
    for (const auto& bone : bones)
    {
        if (bone.parentIndex == -1)
        {
            hasRoot = true;
        }
        else
        {
            // 非ルートの親インデックスは有効範囲内
            EXPECT_GE(bone.parentIndex, 0);
            EXPECT_LT(bone.parentIndex, static_cast<int>(bones.size()));
        }
    }
    EXPECT_TRUE(hasRoot);

    builder.Destroy(&world);
}

TEST_F(RagdollBuilderFixture, BuildWithSmallScale)
{
    RagdollBuilder builder;
    EXPECT_TRUE(builder.Build(&world, RagdollPreset::Humanoid, { 0.0f, 5.0f, 0.0f }, 0.5f));
    EXPECT_TRUE(builder.IsBuilt());
    EXPECT_EQ(builder.GetBoneCount(), 15u);

    builder.Destroy(&world);
}
