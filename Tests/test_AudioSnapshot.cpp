/// @file test_AudioSnapshot.cpp
/// @brief AudioSnapshotManager のユニットテスト（12テスト）

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/AudioSnapshot.h"

using namespace gx;

// ============================================================================
// ヘルパー
// ============================================================================

static AudioSnapshotData MakeSnapshot(const std::string& name,
                                       float masterVol = 1.0f,
                                       float transTime = 0.5f)
{
    AudioSnapshotData snap;
    snap.name = name;
    snap.masterVolume = masterVol;
    snap.transitionTime = transTime;

    BusSnapshot bgm;
    bgm.busName = "BGM";
    bgm.volume = masterVol * 0.8f;
    bgm.muted = false;
    bgm.pan = 0.0f;
    snap.buses.push_back(bgm);

    BusSnapshot se;
    se.busName = "SE";
    se.volume = masterVol;
    se.muted = false;
    se.pan = 0.0f;
    snap.buses.push_back(se);

    return snap;
}

// ============================================================================
// テスト
// ============================================================================

/// 構築と破棄が安全に行えること
TEST(AudioSnapshotTest, ConstructDestruct)
{
    AudioSnapshotManager mgr;
    EXPECT_EQ(mgr.GetSnapshotCount(), 0u);
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "");
    EXPECT_FALSE(mgr.IsTransitioning());
    EXPECT_FLOAT_EQ(mgr.GetTransitionProgress(), 1.0f);
}

/// スナップショット登録が機能すること
TEST(AudioSnapshotTest, RegisterSnapshot)
{
    AudioSnapshotManager mgr;

    mgr.RegisterSnapshot(MakeSnapshot("Normal"));
    EXPECT_EQ(mgr.GetSnapshotCount(), 1u);
    EXPECT_NE(mgr.GetSnapshot("Normal"), nullptr);

    mgr.RegisterSnapshot(MakeSnapshot("Pause", 0.3f));
    EXPECT_EQ(mgr.GetSnapshotCount(), 2u);

    // 同名で上書き登録
    mgr.RegisterSnapshot(MakeSnapshot("Normal", 0.5f));
    EXPECT_EQ(mgr.GetSnapshotCount(), 2u);
    const auto* snap = mgr.GetSnapshot("Normal");
    ASSERT_NE(snap, nullptr);
    EXPECT_FLOAT_EQ(snap->masterVolume, 0.5f);
}

/// スナップショット登録解除が機能すること
TEST(AudioSnapshotTest, UnregisterSnapshot)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("Normal"));
    mgr.RegisterSnapshot(MakeSnapshot("Pause"));
    EXPECT_EQ(mgr.GetSnapshotCount(), 2u);

    mgr.UnregisterSnapshot("Normal");
    EXPECT_EQ(mgr.GetSnapshotCount(), 1u);
    EXPECT_EQ(mgr.GetSnapshot("Normal"), nullptr);
    EXPECT_NE(mgr.GetSnapshot("Pause"), nullptr);

    // 存在しないスナップショットの登録解除は安全に無視
    mgr.UnregisterSnapshot("NonExistent");
    EXPECT_EQ(mgr.GetSnapshotCount(), 1u);
}

/// GetSnapshotが正しいデータを返すこと
TEST(AudioSnapshotTest, GetSnapshot)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("Underwater", 0.6f, 2.0f));

    const auto* snap = mgr.GetSnapshot("Underwater");
    ASSERT_NE(snap, nullptr);
    EXPECT_EQ(snap->name, "Underwater");
    EXPECT_FLOAT_EQ(snap->masterVolume, 0.6f);
    EXPECT_FLOAT_EQ(snap->transitionTime, 2.0f);
    EXPECT_EQ(snap->buses.size(), 2u);
    EXPECT_EQ(snap->buses[0].busName, "BGM");
    EXPECT_EQ(snap->buses[1].busName, "SE");

    // 存在しないスナップショット
    EXPECT_EQ(mgr.GetSnapshot("Missing"), nullptr);
}

/// スナップショット数が正しいこと
TEST(AudioSnapshotTest, SnapshotCount)
{
    AudioSnapshotManager mgr;
    EXPECT_EQ(mgr.GetSnapshotCount(), 0u);

    mgr.RegisterSnapshot(MakeSnapshot("A"));
    EXPECT_EQ(mgr.GetSnapshotCount(), 1u);

    mgr.RegisterSnapshot(MakeSnapshot("B"));
    mgr.RegisterSnapshot(MakeSnapshot("C"));
    EXPECT_EQ(mgr.GetSnapshotCount(), 3u);

    mgr.UnregisterSnapshot("B");
    EXPECT_EQ(mgr.GetSnapshotCount(), 2u);
}

/// ApplySnapshotで即座にスナップショットが適用されること
TEST(AudioSnapshotTest, ApplySnapshot)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("Normal"));
    mgr.RegisterSnapshot(MakeSnapshot("Combat", 0.9f));

    mgr.ApplySnapshot("Normal");
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");
    EXPECT_FALSE(mgr.IsTransitioning());

    mgr.ApplySnapshot("Combat");
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Combat");
    EXPECT_FALSE(mgr.IsTransitioning());

    // 存在しないスナップショットは無視される
    mgr.ApplySnapshot("NonExistent");
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Combat");
}

