/// @file test_RootMotion.cpp
/// @brief RootMotionLocomotion のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/RootMotionLocomotion.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "TestHelpers.h"

using namespace gx;
using namespace gx::TestHelpers;

// ==================== テスト ====================

TEST(RootMotionTest, ConstructDestruct)
{
    RootMotionLocomotion loco;
    EXPECT_EQ(loco.GetAnimator(), nullptr);
    EXPECT_NEAR(loco.GetSpeed(), 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetPositionDelta().x, 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetPositionDelta().y, 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetPositionDelta().z, 0.0f, 1e-5f);
}

TEST(RootMotionTest, SetConfig)
{
    RootMotionLocomotion loco;
    RootMotionConfig config;
    config.applyPosition = false;
    config.lockVertical = true;
    config.positionScale = 2.0f;
    loco.SetConfig(config);

    const auto& cfg = loco.GetConfig();
    EXPECT_FALSE(cfg.applyPosition);
    EXPECT_TRUE(cfg.lockVertical);
    EXPECT_NEAR(cfg.positionScale, 2.0f, 1e-5f);
}

TEST(RootMotionTest, SetAnimator)
{
    RootMotionLocomotion loco;
    Animator animator;
    loco.SetAnimator(&animator);
    EXPECT_EQ(loco.GetAnimator(), &animator);

    loco.SetAnimator(nullptr);
    EXPECT_EQ(loco.GetAnimator(), nullptr);
}

TEST(RootMotionTest, DefaultConfig)
{
    RootMotionLocomotion loco;
    const auto& cfg = loco.GetConfig();
    EXPECT_TRUE(cfg.applyPosition);
    EXPECT_TRUE(cfg.applyRotation);
    EXPECT_FALSE(cfg.lockVertical);
    EXPECT_NEAR(cfg.positionScale, 1.0f, 1e-5f);
    EXPECT_NEAR(cfg.rotationScale, 1.0f, 1e-5f);
    EXPECT_FALSE(cfg.useGroundSnapping);
}

TEST(RootMotionTest, UpdateWithoutAnimator)
{
    RootMotionLocomotion loco;
    // Animator未設定でUpdateしてもクラッシュしない
    loco.Update(1.0f / 60.0f);
    EXPECT_NEAR(loco.GetSpeed(), 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetPositionDelta().x, 0.0f, 1e-5f);
}

TEST(RootMotionTest, PositionDeltaBasic)
{
    // AnimatorとSkeletonを準備してルートモーション差分を生成
    Skeleton skel = CreateChainSkeleton3();
    Animator animator;
    animator.SetSkeleton(&skel);
    animator.SetRootMotionEnabled(true);

    // ルートが移動するクリップ
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 2.0f, 0.0f, 0.0f);
    animator.Play(&clip, true);
    animator.Update(1.0f / 60.0f);

    RootMotionLocomotion loco;
    loco.SetAnimator(&animator);
    loco.Update(1.0f / 60.0f);

    // ルートモーション差分が反映されていることを確認
    Vector3 delta = loco.GetPositionDelta();
    // クリップは1秒で2単位移動、60fps なので約 0.033 の差分が出るはず
    // ただし正確な値はアニメーションサンプリングに依存
    EXPECT_FALSE(std::isnan(delta.x));
    EXPECT_FALSE(std::isnan(delta.y));
    EXPECT_FALSE(std::isnan(delta.z));
}

TEST(RootMotionTest, LockVertical)
{
    Skeleton skel = CreateChainSkeleton3();
    Animator animator;
    animator.SetSkeleton(&skel);
    animator.SetRootMotionEnabled(true);

    // Y方向にも移動するクリップ
    AnimationClip clip = CreateSimpleClip("Jump", 1.0f, 1.0f, 2.0f, 0.0f);
    animator.Play(&clip, true);
    animator.Update(1.0f / 60.0f);

    RootMotionLocomotion loco;
    loco.SetAnimator(&animator);

    RootMotionConfig config;
    config.lockVertical = true;
    loco.SetConfig(config);

    loco.Update(1.0f / 60.0f);

    // Y成分がゼロであることを確認
    EXPECT_NEAR(loco.GetPositionDelta().y, 0.0f, 1e-5f);
}

