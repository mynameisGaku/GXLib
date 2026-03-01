/// @file test_AnimatorStateMachine.cpp
/// @brief AnimatorStateMachineのステート/遷移/トリガー/float条件テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/AnimatorStateMachine.h"
#include "TestHelpers.h"

using namespace gx;
using namespace gx::TestHelpers;

// 永続テストクリップ（ASMが参照する間、生存し続ける必要がある）
static AnimationClip s_idleClip  = CreateSimpleClip("Idle", 2.0f, 0.0f, 0.0f, 0.0f);
static AnimationClip s_walkClip  = CreateSimpleClip("Walk", 1.0f, 1.0f, 0.0f, 0.0f);
static AnimationClip s_runClip   = CreateSimpleClip("Run",  0.5f, 3.0f, 0.0f, 0.0f);
static AnimationClip s_jumpClip  = CreateSimpleClip("Jump", 0.8f, 0.0f, 2.0f, 0.0f);

static AnimatorStateMachine MakeSimpleASM()
{
    AnimatorStateMachine asm_;
    AnimState idle;
    idle.name = "Idle";
    idle.clip = &s_idleClip;
    idle.loop = true;
    idle.speed = 1.0f;
    asm_.AddState(idle);

    AnimState walk;
    walk.name = "Walk";
    walk.clip = &s_walkClip;
    walk.loop = true;
    walk.speed = 1.0f;
    asm_.AddState(walk);

    return asm_;
}

// ==================== ステート管理 ====================

TEST(ASMTest, AddState_ReturnsIndex)
{
    AnimatorStateMachine asm_;
    AnimState state;
    state.name = "Idle";
    state.clip = &s_idleClip;
    uint32_t idx = asm_.AddState(state);
    EXPECT_EQ(idx, 0u);
}

TEST(ASMTest, AddState_MultipleStates)
{
    AnimatorStateMachine asm_;
    AnimState s1; s1.name = "A"; s1.clip = &s_idleClip;
    AnimState s2; s2.name = "B"; s2.clip = &s_walkClip;
    AnimState s3; s3.name = "C"; s3.clip = &s_runClip;
    EXPECT_EQ(asm_.AddState(s1), 0u);
    EXPECT_EQ(asm_.AddState(s2), 1u);
    EXPECT_EQ(asm_.AddState(s3), 2u);
}

TEST(ASMTest, GetStateCount)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    EXPECT_EQ(asm_.GetStateCount(), 2u);
}

TEST(ASMTest, GetState_Valid)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    const AnimState* s = asm_.GetState(0);
    EXPECT_NE(s, nullptr);
    EXPECT_EQ(s->name, "Idle");
}

TEST(ASMTest, GetState_OutOfRange)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    const AnimState* s = asm_.GetState(999);
    EXPECT_EQ(s, nullptr);
}

TEST(ASMTest, GetCurrentState_Default)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
    const AnimState* s = asm_.GetCurrentState();
    EXPECT_NE(s, nullptr);
    EXPECT_EQ(s->name, "Idle");
}

TEST(ASMTest, SetCurrentState)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    asm_.SetCurrentState(1);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);
    EXPECT_EQ(asm_.GetCurrentState()->name, "Walk");
}

// ==================== 遷移 ====================

TEST(ASMTest, AddTransition)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    AnimTransition trans;
    trans.fromState = 0;
    trans.toState = 1;
    trans.duration = 0.3f;
    trans.triggerName = "StartWalk";
    asm_.AddTransition(trans);
    EXPECT_EQ(asm_.GetTransitions().size(), 1u);
}

TEST(ASMTest, Transition_MultipleRules)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    AnimTransition t1; t1.fromState = 0; t1.toState = 1; t1.triggerName = "Walk";
    AnimTransition t2; t2.fromState = 1; t2.toState = 0; t2.triggerName = "Stop";
    asm_.AddTransition(t1);
    asm_.AddTransition(t2);
    EXPECT_EQ(asm_.GetTransitions().size(), 2u);
}