/// BlendToSnapshotでトランジションが開始されること
TEST(AudioSnapshotTest, BlendToSnapshot)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("Normal"));
    mgr.RegisterSnapshot(MakeSnapshot("Combat"));

    mgr.ApplySnapshot("Normal");
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");

    mgr.BlendToSnapshot("Combat", 2.0f);
    EXPECT_TRUE(mgr.IsTransitioning());
    // トランジション中は現在のスナップショット名は変わらない
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");

    // 存在しないスナップショットへのブレンドは無視
    mgr.BlendToSnapshot("NonExistent", 1.0f);
    // 前のトランジションが継続
    EXPECT_TRUE(mgr.IsTransitioning());
}

/// Updateでトランジションが進行・完了すること
TEST(AudioSnapshotTest, UpdateTransition)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("Normal"));
    mgr.RegisterSnapshot(MakeSnapshot("Combat"));

    mgr.ApplySnapshot("Normal");
    mgr.BlendToSnapshot("Combat", 1.0f);
    EXPECT_TRUE(mgr.IsTransitioning());

    // 半分進行
    mgr.Update(0.5f);
    EXPECT_TRUE(mgr.IsTransitioning());
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");

    // 完了
    mgr.Update(0.5f);
    EXPECT_FALSE(mgr.IsTransitioning());
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Combat");
}

/// トランジション進行度が正しく計算されること
TEST(AudioSnapshotTest, TransitionProgress)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("A"));
    mgr.RegisterSnapshot(MakeSnapshot("B"));

    // トランジションなしでは1.0
    EXPECT_FLOAT_EQ(mgr.GetTransitionProgress(), 1.0f);

    mgr.ApplySnapshot("A");
    mgr.BlendToSnapshot("B", 2.0f);

    // 開始直後: 0.0
    EXPECT_NEAR(mgr.GetTransitionProgress(), 0.0f, 0.001f);

    // 25% 進行
    mgr.Update(0.5f);
    EXPECT_NEAR(mgr.GetTransitionProgress(), 0.25f, 0.001f);

    // 50% 進行
    mgr.Update(0.5f);
    EXPECT_NEAR(mgr.GetTransitionProgress(), 0.5f, 0.001f);

    // 75% 進行
    mgr.Update(0.5f);
    EXPECT_NEAR(mgr.GetTransitionProgress(), 0.75f, 0.001f);

    // 完了
    mgr.Update(0.5f);
    EXPECT_FLOAT_EQ(mgr.GetTransitionProgress(), 1.0f);
    EXPECT_FALSE(mgr.IsTransitioning());
}

/// IsTransitioningの状態遷移
TEST(AudioSnapshotTest, IsTransitioning)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("A"));
    mgr.RegisterSnapshot(MakeSnapshot("B"));

    EXPECT_FALSE(mgr.IsTransitioning());

    mgr.ApplySnapshot("A");
    EXPECT_FALSE(mgr.IsTransitioning());

    mgr.BlendToSnapshot("B", 1.0f);
    EXPECT_TRUE(mgr.IsTransitioning());

    mgr.Update(1.0f);
    EXPECT_FALSE(mgr.IsTransitioning());

    // transitionTime=0はApplySnapshotと同等
    mgr.BlendToSnapshot("A", 0.0f);
    EXPECT_FALSE(mgr.IsTransitioning());
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "A");
}

/// Resetで全状態がクリアされること
TEST(AudioSnapshotTest, Reset)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("A"));
    mgr.RegisterSnapshot(MakeSnapshot("B"));

    mgr.ApplySnapshot("A");
    mgr.BlendToSnapshot("B", 2.0f);
    mgr.Update(0.5f);

    EXPECT_TRUE(mgr.IsTransitioning());
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "A");

    mgr.Reset();

    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "");
    EXPECT_FALSE(mgr.IsTransitioning());
    EXPECT_FLOAT_EQ(mgr.GetTransitionProgress(), 1.0f);

    // スナップショットの登録自体は残る
    EXPECT_EQ(mgr.GetSnapshotCount(), 2u);
}

/// 現在のスナップショット名が正しく追跡されること
TEST(AudioSnapshotTest, CurrentSnapshotName)
{
    AudioSnapshotManager mgr;
    mgr.RegisterSnapshot(MakeSnapshot("Normal"));
    mgr.RegisterSnapshot(MakeSnapshot("Pause"));
    mgr.RegisterSnapshot(MakeSnapshot("Combat"));

    // 初期状態は空
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "");

    // Apply で即座にセット
    mgr.ApplySnapshot("Normal");
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");

    // Blend 開始中は前のスナップショット名を保持
    mgr.BlendToSnapshot("Combat", 1.0f);
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");

    // 完了後に遷移先に切り替わる
    mgr.Update(1.0f);
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Combat");

    // Apply で上書き（トランジション中断）
    mgr.BlendToSnapshot("Pause", 5.0f);
    mgr.Update(0.1f);
    EXPECT_TRUE(mgr.IsTransitioning());

    mgr.ApplySnapshot("Normal");
    EXPECT_FALSE(mgr.IsTransitioning());
    EXPECT_EQ(mgr.GetCurrentSnapshotName(), "Normal");
}