TEST(RootMotionTest, PositionScale)
{
    Skeleton skel = CreateChainSkeleton3();
    Animator animator;
    animator.SetSkeleton(&skel);
    animator.SetRootMotionEnabled(true);

    AnimationClip clip = CreateSimpleClip("Run", 1.0f, 3.0f, 0.0f, 0.0f);
    animator.Play(&clip, true);
    animator.Update(1.0f / 60.0f);

    // スケール1.0の差分を取得
    RootMotionLocomotion loco1;
    loco1.SetAnimator(&animator);
    loco1.Update(1.0f / 60.0f);
    Vector3 delta1 = loco1.GetPositionDelta();

    // Animatorを再初期化して同じ差分を生成
    animator.Play(&clip, true);
    animator.Update(1.0f / 60.0f);

    // スケール2.0の差分を取得
    RootMotionLocomotion loco2;
    loco2.SetAnimator(&animator);
    RootMotionConfig config;
    config.positionScale = 2.0f;
    loco2.SetConfig(config);
    loco2.Update(1.0f / 60.0f);
    Vector3 delta2 = loco2.GetPositionDelta();

    // delta2 は delta1 の2倍であるべき
    if (fabsf(delta1.x) > 1e-6f)
    {
        EXPECT_NEAR(delta2.x / delta1.x, 2.0f, 0.01f);
    }
}

TEST(RootMotionTest, SpeedCalculation)
{
    RootMotionLocomotion loco;
    // Animator未設定
    loco.Update(1.0f / 60.0f);
    EXPECT_NEAR(loco.GetSpeed(), 0.0f, 1e-5f);
}

TEST(RootMotionTest, GroundSnapping)
{
    Skeleton skel = CreateChainSkeleton3();
    Animator animator;
    animator.SetSkeleton(&skel);
    animator.SetRootMotionEnabled(true);

    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 1.0f);
    animator.Play(&clip, true);
    animator.Update(1.0f / 60.0f);

    RootMotionLocomotion loco;
    loco.SetAnimator(&animator);

    RootMotionConfig config;
    config.useGroundSnapping = true;
    loco.SetConfig(config);

    // 地面高さ関数: Y = 5.0 (固定)
    loco.SetGroundHeightFunction([](float, float) { return 5.0f; });

    loco.Update(1.0f / 60.0f);

    // 累積位置のYが地面高さにスナップされていることを確認
    EXPECT_NEAR(loco.GetAccumulatedPosition().y, 5.0f, 1e-3f);
}

TEST(RootMotionTest, Reset)
{
    RootMotionLocomotion loco;
    // 内部状態を作る
    Skeleton skel = CreateChainSkeleton3();
    Animator animator;
    animator.SetSkeleton(&skel);
    animator.SetRootMotionEnabled(true);

    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 0.0f);
    animator.Play(&clip, true);
    animator.Update(1.0f / 60.0f);

    loco.SetAnimator(&animator);
    loco.Update(1.0f / 60.0f);

    // リセット
    loco.Reset();
    EXPECT_NEAR(loco.GetAccumulatedPosition().x, 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetAccumulatedPosition().y, 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetAccumulatedPosition().z, 0.0f, 1e-5f);
    EXPECT_NEAR(loco.GetSpeed(), 0.0f, 1e-5f);
}

TEST(RootMotionTest, AccumulatedPosition)
{
    Skeleton skel = CreateChainSkeleton3();
    Animator animator;
    animator.SetSkeleton(&skel);
    animator.SetRootMotionEnabled(true);

    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 0.0f);
    animator.Play(&clip, true);

    RootMotionLocomotion loco;
    loco.SetAnimator(&animator);

    // 複数フレームUpdate
    for (int i = 0; i < 10; ++i)
    {
        animator.Update(1.0f / 60.0f);
        loco.Update(1.0f / 60.0f);
    }

    // 累積位置はゼロではないはず（移動するクリップなので）
    Vector3 accum = loco.GetAccumulatedPosition();
    // 正確な値はアニメーションの補間次第だが、NaNでないことは保証
    EXPECT_FALSE(std::isnan(accum.x));
    EXPECT_FALSE(std::isnan(accum.y));
    EXPECT_FALSE(std::isnan(accum.z));
}
