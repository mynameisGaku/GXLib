/// @file test_MotionMatching.cpp
/// @brief MotionDatabase / MotionMatcher のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/MotionMatching.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Skeleton.h"
#include "TestHelpers.h"

using namespace gx;
using namespace gx::TestHelpers;

// ==================== MotionDatabase ====================

TEST(MotionMatchingTest, DatabaseConstruct)
{
    MotionDatabase db;
    EXPECT_EQ(db.GetClipCount(), 0u);
    EXPECT_EQ(db.GetTotalPoseCount(), 0u);
}

TEST(MotionMatchingTest, DatabaseAddClip)
{
    MotionDatabase db;
    AnimationClip clip1 = CreateSimpleClip("Walk", 1.0f);
    AnimationClip clip2 = CreateSimpleClip("Run", 0.5f);

    db.AddClip(&clip1);
    EXPECT_EQ(db.GetClipCount(), 1u);

    db.AddClip(&clip2);
    EXPECT_EQ(db.GetClipCount(), 2u);

    // nullptr は追加されない
    db.AddClip(nullptr);
    EXPECT_EQ(db.GetClipCount(), 2u);
}

TEST(MotionMatchingTest, DatabaseBuild)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f);

    MotionDatabase db;
    db.AddClip(&clip);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f; // 10fps => 10 samples for 1 sec clip

    db.Build(skel, config);

    // 1秒クリップを10fpsでサンプル -> 約10ポーズ
    EXPECT_GT(db.GetTotalPoseCount(), 0u);
    EXPECT_LE(db.GetTotalPoseCount(), 11u); // 最大 floor(1.0 / 0.1) + 1 = 11
}

TEST(MotionMatchingTest, DatabaseFindBestMatch)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 2.0f, 0.0f, 0.0f);

    MotionDatabase db;
    db.AddClip(&clip);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f;
    db.Build(skel, config);

    ASSERT_GT(db.GetTotalPoseCount(), 0u);

    // クエリ: 右に移動する速度
    MotionFeature query;
    query.velocity = Vector3(2.0f, 0.0f, 0.0f);
    query.facingDirection = Vector3(0, 0, 1);

    float cost = 0.0f;
    MotionPose best = db.FindBestMatch(query, config, cost);

    EXPECT_EQ(best.clipIndex, 0);
    EXPECT_GE(best.time, 0.0f);
    EXPECT_GE(cost, 0.0f); // コストは非負
}

TEST(MotionMatchingTest, DatabaseTotalPoseCount)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip1 = CreateSimpleClip("Walk", 1.0f);
    AnimationClip clip2 = CreateSimpleClip("Run", 0.5f);

    MotionDatabase db;
    db.AddClip(&clip1);
    db.AddClip(&clip2);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f;
    db.Build(skel, config);

    // 1秒 + 0.5秒 = 1.5秒分のポーズ
    size_t totalPoses = db.GetTotalPoseCount();
    EXPECT_GT(totalPoses, 5u); // 少なくとも複数ポーズ
}

// ==================== MotionMatcher ====================

TEST(MotionMatchingTest, MatcherConstruct)
{
    MotionMatcher matcher;
    EXPECT_EQ(matcher.GetDatabase(), nullptr);
    EXPECT_EQ(matcher.GetCurrentClipIndex(), -1);
    EXPECT_NEAR(matcher.GetCurrentTime(), 0.0f, 1e-5f);
    EXPECT_NEAR(matcher.GetLastMatchCost(), 0.0f, 1e-5f);
}

TEST(MotionMatchingTest, MatcherSetDatabase)
{
    MotionMatcher matcher;
    MotionDatabase db;
    matcher.SetDatabase(&db);
    EXPECT_EQ(matcher.GetDatabase(), &db);

    matcher.SetDatabase(nullptr);
    EXPECT_EQ(matcher.GetDatabase(), nullptr);
}

TEST(MotionMatchingTest, MatcherSetConfig)
{
    MotionMatcher matcher;
    MotionMatchingConfig config;
    config.positionWeight = 2.0f;
    config.velocityWeight = 3.0f;
    config.minSwitchInterval = 0.5f;

    matcher.SetConfig(config);
    const auto& cfg = matcher.GetConfig();
    EXPECT_NEAR(cfg.positionWeight, 2.0f, 1e-5f);
    EXPECT_NEAR(cfg.velocityWeight, 3.0f, 1e-5f);
    EXPECT_NEAR(cfg.minSwitchInterval, 0.5f, 1e-5f);
}

TEST(MotionMatchingTest, MatcherUpdateBasic)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 0.0f);

    MotionDatabase db;
    db.AddClip(&clip);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f;
    config.minSwitchInterval = 0.0f; // 即座に切り替え可能
    db.Build(skel, config);

    Animator animator;
    animator.SetSkeleton(&skel);

    MotionMatcher matcher;
    matcher.SetDatabase(&db);
    matcher.SetConfig(config);
    matcher.SetAnimator(&animator);
    matcher.SetDesiredVelocity(Vector3(1.0f, 0.0f, 0.0f));

    matcher.Update(1.0f / 60.0f);

    // クリップが選択されているべき
    EXPECT_EQ(matcher.GetCurrentClipIndex(), 0);
}

TEST(MotionMatchingTest, MatcherDesiredVelocity)
{
    MotionMatcher matcher;
    matcher.SetDesiredVelocity(Vector3(3.0f, 0.0f, 2.0f));
    Vector3 vel = matcher.GetDesiredVelocity();
    EXPECT_NEAR(vel.x, 3.0f, 1e-5f);
    EXPECT_NEAR(vel.y, 0.0f, 1e-5f);
    EXPECT_NEAR(vel.z, 2.0f, 1e-5f);
}