TEST(ASMTest, IsTransitioning_Default)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    EXPECT_FALSE(asm_.IsTransitioning());
}

// ==================== トリガーベースの遷移 ====================

TEST(ASMTest, Trigger_ChangesState)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    AnimTransition trans;
    trans.fromState = 0;
    trans.toState = 1;
    trans.duration = 0.2f;
    trans.triggerName = "Walk";
    asm_.AddTransition(trans);

    asm_.SetTrigger("Walk");

    // 遷移を処理するために更新
    TransformTRS pose[1];
    asm_.Update(0.01f, 1, nullptr, pose);
    EXPECT_TRUE(asm_.IsTransitioning());
}

TEST(ASMTest, Trigger_CompletesTransition)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    AnimTransition trans;
    trans.fromState = 0;
    trans.toState = 1;
    trans.duration = 0.1f;
    trans.triggerName = "Walk";
    asm_.AddTransition(trans);

    asm_.SetTrigger("Walk");

    TransformTRS pose[1];
    // 遷移時間を超えるまで更新
    asm_.Update(0.05f, 1, nullptr, pose);
    asm_.Update(0.05f, 1, nullptr, pose);
    asm_.Update(0.05f, 1, nullptr, pose);  // 完了のための追加フレーム

    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);
}

TEST(ASMTest, Trigger_WrongState)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    AnimTransition trans;
    trans.fromState = 1;  // Walkから
    trans.toState = 0;    // Idleへ
    trans.duration = 0.2f;
    trans.triggerName = "Stop";
    asm_.AddTransition(trans);

    // 現在ステート0（Idle）にいるので、"Stop"トリガーは発火しないはず
    asm_.SetTrigger("Stop");
    TransformTRS pose[1];
    asm_.Update(0.01f, 1, nullptr, pose);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
    EXPECT_FALSE(asm_.IsTransitioning());
}

TEST(ASMTest, Trigger_NonExistentTrigger)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    AnimTransition trans;
    trans.fromState = 0;
    trans.toState = 1;
    trans.triggerName = "Walk";
    asm_.AddTransition(trans);

    asm_.SetTrigger("Fly");  // 間違ったトリガー
    TransformTRS pose[1];
    asm_.Update(0.01f, 1, nullptr, pose);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
}

// ==================== Floatパラメータ ====================

TEST(ASMTest, Float_DefaultZero)
{
    AnimatorStateMachine asm_;
    EXPECT_NEAR(asm_.GetFloat("speed"), 0.0f, 1e-5f);
}

TEST(ASMTest, Float_SetAndGet)
{
    AnimatorStateMachine asm_;
    asm_.SetFloat("speed", 3.5f);
    EXPECT_NEAR(asm_.GetFloat("speed"), 3.5f, 1e-5f);
}

TEST(ASMTest, Float_MultipleParams)
{
    AnimatorStateMachine asm_;
    asm_.SetFloat("speed", 1.0f);
    asm_.SetFloat("direction", 90.0f);
    EXPECT_NEAR(asm_.GetFloat("speed"), 1.0f, 1e-5f);
    EXPECT_NEAR(asm_.GetFloat("direction"), 90.0f, 1e-5f);
}

TEST(ASMTest, Float_Overwrite)
{
    AnimatorStateMachine asm_;
    asm_.SetFloat("speed", 1.0f);
    asm_.SetFloat("speed", 5.0f);
    EXPECT_NEAR(asm_.GetFloat("speed"), 5.0f, 1e-5f);
}

// ==================== 終了時間による遷移 ====================

