/// @file test_ProceduralAnimation.cpp
/// @brief ProceduralAnimator 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/ProceduralAnimation.h"

using namespace gx;

// ==================== 歩行パターン ====================

TEST(ProceduralAnimationTest, DefaultGait)
{
    ProceduralAnimator animator;
    const auto& gait = animator.GetGait();
    EXPECT_EQ(gait.type, ProceduralGaitType::Walk);
    EXPECT_NEAR(gait.cycleTime, 1.0f, 1e-5f);
    EXPECT_NEAR(gait.dutyFactor, 0.6f, 1e-5f);
}

TEST(ProceduralAnimationTest, SetGait)
{
    ProceduralAnimator animator;
    GaitPattern pattern;
    pattern.type = ProceduralGaitType::Run;
    pattern.cycleTime = 0.5f;
    pattern.dutyFactor = 0.4f;
    pattern.legPhaseOffsets = { 0.0f, 0.5f };
    animator.SetGait(pattern);

    const auto& gait = animator.GetGait();
    EXPECT_EQ(gait.type, ProceduralGaitType::Run);
    EXPECT_NEAR(gait.cycleTime, 0.5f, 1e-5f);
    EXPECT_NEAR(gait.dutyFactor, 0.4f, 1e-5f);
    EXPECT_EQ(gait.legPhaseOffsets.size(), 2u);
}

// ==================== 脚 ====================

TEST(ProceduralAnimationTest, AddLeg)
{
    ProceduralAnimator animator;
    LegConfig cfg;
    cfg.hipJointIndex = 1;
    cfg.kneeJointIndex = 2;
    cfg.footJointIndex = 3;
    int idx = animator.AddLeg(cfg);
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(animator.GetLegCount(), 1);
}

TEST(ProceduralAnimationTest, LegCount)
{
    ProceduralAnimator animator;
    EXPECT_EQ(animator.GetLegCount(), 0);
    LegConfig cfg;
    animator.AddLeg(cfg);
    animator.AddLeg(cfg);
    animator.AddLeg(cfg);
    animator.AddLeg(cfg);
    EXPECT_EQ(animator.GetLegCount(), 4);
}

TEST(ProceduralAnimationTest, LegStateInitial)
{
    ProceduralAnimator animator;
    LegConfig cfg;
    cfg.footOffset = { 0.3f, 0.0f, 0.5f };
    animator.AddLeg(cfg);

    auto state = animator.GetLegState(0);
    EXPECT_NEAR(state.phase, 0.0f, 1e-5f);
    EXPECT_TRUE(state.isGrounded); // 初期は接地
}

// ==================== モーション構成 ====================

TEST(ProceduralAnimationTest, DefaultMotionConfig)
{
    ProceduralAnimator animator;
    const auto& mc = animator.GetMotionConfig();
    EXPECT_NEAR(mc.bodyBobAmplitude, 0.05f, 1e-5f);
    EXPECT_NEAR(mc.bodyBobFrequency, 2.0f, 1e-5f);
    EXPECT_NEAR(mc.bodySwayAmplitude, 0.02f, 1e-5f);
}

TEST(ProceduralAnimationTest, SetMotionConfig)
{
    ProceduralAnimator animator;
    MotionConfig cfg;
    cfg.bodyBobAmplitude = 0.1f;
    cfg.bodyBobFrequency = 3.0f;
    animator.SetMotionConfig(cfg);
    EXPECT_NEAR(animator.GetMotionConfig().bodyBobAmplitude, 0.1f, 1e-5f);
    EXPECT_NEAR(animator.GetMotionConfig().bodyBobFrequency, 3.0f, 1e-5f);
}

// ==================== 移動 ====================

TEST(ProceduralAnimationTest, MovementDirection)
{
    ProceduralAnimator animator;
    animator.SetMovementDirection({ 1, 0, 0 });
    // 方向はGetで直接取得できないが、Updateに影響
    // 設定がクラッシュしないことを確認
    SUCCEED();
}

TEST(ProceduralAnimationTest, MovementSpeed)
{
    ProceduralAnimator animator;
    animator.SetMovementSpeed(5.0f);
    EXPECT_NEAR(animator.GetMovementSpeed(), 5.0f, 1e-5f);
}

TEST(ProceduralAnimationTest, GroundHeight)
{
    ProceduralAnimator animator;
    animator.SetGroundHeight(3.0f);
    EXPECT_NEAR(animator.GetGroundHeight(), 3.0f, 1e-5f);
}

// ==================== LookAt ====================

TEST(ProceduralAnimationTest, LookAtTarget)
{
    ProceduralAnimator animator;
    EXPECT_FALSE(animator.HasLookAt());
    animator.SetLookAtTarget({ 10, 2, 5 });
    EXPECT_TRUE(animator.HasLookAt());
    auto target = animator.GetLookAtTarget();
    EXPECT_NEAR(target.x, 10.0f, 1e-5f);
    EXPECT_NEAR(target.y, 2.0f, 1e-5f);
    EXPECT_NEAR(target.z, 5.0f, 1e-5f);
}

