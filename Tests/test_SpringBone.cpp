/// @file test_SpringBone.cpp
/// @brief SpringBone のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/SpringBone.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "TestHelpers.h"

using namespace gx;
using namespace gx::TestHelpers;

// ==================== ヘルパー ====================

/// @brief テスト用のローカルTRS姿勢とグローバル変換を準備する
static void PrepareTransforms(const Skeleton& skel,
                               std::vector<TransformTRS>& localTRS,
                               std::vector<DirectX::XMFLOAT4X4>& globalMats)
{
    uint32_t n = skel.GetJointCount();
    localTRS.resize(n);
    globalMats.resize(n);

    const auto& joints = skel.GetJoints();
    std::vector<XMFLOAT4X4> localMats(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        localTRS[i] = DecomposeTRS(joints[i].localTransform);
        localMats[i] = joints[i].localTransform;
    }
    skel.ComputeGlobalTransforms(localMats.data(), globalMats.data());
}

// ==================== テスト ====================

TEST(SpringBoneTest, ConstructDestruct)
{
    SpringBone sb;
    EXPECT_TRUE(sb.IsEnabled());
    EXPECT_EQ(sb.GetBoneCount(), 0u);
    EXPECT_EQ(sb.GetColliderCount(), 0u);
}

TEST(SpringBoneTest, AddBone)
{
    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 1;
    config.stiffness = 200.0f;
    sb.AddBone(config);
    EXPECT_EQ(sb.GetBoneCount(), 1u);

    config.boneIndex = 2;
    sb.AddBone(config);
    EXPECT_EQ(sb.GetBoneCount(), 2u);
}

TEST(SpringBoneTest, AddChain)
{
    SpringBone sb;
    sb.AddChain({1, 2}, 500.0f, 20.0f);
    EXPECT_EQ(sb.GetBoneCount(), 2u);
}

TEST(SpringBoneTest, AddCollider)
{
    SpringBone sb;
    SpringCollider collider;
    collider.type = SpringCollider::Sphere;
    collider.position = Vector3(0, 0, 0);
    collider.radius = 0.5f;
    sb.AddCollider(collider);
    EXPECT_EQ(sb.GetColliderCount(), 1u);

    SpringCollider capsule;
    capsule.type = SpringCollider::Capsule;
    capsule.position = Vector3(0, 0, 0);
    capsule.endPosition = Vector3(0, 1, 0);
    capsule.radius = 0.2f;
    sb.AddCollider(capsule);
    EXPECT_EQ(sb.GetColliderCount(), 2u);
}

TEST(SpringBoneTest, RemoveBone)
{
    SpringBone sb;
    SpringBoneConfig c1, c2;
    c1.boneIndex = 1;
    c2.boneIndex = 2;
    sb.AddBone(c1);
    sb.AddBone(c2);
    EXPECT_EQ(sb.GetBoneCount(), 2u);

    sb.RemoveBone(1);
    EXPECT_EQ(sb.GetBoneCount(), 1u);

    // 存在しないインデックスの除去は何もしない
    sb.RemoveBone(99);
    EXPECT_EQ(sb.GetBoneCount(), 1u);
}

TEST(SpringBoneTest, SetGravity)
{
    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 1;
    config.gravity = -9.81f;
    sb.AddBone(config);

    sb.SetGravity(-5.0f);
    // 内部設定が変更されたことはUpdate時の挙動で確認（直接アクセスは不可だが、
    // クラッシュしないことを検証）
    EXPECT_EQ(sb.GetBoneCount(), 1u);
}

TEST(SpringBoneTest, SetWindForce)
{
    SpringBone sb;
    sb.SetWindForce(Vector3(1.0f, 0.0f, 0.0f));
    // クラッシュしないことを検証
    EXPECT_TRUE(sb.IsEnabled());
}

TEST(SpringBoneTest, EnableDisable)
{
    SpringBone sb;
    EXPECT_TRUE(sb.IsEnabled());

    sb.SetEnabled(false);
    EXPECT_FALSE(sb.IsEnabled());

    sb.SetEnabled(true);
    EXPECT_TRUE(sb.IsEnabled());
}

TEST(SpringBoneTest, UpdateBasic)
{
    Skeleton skel = CreateChainSkeleton3();
    std::vector<TransformTRS> localTRS;
    std::vector<XMFLOAT4X4> globalMats;
    PrepareTransforms(skel, localTRS, globalMats);

    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 2; // Head
    config.stiffness = 300.0f;
    config.damping = 10.0f;
    sb.AddBone(config);

    // 初回Updateで初期化される
    sb.Update(1.0f / 60.0f, skel, localTRS, globalMats);

    // NaNが発生しないことを確認
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.x));
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.y));
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.z));
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.w));
}

