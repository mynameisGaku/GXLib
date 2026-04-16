/// @file test_TransitionEffect.cpp
/// @brief シーン遷移エフェクトのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/TransitionEffect.h"

using namespace gx;

// =========================================================================
// BuiltinTransition - フェード
// =========================================================================

TEST(BuiltinTransition, Fade_Progress0_FullyTransparent)
{
    BuiltinTransition effect(TransitionType::Fade);
    // progress 0ではマスクは0であるべき（完全透明 / 旧シーンが見える）
    float mask = effect.GetMaskValue(0.0f, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(mask, 0.0f);
}

TEST(BuiltinTransition, Fade_Progress1_FullyOpaque)
{
    BuiltinTransition effect(TransitionType::Fade);
    // progress 1ではマスクは1であるべき（完全不透明 / 新シーンが見える）
    float mask = effect.GetMaskValue(1.0f, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(mask, 1.0f);
}

TEST(BuiltinTransition, Fade_ProgressHalf)
{
    BuiltinTransition effect(TransitionType::Fade);
    float mask = effect.GetMaskValue(0.5f, 0.5f, 0.5f);
    // 約0.5であるべき（線形フェード）
    EXPECT_NEAR(mask, 0.5f, 0.1f);
}

TEST(BuiltinTransition, Fade_UniformAcrossScreen)
{
    BuiltinTransition effect(TransitionType::Fade);
    // フェードは均一 -- 画面のどの位置でも同じマスク値
    float m1 = effect.GetMaskValue(0.5f, 0.0f, 0.0f);
    float m2 = effect.GetMaskValue(0.5f, 1.0f, 1.0f);
    float m3 = effect.GetMaskValue(0.5f, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(m1, m2);
    EXPECT_FLOAT_EQ(m2, m3);
}

// =========================================================================
// BuiltinTransition - ワイプ
// =========================================================================

TEST(BuiltinTransition, Wipe_Progress0_AllTransparent)
{
    BuiltinTransition effect(TransitionType::Wipe);
    // progress 0では画面右端は透明であるべき
    float mask = effect.GetMaskValue(0.0f, 0.8f, 0.5f);
    EXPECT_LE(mask, 0.1f);
}

TEST(BuiltinTransition, Wipe_Progress1_AllOpaque)
{
    BuiltinTransition effect(TransitionType::Wipe);
    // progress 1では全体が不透明であるべき
    float mask = effect.GetMaskValue(1.0f, 0.2f, 0.5f);
    EXPECT_GE(mask, 0.9f);
}

TEST(BuiltinTransition, Wipe_ProgressHalf_LeftOpaqueRightTransparent)
{
    BuiltinTransition effect(TransitionType::Wipe);
    float maskLeft = effect.GetMaskValue(0.5f, 0.1f, 0.5f);
    float maskRight = effect.GetMaskValue(0.5f, 0.9f, 0.5f);
    EXPECT_GT(maskLeft, maskRight);
}

// =========================================================================
// BuiltinTransition - 垂直ワイプ
// =========================================================================

TEST(BuiltinTransition, WipeVertical_ProgressHalf_TopOpaqueBottomTransparent)
{
    BuiltinTransition effect(TransitionType::WipeVertical);
    float maskTop = effect.GetMaskValue(0.5f, 0.5f, 0.1f);
    float maskBottom = effect.GetMaskValue(0.5f, 0.5f, 0.9f);
    EXPECT_GT(maskTop, maskBottom);
}

// =========================================================================
// BuiltinTransition - サークル
// =========================================================================

TEST(BuiltinTransition, CircleOpen_Progress0_AllTransparent)
{
    BuiltinTransition effect(TransitionType::CircleOpen);
    // progress 0では円は閉じている -- 全体が透明
    float mask = effect.GetMaskValue(0.0f, 0.5f, 0.5f);
    EXPECT_LE(mask, 0.1f);
}

TEST(BuiltinTransition, CircleOpen_Progress1_AllOpaque)
{
    BuiltinTransition effect(TransitionType::CircleOpen);
    // progress 1では円は完全に開いている -- 全体が不透明
    float mask = effect.GetMaskValue(1.0f, 0.5f, 0.5f);
    EXPECT_GE(mask, 0.9f);
}

TEST(BuiltinTransition, CircleClose_Symmetry)
{
    BuiltinTransition open(TransitionType::CircleOpen);
    BuiltinTransition close(TransitionType::CircleClose);
    // CircleCloseのprogress pはCircleOpenのprogress pの逆であるべき
    // すなわち close(p) ~ open(1-p) または close(p) ~ 1 - open(p)
    float openMask = open.GetMaskValue(0.3f, 0.5f, 0.5f);
    float closeMask = close.GetMaskValue(0.7f, 0.5f, 0.5f);
    EXPECT_NEAR(openMask, closeMask, 0.15f);
}

// =========================================================================
// BuiltinTransition - ディゾルブ
// =========================================================================

TEST(BuiltinTransition, Dissolve_Progress0_AllTransparent)
{
    BuiltinTransition effect(TransitionType::Dissolve);
    // progress 0では全ピクセルが透明であるべき
    float mask = effect.GetMaskValue(0.0f, 0.5f, 0.5f);
    EXPECT_LE(mask, 0.1f);
}

TEST(BuiltinTransition, Dissolve_Progress1_AllOpaque)
{
    BuiltinTransition effect(TransitionType::Dissolve);
    // progress 1では全ピクセルが不透明であるべき
    float mask = effect.GetMaskValue(1.0f, 0.5f, 0.5f);
    EXPECT_GE(mask, 0.9f);
}

// =========================================================================
// BuiltinTransition - スライド
// =========================================================================

TEST(BuiltinTransition, SlideLeft_Progress0_Transparent)
{
    BuiltinTransition effect(TransitionType::SlideLeft);
    float mask = effect.GetMaskValue(0.0f, 0.5f, 0.5f);
    EXPECT_LE(mask, 0.1f);
}

TEST(BuiltinTransition, SlideRight_Progress1_Opaque)
{
    BuiltinTransition effect(TransitionType::SlideRight);
    float mask = effect.GetMaskValue(1.0f, 0.5f, 0.5f);
    EXPECT_GE(mask, 0.9f);
}

// =========================================================================
// BuiltinTransition - 全般
// =========================================================================

TEST(BuiltinTransition, GetType)
{
    BuiltinTransition fade(TransitionType::Fade);
    EXPECT_EQ(fade.GetType(), TransitionType::Fade);

    BuiltinTransition wipe(TransitionType::Wipe);
    EXPECT_EQ(wipe.GetType(), TransitionType::Wipe);

    BuiltinTransition circle(TransitionType::CircleOpen);
    EXPECT_EQ(circle.GetType(), TransitionType::CircleOpen);
}

TEST(BuiltinTransition, SetParams)
{
    BuiltinTransition effect(TransitionType::Fade);
    TransitionParams params;
    params.centerX = 0.3f;
    params.centerY = 0.7f;
    params.r = 1.0f;
    params.softEdge = 0.1f;
    effect.SetParams(params);

    const auto& p = effect.GetParams();
    EXPECT_FLOAT_EQ(p.centerX, 0.3f);
    EXPECT_FLOAT_EQ(p.centerY, 0.7f);
    EXPECT_FLOAT_EQ(p.r, 1.0f);
    EXPECT_FLOAT_EQ(p.softEdge, 0.1f);
}

// =========================================================================
// TransitionEffectManager
// =========================================================================

TEST(TransitionEffectManager, DefaultState)
{
    TransitionEffectManager mgr;
    EXPECT_FALSE(mgr.IsActive());
    EXPECT_FLOAT_EQ(mgr.GetProgress(), 0.0f);
    EXPECT_EQ(mgr.GetEffect(), nullptr);
}

TEST(TransitionEffectManager, SetEffect_Custom)
{
    TransitionEffectManager mgr;
    auto effect = std::make_unique<BuiltinTransition>(TransitionType::Wipe);
    ITransitionEffect* rawPtr = effect.get();
    mgr.SetEffect(std::move(effect));
    EXPECT_EQ(mgr.GetEffect(), rawPtr);
}

TEST(TransitionEffectManager, SetEffect_Builtin)
{
    TransitionEffectManager mgr;
    mgr.SetEffect(TransitionType::Fade);
    ASSERT_NE(mgr.GetEffect(), nullptr);
    EXPECT_EQ(mgr.GetEffect()->GetType(), TransitionType::Fade);
}

TEST(TransitionEffectManager, BeginEnd)
{
    TransitionEffectManager mgr;
    mgr.SetEffect(TransitionType::Fade);

    EXPECT_FALSE(mgr.IsActive());
    mgr.Begin();
    EXPECT_TRUE(mgr.IsActive());
    EXPECT_FLOAT_EQ(mgr.GetProgress(), 0.0f);

    mgr.End();
    EXPECT_FALSE(mgr.IsActive());
    EXPECT_FLOAT_EQ(mgr.GetProgress(), 0.0f);
}

TEST(TransitionEffectManager, SetProgress_GetProgress)
{
    TransitionEffectManager mgr;
    mgr.SetEffect(TransitionType::Fade);
    mgr.Begin();

    mgr.SetProgress(0.5f);
    EXPECT_FLOAT_EQ(mgr.GetProgress(), 0.5f);

    mgr.SetProgress(1.0f);
    EXPECT_FLOAT_EQ(mgr.GetProgress(), 1.0f);
}

TEST(TransitionEffectManager, SampleMask_DelegatesToEffect)
{
    TransitionEffectManager mgr;
    mgr.SetEffect(TransitionType::Fade);
    mgr.Begin();
    mgr.SetProgress(1.0f);

    // フェードのprogress 1.0ではmask=1.0を返すはず
    float mask = mgr.SampleMask(0.5f, 0.5f);
    EXPECT_FLOAT_EQ(mask, 1.0f);
}

TEST(TransitionEffectManager, SampleMask_NullEffect)
{
    TransitionEffectManager mgr;
    // エフェクト未設定 -- SampleMaskは0を返すかクラッシュしないこと
    float mask = mgr.SampleMask(0.5f, 0.5f);
    EXPECT_FLOAT_EQ(mask, 0.0f);
}