TEST(ASMTest, ExitTime_Transition)
{
    AnimatorStateMachine asm_;
    AnimState idle; idle.name = "Idle"; idle.clip = &s_idleClip; idle.loop = false;
    AnimState walk; walk.name = "Walk"; walk.clip = &s_walkClip; walk.loop = true;
    asm_.AddState(idle);
    asm_.AddState(walk);

    AnimTransition trans;
    trans.fromState = 0;
    trans.toState = 1;
    trans.duration = 0.1f;
    trans.hasExitTime = true;
    trans.exitTimeNorm = 1.0f;  // クリップの終端
    asm_.AddTransition(trans);

    TransformTRS pose[1];
    // 終了時間に到達するまで十分に更新（idleの長さ = 2.0秒）
    for (int i = 0; i < 250; ++i)
        asm_.Update(0.01f, 1, nullptr, pose);

    // 合計2.5秒後、Walkに遷移しているはず
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);
}

// ==================== 更新で有効なポーズが生成される ====================

TEST(ASMTest, Update_ProducesValidPose)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    TransformTRS pose[1];
    asm_.Update(0.5f, 1, nullptr, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
    EXPECT_FALSE(std::isnan(pose[0].rotation.w));
    EXPECT_FALSE(std::isnan(pose[0].scale.x));
}

TEST(ASMTest, Update_WithBindPose)
{
    AnimatorStateMachine asm_ = MakeSimpleASM();
    TransformTRS bind[1] = {IdentityTRS()};
    TransformTRS pose[1];
    asm_.Update(0.5f, 1, bind, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
}

// ==================== 連鎖遷移 ====================

TEST(ASMTest, ChainTransition)
{
    AnimatorStateMachine asm_;
    AnimState s1; s1.name = "A"; s1.clip = &s_idleClip;
    AnimState s2; s2.name = "B"; s2.clip = &s_walkClip;
    AnimState s3; s3.name = "C"; s3.clip = &s_runClip;
    asm_.AddState(s1);
    asm_.AddState(s2);
    asm_.AddState(s3);

    AnimTransition t1; t1.fromState = 0; t1.toState = 1; t1.duration = 0.05f; t1.triggerName = "GoB";
    AnimTransition t2; t2.fromState = 1; t2.toState = 2; t2.duration = 0.05f; t2.triggerName = "GoC";
    asm_.AddTransition(t1);
    asm_.AddTransition(t2);

    TransformTRS pose[1];

    // 遷移 A -> B
    asm_.SetTrigger("GoB");
    for (int i = 0; i < 20; ++i)
        asm_.Update(0.01f, 1, nullptr, pose);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);

    // 遷移 B -> C
    asm_.SetTrigger("GoC");
    for (int i = 0; i < 20; ++i)
        asm_.Update(0.01f, 1, nullptr, pose);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 2u);
}

// ==================== AnimStateプロパティ ====================

TEST(ASMTest, StateProperties)
{
    AnimState state;
    state.name = "Jump";
    state.clip = &s_jumpClip;
    state.loop = false;
    state.speed = 2.0f;
    state.blendTree = nullptr;

    EXPECT_EQ(state.name, "Jump");
    EXPECT_EQ(state.clip, &s_jumpClip);
    EXPECT_FALSE(state.loop);
    EXPECT_NEAR(state.speed, 2.0f, 1e-5f);
    EXPECT_EQ(state.blendTree, nullptr);
}

TEST(ASMTest, TransitionProperties)
{
    AnimTransition trans;
    trans.fromState = 2;
    trans.toState = 0;
    trans.duration = 0.5f;
    trans.triggerName = "Reset";
    trans.hasExitTime = true;
    trans.exitTimeNorm = 0.9f;

    EXPECT_EQ(trans.fromState, 2u);
    EXPECT_EQ(trans.toState, 0u);
    EXPECT_NEAR(trans.duration, 0.5f, 1e-5f);
    EXPECT_EQ(trans.triggerName, "Reset");
    EXPECT_TRUE(trans.hasExitTime);
    EXPECT_NEAR(trans.exitTimeNorm, 0.9f, 1e-5f);
}
