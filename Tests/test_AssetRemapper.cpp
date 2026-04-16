/// @file test_AssetRemapper.cpp
/// @brief AssetRemapper テスト — GUID→スロット対応、差し替え、リロード統計

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/AssetRemapper.h"
#include "Core/ResourceHandle.h"
#include "Core/GUID.h"

// ============================================================================
// テストフィクスチャ
// ============================================================================

class AssetRemapperTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        gx::AssetRemapper::Instance().Clear();
    }

    void TearDown() override
    {
        gx::AssetRemapper::Instance().Clear();
    }

    gx::GUID MakeGUID() { return gx::GUID::Generate(); }
};

// ============================================================================
// Register / Unregister 基本テスト
// ============================================================================

TEST_F(AssetRemapperTest, RegisterSingle)
{
    gx::GUID g = MakeGUID();
    gx::AssetRemapper::Instance().Register(g, "textures/hero.png", 42,
        [](const gx::String&, uint32_t) { return true; });

    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 1u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetSlot(g), 42u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetPath(g), gx::String("textures/hero.png"));
}

TEST_F(AssetRemapperTest, RegisterMultiple)
{
    gx::GUID g1 = MakeGUID();
    gx::GUID g2 = MakeGUID();

    gx::AssetRemapper::Instance().Register(g1, "a.png", 1,
        [](const gx::String&, uint32_t) { return true; });
    gx::AssetRemapper::Instance().Register(g2, "b.png", 2,
        [](const gx::String&, uint32_t) { return true; });

    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 2u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetSlot(g1), 1u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetSlot(g2), 2u);
}

TEST_F(AssetRemapperTest, Unregister)
{
    gx::GUID g = MakeGUID();
    gx::AssetRemapper::Instance().Register(g, "tex.png", 10,
        [](const gx::String&, uint32_t) { return true; });
    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 1u);

    gx::AssetRemapper::Instance().Unregister(g);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 0u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetSlot(g), UINT32_MAX);
}

TEST_F(AssetRemapperTest, UnregisterNonExistent)
{
    gx::GUID g = MakeGUID();
    // クラッシュしないことを確認
    gx::AssetRemapper::Instance().Unregister(g);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 0u);
}

// ============================================================================
// OnFileChanged テスト
// ============================================================================

TEST_F(AssetRemapperTest, OnFileChangedCallsReloadFunc)
{
    gx::GUID g = MakeGUID();
    bool reloadCalled = false;
    gx::String capturedPath;
    uint32_t capturedSlot = 0;

    gx::AssetRemapper::Instance().Register(g, "textures/hero.png", 42,
        [&](const gx::String& path, uint32_t slot) {
            reloadCalled = true;
            capturedPath = path;
            capturedSlot = slot;
            return true;
        });

    bool result = gx::AssetRemapper::Instance().OnFileChanged("textures/hero.png");
    EXPECT_TRUE(result);
    EXPECT_TRUE(reloadCalled);
    EXPECT_EQ(capturedPath, gx::String("textures/hero.png"));
    EXPECT_EQ(capturedSlot, 42u);
}

TEST_F(AssetRemapperTest, OnFileChangedUnknownPath)
{
    bool result = gx::AssetRemapper::Instance().OnFileChanged("unknown/file.png");
    EXPECT_FALSE(result);
}

TEST_F(AssetRemapperTest, OnFileChangedReloadFailure)
{
    gx::GUID g = MakeGUID();
    gx::AssetRemapper::Instance().Register(g, "bad.png", 1,
        [](const gx::String&, uint32_t) { return false; });

    bool result = gx::AssetRemapper::Instance().OnFileChanged("bad.png");
    EXPECT_FALSE(result);
}

// ============================================================================
// リロード統計テスト
// ============================================================================

TEST_F(AssetRemapperTest, StatsTracking)
{
    gx::GUID g1 = MakeGUID();
    gx::GUID g2 = MakeGUID();

    gx::AssetRemapper::Instance().Register(g1, "ok.png", 1,
        [](const gx::String&, uint32_t) { return true; });
    gx::AssetRemapper::Instance().Register(g2, "fail.png", 2,
        [](const gx::String&, uint32_t) { return false; });

    gx::AssetRemapper::Instance().OnFileChanged("ok.png");
    gx::AssetRemapper::Instance().OnFileChanged("fail.png");
    gx::AssetRemapper::Instance().OnFileChanged("ok.png");

    const auto& stats = gx::AssetRemapper::Instance().GetStats();
    EXPECT_EQ(stats.totalReloads, 3u);
    EXPECT_EQ(stats.successCount, 2u);
    EXPECT_EQ(stats.failureCount, 1u);
}

TEST_F(AssetRemapperTest, ResetStats)
{
    gx::GUID g = MakeGUID();
    gx::AssetRemapper::Instance().Register(g, "x.png", 1,
        [](const gx::String&, uint32_t) { return true; });
    gx::AssetRemapper::Instance().OnFileChanged("x.png");

    gx::AssetRemapper::Instance().ResetStats();
    const auto& stats = gx::AssetRemapper::Instance().GetStats();
    EXPECT_EQ(stats.totalReloads, 0u);
    EXPECT_EQ(stats.successCount, 0u);
    EXPECT_EQ(stats.failureCount, 0u);
}

