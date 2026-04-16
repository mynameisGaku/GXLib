/// @file test_DecalEnhanced.cpp
/// @brief デカール拡張機能のテスト（PBRデカール・ブレンドモード・距離フェード）
///
/// 既存の Decal.h/.cpp を変更せず、拡張デカルデータ構造を
/// スタンドアロンで定義してテストする。

#include "pch.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <vector>

// 拡張デカールデータ構造（ローカル定義）
namespace {

struct EnhancedDecalData {
    float normalStrength = 1.0f;        ///< 法線マップの影響強度
    float roughnessMultiplier = 1.0f;   ///< ラフネス乗算係数
    float metallicOverride = -1.0f;     ///< メタリック上書き値 (-1=無効)
    float fadeStartDistance = 20.0f;     ///< フェード開始距離
    float fadeEndDistance = 30.0f;       ///< フェード終了距離
    enum BlendMode { Alpha, Additive, Multiply } blendMode = Alpha;
    uint8_t priority = 128;             ///< 描画優先度 (0=最優先, 255=最低)
    int normalMapHandle = -1;           ///< 法線マップテクスチャハンドル
    int roughnessMapHandle = -1;        ///< ラフネスマップテクスチャハンドル
};

static constexpr float kEps = 1e-5f;

} // anonymous namespace

// ============================================================================
// DefaultNewFields: 拡張フィールドのデフォルト値
// ============================================================================
TEST(DecalEnhancedTest, DefaultNewFields)
{
    EnhancedDecalData decal;
    EXPECT_NEAR(decal.normalStrength, 1.0f, kEps);
    EXPECT_NEAR(decal.roughnessMultiplier, 1.0f, kEps);
    EXPECT_NEAR(decal.metallicOverride, -1.0f, kEps);
    EXPECT_NEAR(decal.fadeStartDistance, 20.0f, kEps);
    EXPECT_NEAR(decal.fadeEndDistance, 30.0f, kEps);
    EXPECT_EQ(decal.blendMode, EnhancedDecalData::Alpha);
    EXPECT_EQ(decal.priority, 128);
    EXPECT_EQ(decal.normalMapHandle, -1);
    EXPECT_EQ(decal.roughnessMapHandle, -1);
}

// ============================================================================
// SetNormalMap: 法線マップハンドル設定
// ============================================================================
TEST(DecalEnhancedTest, SetNormalMap)
{
    EnhancedDecalData decal;
    decal.normalMapHandle = 42;
    EXPECT_EQ(decal.normalMapHandle, 42);
}

// ============================================================================
// SetRoughnessMap: ラフネスマップハンドル設定
// ============================================================================
TEST(DecalEnhancedTest, SetRoughnessMap)
{
    EnhancedDecalData decal;
    decal.roughnessMapHandle = 7;
    EXPECT_EQ(decal.roughnessMapHandle, 7);
}

// ============================================================================
// BlendModeAlpha: Alphaブレンドモード
// ============================================================================
TEST(DecalEnhancedTest, BlendModeAlpha)
{
    EnhancedDecalData decal;
    decal.blendMode = EnhancedDecalData::Alpha;
    EXPECT_EQ(decal.blendMode, EnhancedDecalData::Alpha);
    EXPECT_EQ(static_cast<int>(decal.blendMode), 0);
}

// ============================================================================
// BlendModeAdditive: Additiveブレンドモード
// ============================================================================
TEST(DecalEnhancedTest, BlendModeAdditive)
{
    EnhancedDecalData decal;
    decal.blendMode = EnhancedDecalData::Additive;
    EXPECT_EQ(decal.blendMode, EnhancedDecalData::Additive);
    EXPECT_EQ(static_cast<int>(decal.blendMode), 1);
}

// ============================================================================
// BlendModeMultiply: Multiplyブレンドモード
// ============================================================================
TEST(DecalEnhancedTest, BlendModeMultiply)
{
    EnhancedDecalData decal;
    decal.blendMode = EnhancedDecalData::Multiply;
    EXPECT_EQ(decal.blendMode, EnhancedDecalData::Multiply);
    EXPECT_EQ(static_cast<int>(decal.blendMode), 2);
}

