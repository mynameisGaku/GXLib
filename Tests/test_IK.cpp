/// @file test_IK.cpp
/// @brief CCDIKSolver / LookAtIK のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/IKSolver.h"
#include "Graphics/3D/LookAtIK.h"
#include "TestHelpers.h"

using namespace gx;
using namespace gx::TestHelpers;

// ==================== IKChain ====================

TEST(IKTest, Chain_Defaults)
{
    IKChain chain;
    EXPECT_EQ(chain.effectorIndex, -1);
    EXPECT_NEAR(chain.tolerance, 0.001f, 1e-5f);
    EXPECT_EQ(chain.maxIterations, 20u);
    EXPECT_TRUE(chain.jointIndices.empty());
}

TEST(IKTest, Chain_SetValues)
{
    IKChain chain;
    chain.jointIndices = {0, 1, 2};
    chain.effectorIndex = 2;
    chain.tolerance = 0.01f;
    chain.maxIterations = 50;
    EXPECT_EQ(chain.jointIndices.size(), 3u);
    EXPECT_EQ(chain.effectorIndex, 2);
}

// ==================== CCDIKSolver::Solve ====================

TEST(IKTest, Solve_ReachableTarget)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();

    // スケルトンからローカル変換を初期化
    std::vector<XMFLOAT4X4> local(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;

    std::vector<XMFLOAT4X4> global(n);
    skel.ComputeGlobalTransforms(local.data(), global.data());

    IKChain chain;
    chain.jointIndices = {0, 1, 2};
    chain.effectorIndex = 2;
    chain.tolerance = 0.01f;
    chain.maxIterations = 30;

    // 元の位置に近いターゲット（容易に到達可能）
    XMFLOAT3 target = {0.5f, 1.5f, 0.0f};

    CCDIKSolver solver;
    bool converged = solver.Solve(chain, target, skel, local.data(), global.data());

    // エフェクターがターゲットに近いことを確認
    XMFLOAT3 effectorPos = {global[2]._41, global[2]._42, global[2]._43};
    float dist = sqrtf(
        (effectorPos.x - target.x) * (effectorPos.x - target.x) +
        (effectorPos.y - target.y) * (effectorPos.y - target.y) +
        (effectorPos.z - target.z) * (effectorPos.z - target.z));
    EXPECT_LT(dist, 0.5f);
}

TEST(IKTest, Solve_UnreachableTarget)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();

    std::vector<XMFLOAT4X4> local(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;

    std::vector<XMFLOAT4X4> global(n);
    skel.ComputeGlobalTransforms(local.data(), global.data());

    IKChain chain;
    chain.jointIndices = {0, 1, 2};
    chain.effectorIndex = 2;
    chain.tolerance = 0.001f;
    chain.maxIterations = 20;

    // 到達不可能な遠すぎるターゲット（チェーン長は約2単位）
    XMFLOAT3 target = {100.0f, 100.0f, 100.0f};

    CCDIKSolver solver;
    bool converged = solver.Solve(chain, target, skel, local.data(), global.data());
    // 収束しないがクラッシュもしないべき
    EXPECT_FALSE(converged);
}

TEST(IKTest, Solve_AlreadyAtTarget)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();

    std::vector<XMFLOAT4X4> local(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;

    std::vector<XMFLOAT4X4> global(n);
    skel.ComputeGlobalTransforms(local.data(), global.data());

    // ターゲットはエフェクターが既にいる位置と完全に一致
    XMFLOAT3 target = {global[2]._41, global[2]._42, global[2]._43};

    IKChain chain;
    chain.jointIndices = {0, 1, 2};
    chain.effectorIndex = 2;
    chain.tolerance = 0.01f;
    chain.maxIterations = 20;

    CCDIKSolver solver;
    bool converged = solver.Solve(chain, target, skel, local.data(), global.data());
    EXPECT_TRUE(converged);
}

TEST(IKTest, Solve_SingleJointChain)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();

    std::vector<XMFLOAT4X4> local(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;

    std::vector<XMFLOAT4X4> global(n);
    skel.ComputeGlobalTransforms(local.data(), global.data());

    IKChain chain;
    chain.jointIndices = {1};
    chain.effectorIndex = 2;
    chain.tolerance = 0.1f;
    chain.maxIterations = 30;

    XMFLOAT3 target = {0.5f, 1.5f, 0.0f};

    CCDIKSolver solver;
    // 単一ジョイントチェーンでもクラッシュしないべき
    solver.Solve(chain, target, skel, local.data(), global.data());
    EXPECT_FALSE(std::isnan(global[2]._41));
}

// ==================== LookAtIK ====================

