/// @file test_AssetReloader.cpp
/// @brief AssetReloader（ホットリロード拡張）のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/AssetReloader.h"

using namespace gx;

// ============================================================================
// デフォルト状態
// ============================================================================

TEST(AssetReloaderTest, DefaultNotInitialized)
{
    AssetReloader reloader;
    // 未初期化だが、自動リロードはデフォルトで有効
    EXPECT_TRUE(reloader.IsAutoReloadEnabled());
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 0u);
    EXPECT_EQ(reloader.GetRegisteredMaterialCount(), 0u);
    EXPECT_EQ(reloader.GetRegisteredScriptCount(), 0u);
    EXPECT_EQ(reloader.GetReloadSuccessCount(), 0u);
    EXPECT_EQ(reloader.GetReloadFailureCount(), 0u);
}

// ============================================================================
// nullptrで初期化
// ============================================================================

TEST(AssetReloaderTest, InitializeWithNullReturnsFalse)
{
    AssetReloader reloader;
    bool result = reloader.Initialize(nullptr);
    EXPECT_FALSE(result);
}

// ============================================================================
// テクスチャ登録
// ============================================================================

TEST(AssetReloaderTest, RegisterTextureIncreasesCount)
{
    AssetReloader reloader;
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 0u);
    reloader.RegisterTexture(L"textures/grass.png", 1);
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 1u);
    reloader.RegisterTexture(L"textures/stone.dds", 2);
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 2u);
}

// ============================================================================
// マテリアル登録
// ============================================================================

TEST(AssetReloaderTest, RegisterMaterialIncreasesCount)
{
    AssetReloader reloader;
    EXPECT_EQ(reloader.GetRegisteredMaterialCount(), 0u);
    reloader.RegisterMaterial(L"materials/wood.mat", "wood");
    EXPECT_EQ(reloader.GetRegisteredMaterialCount(), 1u);
    reloader.RegisterMaterial(L"materials/metal.mat", "metal");
    EXPECT_EQ(reloader.GetRegisteredMaterialCount(), 2u);
}

// ============================================================================
// スクリプト登録
// ============================================================================

TEST(AssetReloaderTest, RegisterScriptIncreasesCount)
{
    AssetReloader reloader;
    EXPECT_EQ(reloader.GetRegisteredScriptCount(), 0u);
    reloader.RegisterScript(L"scripts/player.lua", "player");
    EXPECT_EQ(reloader.GetRegisteredScriptCount(), 1u);
}

// ============================================================================
// 自動リロードの有効/無効
// ============================================================================

TEST(AssetReloaderTest, SetAutoReloadEnabled)
{
    AssetReloader reloader;
    EXPECT_TRUE(reloader.IsAutoReloadEnabled());
    reloader.SetAutoReloadEnabled(false);
    EXPECT_FALSE(reloader.IsAutoReloadEnabled());
    reloader.SetAutoReloadEnabled(true);
    EXPECT_TRUE(reloader.IsAutoReloadEnabled());
}

// ============================================================================
// リロードカウンター
// ============================================================================

TEST(AssetReloaderTest, ReloadCountsInitiallyZero)
{
    AssetReloader reloader;
    EXPECT_EQ(reloader.GetReloadSuccessCount(), 0u);
    EXPECT_EQ(reloader.GetReloadFailureCount(), 0u);
}

// ============================================================================
// シャットダウンで状態がリセットされる
// ============================================================================

TEST(AssetReloaderTest, ShutdownResetsState)
{
    AssetReloader reloader;
    reloader.RegisterTexture(L"textures/a.png", 1);
    reloader.RegisterMaterial(L"materials/b.mat", "b");
    reloader.RegisterScript(L"scripts/c.lua", "c");

    // シャットダウンですべてクリアされる（Initializeなしでも）
    reloader.Shutdown();
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 0u);
    EXPECT_EQ(reloader.GetRegisteredMaterialCount(), 0u);
    EXPECT_EQ(reloader.GetRegisteredScriptCount(), 0u);
    EXPECT_EQ(reloader.GetReloadSuccessCount(), 0u);
    EXPECT_EQ(reloader.GetReloadFailureCount(), 0u);
}

// ============================================================================
// SetOnReloadコールバック
// ============================================================================

TEST(AssetReloaderTest, SetOnReloadCallback)
{
    AssetReloader reloader;
    bool callbackSet = false;
    reloader.SetOnReload([&callbackSet](const ReloadEvent&) {
        callbackSet = true;
    });
    // コールバックは設定されたが、HotReloadManagerなしではトリガーできない。
    // クラッシュしないことと基本的な状態の整合性を確認。
    EXPECT_EQ(reloader.GetReloadSuccessCount(), 0u);
}

// ============================================================================
// テクスチャマネージャー設定
// ============================================================================

TEST(AssetReloaderTest, SetTextureManager)
{
    AssetReloader reloader;
    // nullptrを設定してもクラッシュしないこと
    reloader.SetTextureManager(nullptr);
    // Initializeなしではポインタが保存されたか確認できないが、
    // クラッシュしないことと状態が有効であることを確認。
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 0u);
}

// ============================================================================
// 同一パスの複数登録は上書きされる
// ============================================================================

TEST(AssetReloaderTest, DuplicateTextureRegistrationOverwrites)
{
    AssetReloader reloader;
    reloader.RegisterTexture(L"textures/dup.png", 1);
    reloader.RegisterTexture(L"textures/dup.png", 2); // 同じパス、異なるハンドル
    // マップはパスをキーとして使うため、カウントは1のまま
    EXPECT_EQ(reloader.GetRegisteredTextureCount(), 1u);
}