// ============================================================================
// PrioritySort: 優先度によるソート
// ============================================================================
TEST(DecalEnhancedTest, PrioritySort)
{
    gx::Vector<EnhancedDecalData> decals(4);
    decals[0].priority = 200;
    decals[1].priority = 50;
    decals[2].priority = 128;
    decals[3].priority = 10;

    // 優先度昇順ソート (0が最優先)
    std::sort(decals.begin(), decals.end(),
              [](const EnhancedDecalData& a, const EnhancedDecalData& b) {
                  return a.priority < b.priority;
              });

    EXPECT_EQ(decals[0].priority, 10);
    EXPECT_EQ(decals[1].priority, 50);
    EXPECT_EQ(decals[2].priority, 128);
    EXPECT_EQ(decals[3].priority, 200);
}

// ============================================================================
// DistanceFade: 距離フェード計算
// ============================================================================
TEST(DecalEnhancedTest, DistanceFade)
{
    EnhancedDecalData decal;
    decal.fadeStartDistance = 20.0f;
    decal.fadeEndDistance = 30.0f;

    // フェード計算ロジック
    auto calcFade = [](const EnhancedDecalData& d, float distance) -> float {
        if (distance <= d.fadeStartDistance) return 1.0f;
        if (distance >= d.fadeEndDistance) return 0.0f;
        return 1.0f - (distance - d.fadeStartDistance) / (d.fadeEndDistance - d.fadeStartDistance);
    };

    // フェード開始前: 完全不透明
    EXPECT_NEAR(calcFade(decal, 10.0f), 1.0f, kEps);

    // フェード開始地点: 完全不透明
    EXPECT_NEAR(calcFade(decal, 20.0f), 1.0f, kEps);

    // フェード中間地点: 半透明
    EXPECT_NEAR(calcFade(decal, 25.0f), 0.5f, kEps);

    // フェード終了地点: 完全透明
    EXPECT_NEAR(calcFade(decal, 30.0f), 0.0f, kEps);

    // フェード終了後: 完全透明
    EXPECT_NEAR(calcFade(decal, 50.0f), 0.0f, kEps);
}

// ============================================================================
// NormalStrength: 法線強度設定
// ============================================================================
TEST(DecalEnhancedTest, NormalStrength)
{
    EnhancedDecalData decal;

    // デフォルト: フル強度
    EXPECT_NEAR(decal.normalStrength, 1.0f, kEps);

    // 弱い法線マップ
    decal.normalStrength = 0.3f;
    EXPECT_NEAR(decal.normalStrength, 0.3f, kEps);

    // 強い法線マップ
    decal.normalStrength = 2.0f;
    EXPECT_NEAR(decal.normalStrength, 2.0f, kEps);

    // 無効化
    decal.normalStrength = 0.0f;
    EXPECT_NEAR(decal.normalStrength, 0.0f, kEps);
}

// ============================================================================
// MetallicOverride: メタリック上書き
// ============================================================================
TEST(DecalEnhancedTest, MetallicOverride)
{
    EnhancedDecalData decal;

    // デフォルト: 無効 (-1)
    EXPECT_NEAR(decal.metallicOverride, -1.0f, kEps);
    bool isDisabled = (decal.metallicOverride < 0.0f);
    EXPECT_TRUE(isDisabled);

    // 有効化: メタリック素材
    decal.metallicOverride = 0.9f;
    EXPECT_NEAR(decal.metallicOverride, 0.9f, kEps);
    bool isEnabled = (decal.metallicOverride >= 0.0f);
    EXPECT_TRUE(isEnabled);

    // 有効化: 非メタリック素材
    decal.metallicOverride = 0.0f;
    EXPECT_NEAR(decal.metallicOverride, 0.0f, kEps);
    isEnabled = (decal.metallicOverride >= 0.0f);
    EXPECT_TRUE(isEnabled);
}