TEST(MotionMatchingTest, MatcherDesiredFacing)
{
    MotionMatcher matcher;
    matcher.SetDesiredFacing(Vector3(0, 0, -1));
    Vector3 facing = matcher.GetDesiredFacing();
    EXPECT_NEAR(facing.x, 0.0f, 1e-5f);
    EXPECT_NEAR(facing.y, 0.0f, 1e-5f);
    EXPECT_NEAR(facing.z, -1.0f, 1e-5f);
}

TEST(MotionMatchingTest, MatcherMinSwitchInterval)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f);

    MotionDatabase db;
    db.AddClip(&clip);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f;
    config.minSwitchInterval = 1.0f; // 1秒間は切り替え不可
    db.Build(skel, config);

    MotionMatcher matcher;
    matcher.SetDatabase(&db);
    matcher.SetConfig(config);

    // 最初のUpdateでは timeSinceSwitch=0 < minSwitchInterval=1.0 なのでスキップ
    matcher.Update(0.01f);
    // クリップは切り替わっていないはず（初期値-1のまま）
    EXPECT_EQ(matcher.GetCurrentClipIndex(), -1);
}

TEST(MotionMatchingTest, MatcherCostTracking)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 0.0f);

    MotionDatabase db;
    db.AddClip(&clip);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f;
    config.minSwitchInterval = 0.0f;
    db.Build(skel, config);

    MotionMatcher matcher;
    matcher.SetDatabase(&db);
    matcher.SetConfig(config);
    matcher.SetDesiredVelocity(Vector3(1.0f, 0.0f, 0.0f));

    matcher.Update(1.0f / 60.0f);

    // コストは非負
    EXPECT_GE(matcher.GetLastMatchCost(), 0.0f);
}

TEST(MotionMatchingTest, FeatureWeights)
{
    Skeleton skel = CreateChainSkeleton3();
    AnimationClip clip = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 0.0f);

    MotionDatabase db;
    db.AddClip(&clip);

    // 速度重みのみ
    MotionMatchingConfig config1;
    config1.sampleRate = 10.0f;
    config1.velocityWeight = 10.0f;
    config1.positionWeight = 0.0f;
    config1.footWeight = 0.0f;
    config1.facingWeight = 0.0f;
    db.Build(skel, config1);

    MotionFeature query;
    query.velocity = Vector3(100.0f, 0.0f, 0.0f); // 非常に大きな速度

    float cost1 = 0.0f;
    db.FindBestMatch(query, config1, cost1);

    // 重み0の場合のコスト
    MotionMatchingConfig config2;
    config2.sampleRate = 10.0f;
    config2.velocityWeight = 0.0f;
    config2.positionWeight = 0.0f;
    config2.footWeight = 0.0f;
    config2.facingWeight = 0.0f;
    // データベースは再ビルドしなくてもよい（ポーズ自体は同じ）

    float cost2 = 0.0f;
    db.FindBestMatch(query, config2, cost2);

    // 全重みゼロならコストもゼロ
    EXPECT_NEAR(cost2, 0.0f, 1e-5f);
    // 速度重みありならコストは正
    EXPECT_GT(cost1, 0.0f);
}

TEST(MotionMatchingTest, EmptyDatabaseMatch)
{
    MotionDatabase db;
    // Build せずに FindBestMatch

    MotionFeature query;
    query.velocity = Vector3(1.0f, 0.0f, 0.0f);

    MotionMatchingConfig config;
    float cost = 0.0f;
    MotionPose best = db.FindBestMatch(query, config, cost);

    // 空データベースではFLT_MAXが返る
    EXPECT_EQ(best.clipIndex, -1);
    EXPECT_NEAR(cost, FLT_MAX, 1.0f);
}

TEST(MotionMatchingTest, MultiClipMatching)
{
    Skeleton skel = CreateChainSkeleton3();

    // 2つの異なるクリップ: 一方はX方向移動、もう一方はZ方向移動
    AnimationClip walkX = CreateSimpleClip("WalkX", 1.0f, 3.0f, 0.0f, 0.0f);
    AnimationClip walkZ = CreateSimpleClip("WalkZ", 1.0f, 0.0f, 0.0f, 3.0f);

    MotionDatabase db;
    db.AddClip(&walkX);
    db.AddClip(&walkZ);

    MotionMatchingConfig config;
    config.sampleRate = 10.0f;
    config.velocityWeight = 1.0f;
    config.positionWeight = 0.0f;
    config.footWeight = 0.0f;
    config.facingWeight = 0.0f;
    db.Build(skel, config);

    ASSERT_GT(db.GetTotalPoseCount(), 0u);

    // X方向の速度を求めるクエリ
    MotionFeature queryX;
    queryX.velocity = Vector3(3.0f, 0.0f, 0.0f);

    float costX = 0.0f;
    MotionPose bestX = db.FindBestMatch(queryX, config, costX);

    // Z方向の速度を求めるクエリ
    MotionFeature queryZ;
    queryZ.velocity = Vector3(0.0f, 0.0f, 3.0f);

    float costZ = 0.0f;
    MotionPose bestZ = db.FindBestMatch(queryZ, config, costZ);

    // X方向クエリはclip0 (WalkX) がマッチし、Z方向クエリはclip1 (WalkZ) がマッチするはず
    EXPECT_EQ(bestX.clipIndex, 0);
    EXPECT_EQ(bestZ.clipIndex, 1);
}