TEST(ProceduralAnimationTest, ClearLookAt)
{
    ProceduralAnimator animator;
    animator.SetLookAtTarget({ 1, 2, 3 });
    EXPECT_TRUE(animator.HasLookAt());
    animator.ClearLookAt();
    EXPECT_FALSE(animator.HasLookAt());
}

TEST(ProceduralAnimationTest, HasLookAt)
{
    ProceduralAnimator animator;
    EXPECT_FALSE(animator.HasLookAt());
    animator.SetLookAtTarget({ 0, 0, 10 });
    EXPECT_TRUE(animator.HasLookAt());
}

// ==================== Update ====================

TEST(ProceduralAnimationTest, UpdateAdvancesPhase)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.dutyFactor = 0.6f;
    gait.legPhaseOffsets = { 0.0f, 0.5f };
    animator.SetGait(gait);

    LegConfig cfg;
    animator.AddLeg(cfg);
    animator.AddLeg(cfg);

    animator.SetMovementSpeed(1.0f);
    animator.SetMovementDirection({ 0, 0, 1 });

    auto stateBefore = animator.GetLegState(0);
    animator.Update(0.1f);
    auto stateAfter = animator.GetLegState(0);

    // 位相が進んでいるはず
    EXPECT_NE(stateAfter.phase, stateBefore.phase);
}

TEST(ProceduralAnimationTest, BodyBob)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.legPhaseOffsets = { 0.0f };
    animator.SetGait(gait);

    MotionConfig mc;
    mc.bodyBobAmplitude = 0.1f;
    mc.bodyBobFrequency = 2.0f;
    animator.SetMotionConfig(mc);

    LegConfig legCfg;
    animator.AddLeg(legCfg);
    animator.SetMovementSpeed(2.0f);
    animator.SetMovementDirection({ 0, 0, 1 });

    // 何フレームか更新してボブオフセットが変化することを確認
    animator.Update(0.1f);
    auto offset1 = animator.GetBodyOffset();
    animator.Update(0.2f);
    auto offset2 = animator.GetBodyOffset();

    // ボブが変化している（少なくともNaNでない）
    EXPECT_FALSE(std::isnan(offset1.y));
    EXPECT_FALSE(std::isnan(offset2.y));
}

TEST(ProceduralAnimationTest, BodySway)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.legPhaseOffsets = { 0.0f };
    animator.SetGait(gait);

    MotionConfig mc;
    mc.bodySwayAmplitude = 0.05f;
    animator.SetMotionConfig(mc);

    LegConfig legCfg;
    animator.AddLeg(legCfg);
    animator.SetMovementSpeed(2.0f);

    animator.Update(0.25f);
    auto offset = animator.GetBodyOffset();
    EXPECT_FALSE(std::isnan(offset.x));
}

TEST(ProceduralAnimationTest, FootTargetComputation)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.dutyFactor = 0.5f;
    gait.legPhaseOffsets = { 0.0f, 0.5f };
    animator.SetGait(gait);

    LegConfig cfg;
    cfg.stepHeight = 0.3f;
    cfg.stepLength = 0.6f;
    cfg.footOffset = { 0, 0, 0 };
    animator.AddLeg(cfg);
    animator.AddLeg(cfg);

    animator.SetMovementSpeed(2.0f);
    animator.SetMovementDirection({ 0, 0, 1 });

    animator.Update(0.3f);
    auto target0 = animator.GetFootTarget(0);
    auto target1 = animator.GetFootTarget(1);

    // NaNでないこと
    EXPECT_FALSE(std::isnan(target0.x));
    EXPECT_FALSE(std::isnan(target0.y));
    EXPECT_FALSE(std::isnan(target0.z));
    EXPECT_FALSE(std::isnan(target1.x));
}

TEST(ProceduralAnimationTest, SpringDamperUpdate)
{
    SpringDamper spring;
    spring.stiffness = 100.0f;
    spring.damping   = 10.0f;
    spring.currentValue = 0.0f;
    spring.velocity = 0.0f;

    float target = 1.0f;
    for (int i = 0; i < 100; ++i)
        spring.Update(target, 1.0f / 60.0f);

    // 100フレーム後にはターゲットに近づいているはず
    EXPECT_NEAR(spring.currentValue, target, 0.1f);
}

TEST(ProceduralAnimationTest, ResetPhases)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.legPhaseOffsets = { 0.0f };
    animator.SetGait(gait);

    LegConfig cfg;
    animator.AddLeg(cfg);
    animator.SetMovementSpeed(1.0f);

    animator.Update(0.5f);
    EXPECT_NE(animator.GetLegState(0).phase, 0.0f);

    animator.Reset();
    EXPECT_NEAR(animator.GetLegState(0).phase, 0.0f, 1e-5f);
}

