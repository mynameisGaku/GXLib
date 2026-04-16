/// @file test_UITween.cpp
/// @brief UITweenManagerアニメーションロジックの単体テスト

#include <gtest/gtest.h>
#include "GUI/UITween.h"
#include "GUI/Widgets/Panel.h"

using namespace gx::GUI;

// テスト用のシンプルなウィジェットを作成するヘルパー
static std::unique_ptr<Widget> MakeTestWidget()
{
    auto panel = std::make_unique<Panel>();
    panel->opacity = 1.0f;
    panel->layoutRect = {0, 0, 100, 50};
    panel->globalRect = {0, 0, 100, 50};
    return panel;
}

TEST(UITweenManager, DefaultNoAnimations)
{
    UITweenManager mgr;
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 0u);
}

TEST(UITweenManager, AnimateReturnsHandle)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 0.5f);
    EXPECT_NE(h, k_InvalidAnimHandle);
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 1u);
}

TEST(UITweenManager, AnimateSetsFromValue)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f);
    // delay=0の場合、'from'値が即座に適用される
    EXPECT_FLOAT_EQ(widget->opacity, 0.0f);
}

TEST(UITweenManager, AnimateChangesValueOnUpdate)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f);

    // 中間まで進める
    mgr.Update(0.5f);
    // 値は0と1の間のどこかにあるはず（イージングによる）
    EXPECT_GT(widget->opacity, 0.0f);
    EXPECT_LE(widget->opacity, 1.0f);
}

TEST(UITweenManager, AnimationCompletesLinear)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 0.5f, UIEaseType::Linear);

    // 期間を超えて進める
    mgr.Update(1.0f);
    EXPECT_FLOAT_EQ(widget->opacity, 1.0f);
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 0u);
}

TEST(UITweenManager, LinearMidpoint)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f, UIEaseType::Linear);

    mgr.Update(0.5f);
    EXPECT_FLOAT_EQ(widget->opacity, 0.5f);
}

TEST(UITweenManager, Cancel)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f);
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 1u);
    mgr.Cancel(h);
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 0u);
}

TEST(UITweenManager, CancelAll)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f);
    mgr.Animate(widget.get(), UIAnimProperty::PositionX, 0.0f, 100.0f, 1.0f);
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 2u);
    mgr.CancelAll(widget.get());
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 0u);
}

TEST(UITweenManager, IsAnimating)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f);
    EXPECT_TRUE(mgr.IsAnimating(h));
    mgr.Cancel(h);
    EXPECT_FALSE(mgr.IsAnimating(h));
}

TEST(UITweenManager, DelayedAnimation)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    widget->opacity = 1.0f;
    mgr.AnimateDelayed(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f,
                       0.5f, 0.5f, UIEaseType::Linear);

    // 遅延中はopacityが元の値のまま（fromはまだ適用されていない）
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 1u);

    // 遅延を超えて進める
    mgr.Update(0.6f);
    // アニメーションが開始され、from値が適用されて進行が始まったはず
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 1u);
}

TEST(UITweenManager, FadeIn)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.FadeIn(widget.get(), 0.3f);
    EXPECT_NE(h, k_InvalidAnimHandle);
    // FadeInはopacityを0から1に設定する
    EXPECT_FLOAT_EQ(widget->opacity, 0.0f);
}

TEST(UITweenManager, FadeOut)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.FadeOut(widget.get(), 0.3f);
    EXPECT_NE(h, k_InvalidAnimHandle);
    // FadeOutはopacityを1から0に設定する
    EXPECT_FLOAT_EQ(widget->opacity, 1.0f);
}

TEST(UITweenManager, SlideInFromLeft)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.SlideInFromLeft(widget.get(), 200.0f, 0.4f);
    EXPECT_NE(h, k_InvalidAnimHandle);
    // 位置は左に200オフセットされた状態で開始するはず
    EXPECT_FLOAT_EQ(widget->layoutRect.x, -200.0f);
}

TEST(UITweenManager, ScaleIn)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    UIAnimHandle h = mgr.ScaleIn(widget.get(), 0.3f);
    EXPECT_NE(h, k_InvalidAnimHandle);
    // ScaleInは2つのアニメーション（ScaleXとScaleY）を作成する
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 2u);
}

TEST(UITweenManager, OnCompleteCallback)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    bool called = false;
    UIAnimHandle h = mgr.Animate(widget.get(), UIAnimProperty::Opacity,
                                 0.0f, 1.0f, 0.1f, UIEaseType::Linear);
    mgr.SetOnComplete(h, [&called]() { called = true; });

    mgr.Update(1.0f); // アニメーションを完了させる
    EXPECT_TRUE(called);
}

TEST(UITweenManager, MultipleAnimationsSameWidget)
{
    UITweenManager mgr;
    auto widget = MakeTestWidget();
    mgr.Animate(widget.get(), UIAnimProperty::Opacity, 0.0f, 1.0f, 1.0f);
    mgr.Animate(widget.get(), UIAnimProperty::PositionX, 0.0f, 100.0f, 1.0f);
    mgr.Animate(widget.get(), UIAnimProperty::PositionY, 0.0f, 50.0f, 1.0f);
    EXPECT_EQ(mgr.GetActiveAnimationCount(), 3u);
}

TEST(UITweenManager, UIAnimPropertyEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(UIAnimProperty::Opacity), 0u);
    EXPECT_EQ(static_cast<uint32_t>(UIAnimProperty::PositionX), 1u);
    EXPECT_EQ(static_cast<uint32_t>(UIAnimProperty::PositionY), 2u);
    EXPECT_EQ(static_cast<uint32_t>(UIAnimProperty::Width), 3u);
    EXPECT_EQ(static_cast<uint32_t>(UIAnimProperty::Height), 4u);
}

TEST(UITweenManager, UIEaseTypeEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(UIEaseType::Linear), 0u);
    EXPECT_EQ(static_cast<uint32_t>(UIEaseType::EaseOutBounce), 8u);
    EXPECT_EQ(static_cast<uint32_t>(UIEaseType::EaseOutElastic), 9u);
}