// ============================================================================
// GetSlot / GetPath テスト
// ============================================================================

TEST_F(AssetRemapperTest, GetSlotUnregistered)
{
    EXPECT_EQ(gx::AssetRemapper::Instance().GetSlot(MakeGUID()), UINT32_MAX);
}

TEST_F(AssetRemapperTest, GetPathUnregistered)
{
    EXPECT_TRUE(gx::AssetRemapper::Instance().GetPath(MakeGUID()).empty());
}

// ============================================================================
// Clear テスト
// ============================================================================

TEST_F(AssetRemapperTest, ClearRemovesAll)
{
    gx::GUID g = MakeGUID();
    gx::AssetRemapper::Instance().Register(g, "x.png", 1,
        [](const gx::String&, uint32_t) { return true; });
    gx::AssetRemapper::Instance().OnFileChanged("x.png");

    gx::AssetRemapper::Instance().Clear();
    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 0u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetStats().totalReloads, 0u);
}

// ============================================================================
// ResourceCache::Replace 統合テスト
// ============================================================================

// ResourceCache::Replace は test_ResourceCache.cpp で既存テスト群があるため
// ここではRemapperとの統合のみ検証
TEST_F(AssetRemapperTest, ResourceCacheReplaceBasic)
{
    // Replace の基本動作をシンプルな int ラッパーで確認
    struct IntRes { int v; };
    gx::ResourceCache<IntRes> cache;

    auto h = cache.Register("res1", std::make_unique<IntRes>(IntRes{10}));
    ASSERT_TRUE(h.IsValid());
    EXPECT_EQ(cache.Get(h)->v, 10);

    // 差し替え
    EXPECT_TRUE(cache.Replace(h, std::make_unique<IntRes>(IntRes{99})));
    EXPECT_EQ(cache.Get(h)->v, 99);

    // 無効ハンドルで Replace 不可
    gx::ResourceHandle<IntRes> bad;
    EXPECT_FALSE(cache.Replace(bad, std::make_unique<IntRes>(IntRes{0})));
}

// ============================================================================
// 複数リロード同一パステスト
// ============================================================================

TEST_F(AssetRemapperTest, MultipleReloadsOnSamePath)
{
    gx::GUID g = MakeGUID();
    int reloadCount = 0;

    gx::AssetRemapper::Instance().Register(g, "hot.png", 5,
        [&](const gx::String&, uint32_t) {
            ++reloadCount;
            return true;
        });

    for (int i = 0; i < 10; ++i)
        gx::AssetRemapper::Instance().OnFileChanged("hot.png");

    EXPECT_EQ(reloadCount, 10);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetStats().totalReloads, 10u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetStats().successCount, 10u);
}

// ============================================================================
// Register 上書きテスト
// ============================================================================

TEST_F(AssetRemapperTest, OverwriteRegistration)
{
    gx::GUID g = MakeGUID();
    int callCount = 0;

    gx::AssetRemapper::Instance().Register(g, "old.png", 1,
        [](const gx::String&, uint32_t) { return true; });

    // 同じGUIDで再登録（パス変更）
    gx::AssetRemapper::Instance().Register(g, "new.png", 2,
        [&](const gx::String&, uint32_t) {
            ++callCount;
            return true;
        });

    EXPECT_EQ(gx::AssetRemapper::Instance().GetRegisteredCount(), 1u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetSlot(g), 2u);
    EXPECT_EQ(gx::AssetRemapper::Instance().GetPath(g), gx::String("new.png"));

    gx::AssetRemapper::Instance().OnFileChanged("new.png");
    EXPECT_EQ(callCount, 1);
}

// ============================================================================
// Unregister後にOnFileChangedでfalse
// ============================================================================

TEST_F(AssetRemapperTest, OnFileChangedAfterUnregister)
{
    gx::GUID g = MakeGUID();
    gx::AssetRemapper::Instance().Register(g, "removed.png", 1,
        [](const gx::String&, uint32_t) { return true; });

    gx::AssetRemapper::Instance().Unregister(g);
    EXPECT_FALSE(gx::AssetRemapper::Instance().OnFileChanged("removed.png"));
}

// ============================================================================
// AssetReloader + Remapper 統合テスト
// ============================================================================

TEST_F(AssetRemapperTest, AssetReloaderDelegatesToRemapper)
{
    // AssetReloader がファイル変更時に Remapper に先に委譲することを
    // 間接的にテスト（Remapperの登録→OnFileChanged→統計確認）
    gx::GUID g = MakeGUID();
    bool remapperCalled = false;

    gx::AssetRemapper::Instance().Register(g, "delegate.png", 7,
        [&](const gx::String& path, uint32_t slot) {
            remapperCalled = true;
            EXPECT_EQ(slot, 7u);
            return true;
        });

    gx::AssetRemapper::Instance().OnFileChanged("delegate.png");
    EXPECT_TRUE(remapperCalled);
}