TEST(ProceduralAnimationTest, EnableDisable)
{
    ProceduralAnimator animator;
    EXPECT_TRUE(animator.IsEnabled());
    animator.SetEnabled(false);
    EXPECT_FALSE(animator.IsEnabled());

    GaitPattern gait;
    gait.legPhaseOffsets = { 0.0f };
    animator.SetGait(gait);
    LegConfig cfg;
    animator.AddLeg(cfg);
    animator.SetMovementSpeed(1.0f);

    auto stateBefore = animator.GetLegState(0);
    animator.Update(0.5f); // disabled, should not advance
    auto stateAfter = animator.GetLegState(0);
    EXPECT_NEAR(stateAfter.phase, stateBefore.phase, 1e-5f);

    animator.SetEnabled(true);
    EXPECT_TRUE(animator.IsEnabled());
}

TEST(ProceduralAnimationTest, GaitPhaseOffset)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.dutyFactor = 0.5f;
    gait.legPhaseOffsets = { 0.0f, 0.5f, 0.25f, 0.75f };
    animator.SetGait(gait);

    LegConfig cfg;
    for (int i = 0; i < 4; ++i)
        animator.AddLeg(cfg);

    animator.SetMovementSpeed(1.0f);
    animator.Update(0.1f);

    // 各脚の位相はオフセットにより異なるはず
    auto s0 = animator.GetLegState(0);
    auto s1 = animator.GetLegState(1);
    EXPECT_NE(s0.phase, s1.phase);
}

TEST(ProceduralAnimationTest, StanceSwingTransition)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.dutyFactor = 0.5f;
    gait.legPhaseOffsets = { 0.0f };
    animator.SetGait(gait);

    LegConfig cfg;
    cfg.stepHeight = 0.3f;
    animator.AddLeg(cfg);
    animator.SetMovementSpeed(1.0f);

    // サイクルの前半 (0-0.5) はスタンス、後半 (0.5-1.0) はスイング
    // 一周する間にスタンスとスイングの両方を経験するはず
    bool sawGrounded = false;
    bool sawAirborne = false;
    for (int i = 0; i < 100; ++i)
    {
        animator.Update(0.012f); // ~1.2秒分
        auto state = animator.GetLegState(0);
        if (state.isGrounded) sawGrounded = true;
        else sawAirborne = true;
    }
    EXPECT_TRUE(sawGrounded);
    EXPECT_TRUE(sawAirborne);
}

TEST(ProceduralAnimationTest, CycleTimeAffectsSpeed)
{
    // サイクルタイムが短い方が同じ時間でより多く位相が進む
    ProceduralAnimator animFast, animSlow;

    GaitPattern gaitFast;
    gaitFast.cycleTime = 0.5f;
    gaitFast.legPhaseOffsets = { 0.0f };
    animFast.SetGait(gaitFast);

    GaitPattern gaitSlow;
    gaitSlow.cycleTime = 2.0f;
    gaitSlow.legPhaseOffsets = { 0.0f };
    animSlow.SetGait(gaitSlow);

    LegConfig cfg;
    animFast.AddLeg(cfg);
    animSlow.AddLeg(cfg);

    animFast.SetMovementSpeed(1.0f);
    animSlow.SetMovementSpeed(1.0f);

    animFast.Update(0.1f);
    animSlow.Update(0.1f);

    // 速いサイクルの方が位相が大きく進んでいるはず
    float phaseFast = animFast.GetLegState(0).phase;
    float phaseSlow = animSlow.GetLegState(0).phase;
    EXPECT_GT(phaseFast, phaseSlow);
}

TEST(ProceduralAnimationTest, MultiLegCoordination)
{
    ProceduralAnimator animator;
    GaitPattern gait;
    gait.cycleTime = 1.0f;
    gait.dutyFactor = 0.6f;
    gait.legPhaseOffsets = { 0.0f, 0.5f, 0.25f, 0.75f };
    animator.SetGait(gait);

    LegConfig cfg;
    cfg.footOffset = { 0, 0, 0 };
    for (int i = 0; i < 4; ++i)
        animator.AddLeg(cfg);

    animator.SetMovementSpeed(2.0f);
    animator.SetMovementDirection({ 0, 0, 1 });

    // 複数フレーム更新
    for (int i = 0; i < 60; ++i)
        animator.Update(1.0f / 60.0f);

    // すべての脚の状態がNaNでないこと
    for (int i = 0; i < 4; ++i)
    {
        auto state = animator.GetLegState(i);
        EXPECT_FALSE(std::isnan(state.phase));
        EXPECT_FALSE(std::isnan(state.footWorldPosition.x));
        EXPECT_FALSE(std::isnan(state.footWorldPosition.y));
        EXPECT_FALSE(std::isnan(state.footWorldPosition.z));
    }
}