TEST(SpringBoneTest, UpdateWithGravity)
{
    Skeleton skel = CreateChainSkeleton3();
    std::vector<TransformTRS> localTRS;
    std::vector<XMFLOAT4X4> globalMats;
    PrepareTransforms(skel, localTRS, globalMats);

    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 2; // Head
    config.stiffness = 100.0f;
    config.damping = 5.0f;
    config.gravity = -20.0f;
    sb.AddBone(config);

    // 複数フレーム更新
    for (int i = 0; i < 10; ++i)
        sb.Update(1.0f / 60.0f, skel, localTRS, globalMats);

    // NaNが発生しないことを確認
    EXPECT_FALSE(std::isnan(localTRS[2].translation.x));
    EXPECT_FALSE(std::isnan(localTRS[2].translation.y));
    EXPECT_FALSE(std::isnan(localTRS[2].translation.z));
}

TEST(SpringBoneTest, ColliderSphereBasic)
{
    Skeleton skel = CreateChainSkeleton3();
    std::vector<TransformTRS> localTRS;
    std::vector<XMFLOAT4X4> globalMats;
    PrepareTransforms(skel, localTRS, globalMats);

    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 2;
    config.stiffness = 100.0f;
    config.damping = 5.0f;
    config.radius = 0.1f;
    sb.AddBone(config);

    SpringCollider collider;
    collider.type = SpringCollider::Sphere;
    collider.position = Vector3(0, 2, 0); // Head付近
    collider.radius = 0.5f;
    sb.AddCollider(collider);

    // 複数フレーム更新してコライダーとの衝突処理がクラッシュしないことを確認
    for (int i = 0; i < 10; ++i)
        sb.Update(1.0f / 60.0f, skel, localTRS, globalMats);

    EXPECT_FALSE(std::isnan(localTRS[2].rotation.x));
}

TEST(SpringBoneTest, ColliderCapsuleBasic)
{
    Skeleton skel = CreateChainSkeleton3();
    std::vector<TransformTRS> localTRS;
    std::vector<XMFLOAT4X4> globalMats;
    PrepareTransforms(skel, localTRS, globalMats);

    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 2;
    config.stiffness = 100.0f;
    config.damping = 5.0f;
    config.radius = 0.1f;
    sb.AddBone(config);

    SpringCollider capsule;
    capsule.type = SpringCollider::Capsule;
    capsule.position = Vector3(-1, 1, 0);
    capsule.endPosition = Vector3(1, 1, 0);
    capsule.radius = 0.3f;
    sb.AddCollider(capsule);

    for (int i = 0; i < 10; ++i)
        sb.Update(1.0f / 60.0f, skel, localTRS, globalMats);

    EXPECT_FALSE(std::isnan(localTRS[2].rotation.w));
}

TEST(SpringBoneTest, ResetToInitial)
{
    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 1;
    sb.AddBone(config);

    // Reset後もボーン数は変わらない
    sb.Reset();
    EXPECT_EQ(sb.GetBoneCount(), 1u);
}

TEST(SpringBoneTest, ChainPropagation)
{
    Skeleton skel = CreateChainSkeleton3();
    std::vector<TransformTRS> localTRS;
    std::vector<XMFLOAT4X4> globalMats;
    PrepareTransforms(skel, localTRS, globalMats);

    SpringBone sb;
    // チェーンで Spine + Head を追加
    sb.AddChain({1, 2}, 200.0f, 10.0f);
    EXPECT_EQ(sb.GetBoneCount(), 2u);

    sb.SetWindForce(Vector3(2.0f, 0.0f, 0.0f));

    for (int i = 0; i < 20; ++i)
        sb.Update(1.0f / 60.0f, skel, localTRS, globalMats);

    // 全ボーンの回転がNaNでないことを検証
    for (uint32_t j = 0; j < skel.GetJointCount(); ++j)
    {
        EXPECT_FALSE(std::isnan(localTRS[j].rotation.x));
        EXPECT_FALSE(std::isnan(localTRS[j].rotation.y));
        EXPECT_FALSE(std::isnan(localTRS[j].rotation.z));
        EXPECT_FALSE(std::isnan(localTRS[j].rotation.w));
    }
}

TEST(SpringBoneTest, MaxAngleClamp)
{
    Skeleton skel = CreateChainSkeleton3();
    std::vector<TransformTRS> localTRS;
    std::vector<XMFLOAT4X4> globalMats;
    PrepareTransforms(skel, localTRS, globalMats);

    SpringBone sb;
    SpringBoneConfig config;
    config.boneIndex = 2;
    config.stiffness = 50.0f;
    config.damping = 2.0f;
    config.maxAngle = 10.0f; // 非常に小さい角度制限
    config.gravity = -30.0f; // 強い重力
    sb.AddBone(config);

    for (int i = 0; i < 30; ++i)
        sb.Update(1.0f / 60.0f, skel, localTRS, globalMats);

    // NaNが発生しないことを確認（角度制限処理のロバスト性）
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.x));
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.y));
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.z));
    EXPECT_FALSE(std::isnan(localTRS[2].rotation.w));
}
