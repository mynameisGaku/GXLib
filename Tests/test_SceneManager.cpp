/// @file test_SceneManager.cpp
/// @brief Phase 2: SceneManager 遷移・スタック・アルファ

#include <gtest/gtest.h>
#include "Core/Scene/SceneManager.h"

using namespace gx;

TEST(SceneManagerTest, ChangeScene)
{
    SceneManager mgr;
    EXPECT_EQ(mgr.GetCurrentScene(), nullptr);

    mgr.ChangeScene([]() { return std::make_unique<Scene>("TestScene"); },
                    { 0.0f, 0.0f }); // フェードなし
    // ローディング状態を処理
    mgr.Update(0.016f);

    ASSERT_NE(mgr.GetCurrentScene(), nullptr);
    EXPECT_EQ(mgr.GetCurrentScene()->GetName(), "TestScene");
}

TEST(SceneManagerTest, PushAndPopScene)
{
    SceneManager mgr;

    // 初期シーンを設定（フェードなし）
    mgr.ChangeScene([]() { return std::make_unique<Scene>("Base"); }, { 0, 0 });
    mgr.Update(0.016f);
    EXPECT_EQ(mgr.GetStackDepth(), 1u);

    // オーバーレイシーンをプッシュ
    mgr.PushScene([]() { return std::make_unique<Scene>("Overlay"); }, { 0, 0 });
    mgr.Update(0.016f);
    EXPECT_EQ(mgr.GetStackDepth(), 2u);
    EXPECT_EQ(mgr.GetCurrentScene()->GetName(), "Overlay");

    // オーバーレイをポップ
    mgr.PopScene({ 0, 0 });
    mgr.Update(0.016f); // フェードアウト
    mgr.Update(0.016f); // ローディング/復帰
    EXPECT_EQ(mgr.GetStackDepth(), 1u);
    EXPECT_EQ(mgr.GetCurrentScene()->GetName(), "Base");
}

TEST(SceneManagerTest, TransitionAlpha)
{
    SceneManager mgr;

    // 初期シーン
    mgr.ChangeScene([]() { return std::make_unique<Scene>("A"); }, { 0, 0 });
    mgr.Update(0.016f);

    // フェード付きで変更（0.5秒アウト、0.5秒イン）
    mgr.ChangeScene([]() { return std::make_unique<Scene>("B"); }, { 0.5f, 0.5f });
    EXPECT_TRUE(mgr.IsTransitioning());

    // フェードアウト中、アルファは増加するはず
    mgr.Update(0.25f);
    EXPECT_TRUE(mgr.IsTransitioning());
    EXPECT_GT(mgr.GetTransitionAlpha(), 0.0f);

    // フェードアウト完了
    mgr.Update(0.3f);

    // ローディング状態 → フェードインへ遷移
    mgr.Update(0.01f);

    // フェードイン中、アルファは減少しているはず
    float alpha = mgr.GetTransitionAlpha();
    mgr.Update(0.3f);
    EXPECT_LE(mgr.GetTransitionAlpha(), alpha);
}

TEST(SceneManagerTest, ChangeSceneReplacesStack)
{
    SceneManager mgr;

    mgr.ChangeScene([]() { return std::make_unique<Scene>("A"); }, { 0, 0 });
    mgr.Update(0.016f);
    mgr.PushScene([]() { return std::make_unique<Scene>("B"); }, { 0, 0 });
    mgr.Update(0.016f);
    EXPECT_EQ(mgr.GetStackDepth(), 2u);

    // ChangeSceneはスタック全体をクリアするはず
    mgr.ChangeScene([]() { return std::make_unique<Scene>("C"); }, { 0, 0 });
    mgr.Update(0.016f);
    EXPECT_EQ(mgr.GetStackDepth(), 1u);
    EXPECT_EQ(mgr.GetCurrentScene()->GetName(), "C");
}

TEST(SceneManagerTest, EmptyManagerReturnsNull)
{
    SceneManager mgr;
    EXPECT_EQ(mgr.GetCurrentScene(), nullptr);
    EXPECT_FALSE(mgr.IsTransitioning());
    mgr.Update(0.016f);  // クラッシュしないことを確認
}