TEST(LookAtIKTest, DefaultState)
{
    LookAtIK lookAt;
    EXPECT_TRUE(lookAt.IsEnabled());
    EXPECT_FALSE(lookAt.IsSetup());
    EXPECT_NEAR(lookAt.GetMaxAngle(), XM_PIDIV4, 1e-5f);
}

TEST(LookAtIKTest, SetEnabled)
{
    LookAtIK lookAt;
    lookAt.SetEnabled(false);
    EXPECT_FALSE(lookAt.IsEnabled());
    lookAt.SetEnabled(true);
    EXPECT_TRUE(lookAt.IsEnabled());
}

TEST(LookAtIKTest, SetMaxAngle)
{
    LookAtIK lookAt;
    lookAt.SetMaxAngle(XM_PIDIV2);
    EXPECT_NEAR(lookAt.GetMaxAngle(), XM_PIDIV2, 1e-5f);
}

TEST(LookAtIKTest, Setup_ValidJoints)
{
    Skeleton skel = CreateChainSkeleton3();
    LookAtIK lookAt;
    lookAt.Setup(skel, "Head", "Spine");
    EXPECT_TRUE(lookAt.IsSetup());
}

TEST(LookAtIKTest, Setup_HeadOnly)
{
    Skeleton skel = CreateChainSkeleton3();
    LookAtIK lookAt;
    lookAt.Setup(skel, "Head");
    EXPECT_TRUE(lookAt.IsSetup());
}

TEST(LookAtIKTest, Setup_InvalidJoint)
{
    Skeleton skel = CreateChainSkeleton3();
    LookAtIK lookAt;
    lookAt.Setup(skel, "NonExistent");
    EXPECT_FALSE(lookAt.IsSetup());
}

TEST(LookAtIKTest, Apply_NoSetup)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();
    std::vector<XMFLOAT4X4> local(n), global(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;
    skel.ComputeGlobalTransforms(local.data(), global.data());

    LookAtIK lookAt;
    Transform3D worldTf;
    // セットアップなしでもクラッシュしないべき
    lookAt.Apply(local.data(), global.data(), skel, worldTf,
                 XMFLOAT3{0, 0, 5}, 1.0f);
    EXPECT_FALSE(std::isnan(global[0]._41));
}

TEST(LookAtIKTest, Apply_WithSetup)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();
    std::vector<XMFLOAT4X4> local(n), global(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;
    skel.ComputeGlobalTransforms(local.data(), global.data());

    LookAtIK lookAt;
    lookAt.Setup(skel, "Head", "Spine");
    Transform3D worldTf;
    lookAt.Apply(local.data(), global.data(), skel, worldTf,
                 XMFLOAT3{5, 2, 0}, 1.0f);
    // NaNを生成しないべき
    EXPECT_FALSE(std::isnan(global[2]._41));
    EXPECT_FALSE(std::isnan(global[2]._42));
}

TEST(LookAtIKTest, Apply_WeightZero)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();
    std::vector<XMFLOAT4X4> local(n), global(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;
    skel.ComputeGlobalTransforms(local.data(), global.data());

    // 元の頭の変換を保存
    XMFLOAT4X4 originalHead = global[2];

    LookAtIK lookAt;
    lookAt.Setup(skel, "Head", "Spine");
    Transform3D worldTf;
    lookAt.Apply(local.data(), global.data(), skel, worldTf,
                 XMFLOAT3{100, 0, 0}, 0.0f);

    // weight=0では、頭は動かないべき
    EXPECT_NEAR(global[2]._41, originalHead._41, 0.01f);
    EXPECT_NEAR(global[2]._42, originalHead._42, 0.01f);
    EXPECT_NEAR(global[2]._43, originalHead._43, 0.01f);
}

TEST(LookAtIKTest, Apply_Disabled)
{
    Skeleton skel = CreateChainSkeleton3();
    uint32_t n = skel.GetJointCount();
    std::vector<XMFLOAT4X4> local(n), global(n);
    for (uint32_t i = 0; i < n; ++i)
        local[i] = skel.GetJoints()[i].localTransform;
    skel.ComputeGlobalTransforms(local.data(), global.data());

    XMFLOAT4X4 originalHead = global[2];

    LookAtIK lookAt;
    lookAt.Setup(skel, "Head");
    lookAt.SetEnabled(false);
    Transform3D worldTf;
    lookAt.Apply(local.data(), global.data(), skel, worldTf,
                 XMFLOAT3{100, 0, 0}, 1.0f);

    // 無効化: 頭は動かないべき
    EXPECT_NEAR(global[2]._41, originalHead._41, 0.01f);
    EXPECT_NEAR(global[2]._42, originalHead._42, 0.01f);
}
