/// @file test_ReverbZone.cpp
/// @brief ReverbZoneManager のユニットテスト（14テスト）

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/ReverbZone.h"
#include <cmath>

using namespace gx;

// ============================================================================
// ヘルパー
// ============================================================================

static ReverbZoneConfig MakeZone(float x, float y, float z,
                                  float innerR = 5.0f, float outerR = 15.0f,
                                  int priority = 0, const gx::String& name = "zone")
{
    ReverbZoneConfig cfg;
    cfg.position[0] = x;
    cfg.position[1] = y;
    cfg.position[2] = z;
    cfg.innerRadius = innerR;
    cfg.outerRadius = outerR;
    cfg.priority = priority;
    cfg.name = name;
    return cfg;
}

// ============================================================================
// テスト
// ============================================================================

/// 構築と破棄が安全に行えること
TEST(ReverbZoneTest, ConstructDestruct)
{
    ReverbZoneManager mgr;
    EXPECT_EQ(mgr.GetZoneCount(), 0u);
    EXPECT_EQ(mgr.GetActiveZoneId(), 0u);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 0.0f);
    EXPECT_TRUE(mgr.IsEnabled());
}

/// ゾーン追加で一意のIDが返ること
TEST(ReverbZoneTest, AddZone)
{
    ReverbZoneManager mgr;
    uint32_t id1 = mgr.AddZone(MakeZone(0, 0, 0));
    uint32_t id2 = mgr.AddZone(MakeZone(10, 0, 0));
    EXPECT_NE(id1, 0u);
    EXPECT_NE(id2, 0u);
    EXPECT_NE(id1, id2);
    EXPECT_EQ(mgr.GetZoneCount(), 2u);
}

/// ゾーン削除が機能すること
TEST(ReverbZoneTest, RemoveZone)
{
    ReverbZoneManager mgr;
    uint32_t id1 = mgr.AddZone(MakeZone(0, 0, 0));
    uint32_t id2 = mgr.AddZone(MakeZone(10, 0, 0));
    EXPECT_EQ(mgr.GetZoneCount(), 2u);

    mgr.RemoveZone(id1);
    EXPECT_EQ(mgr.GetZoneCount(), 1u);
    EXPECT_EQ(mgr.GetZone(id1), nullptr);
    EXPECT_NE(mgr.GetZone(id2), nullptr);

    // 存在しないIDの削除は安全に無視される
    mgr.RemoveZone(999);
    EXPECT_EQ(mgr.GetZoneCount(), 1u);
}

/// ゾーン設定の更新が反映されること
TEST(ReverbZoneTest, UpdateZone)
{
    ReverbZoneManager mgr;
    uint32_t id = mgr.AddZone(MakeZone(0, 0, 0, 5.0f, 15.0f, 0, "original"));

    const ReverbZoneConfig* cfg = mgr.GetZone(id);
    ASSERT_NE(cfg, nullptr);
    EXPECT_EQ(cfg->name, "original");
    EXPECT_FLOAT_EQ(cfg->innerRadius, 5.0f);

    ReverbZoneConfig updated = *cfg;
    updated.name = "updated";
    updated.innerRadius = 10.0f;
    mgr.UpdateZone(id, updated);

    cfg = mgr.GetZone(id);
    ASSERT_NE(cfg, nullptr);
    EXPECT_EQ(cfg->name, "updated");
    EXPECT_FLOAT_EQ(cfg->innerRadius, 10.0f);
}

/// GetZoneが正しく設定を返すこと
TEST(ReverbZoneTest, GetZone)
{
    ReverbZoneManager mgr;
    uint32_t id = mgr.AddZone(MakeZone(1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 5, "test"));

    const ReverbZoneConfig* cfg = mgr.GetZone(id);
    ASSERT_NE(cfg, nullptr);
    EXPECT_FLOAT_EQ(cfg->position[0], 1.0f);
    EXPECT_FLOAT_EQ(cfg->position[1], 2.0f);
    EXPECT_FLOAT_EQ(cfg->position[2], 3.0f);
    EXPECT_FLOAT_EQ(cfg->innerRadius, 4.0f);
    EXPECT_FLOAT_EQ(cfg->outerRadius, 8.0f);
    EXPECT_EQ(cfg->priority, 5);
    EXPECT_EQ(cfg->name, "test");

    // 存在しないIDはnullptr
    EXPECT_EQ(mgr.GetZone(999), nullptr);
}

/// ゾーン数が正しく報告されること
TEST(ReverbZoneTest, ZoneCount)
{
    ReverbZoneManager mgr;
    EXPECT_EQ(mgr.GetZoneCount(), 0u);

    uint32_t id1 = mgr.AddZone(MakeZone(0, 0, 0));
    EXPECT_EQ(mgr.GetZoneCount(), 1u);

    uint32_t id2 = mgr.AddZone(MakeZone(10, 0, 0));
    uint32_t id3 = mgr.AddZone(MakeZone(20, 0, 0));
    EXPECT_EQ(mgr.GetZoneCount(), 3u);

    mgr.RemoveZone(id2);
    EXPECT_EQ(mgr.GetZoneCount(), 2u);

    mgr.RemoveZone(id1);
    mgr.RemoveZone(id3);
    EXPECT_EQ(mgr.GetZoneCount(), 0u);
}

/// リスナーが内径内にいる場合、blend=1.0
TEST(ReverbZoneTest, UpdateInsideInner)
{
    ReverbZoneManager mgr;
    uint32_t id = mgr.AddZone(MakeZone(0, 0, 0, 10.0f, 20.0f));

    // リスナーが原点（ゾーン中心と一致）
    mgr.Update(0.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), id);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 1.0f);

    // 内径ギリギリ内
    mgr.Update(9.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 1.0f);
}

/// リスナーが内径〜外径の間にいる場合、0<blend<1
TEST(ReverbZoneTest, UpdateBetweenRadii)
{
    ReverbZoneManager mgr;
    uint32_t id = mgr.AddZone(MakeZone(0, 0, 0, 10.0f, 20.0f));

    // ちょうど中間点: dist=15, blend = 1 - (15-10)/(20-10) = 0.5
    mgr.Update(15.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), id);
    EXPECT_NEAR(mgr.GetCurrentBlendFactor(), 0.5f, 0.001f);

    // 外径寄り: dist=18, blend = 1 - (18-10)/(20-10) = 0.2
    mgr.Update(18.0f, 0.0f, 0.0f);
    EXPECT_NEAR(mgr.GetCurrentBlendFactor(), 0.2f, 0.001f);

    // 内径寄り: dist=12, blend = 1 - (12-10)/(20-10) = 0.8
    mgr.Update(12.0f, 0.0f, 0.0f);
    EXPECT_NEAR(mgr.GetCurrentBlendFactor(), 0.8f, 0.001f);
}

/// リスナーが外径の外にいる場合、blend=0.0
TEST(ReverbZoneTest, UpdateOutside)
{
    ReverbZoneManager mgr;
    mgr.AddZone(MakeZone(0, 0, 0, 5.0f, 10.0f));

    mgr.Update(50.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), 0u);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 0.0f);
}

/// アクティブゾーンIDが最も高いブレンドのゾーンになること
TEST(ReverbZoneTest, ActiveZoneId)
{
    ReverbZoneManager mgr;
    uint32_t idA = mgr.AddZone(MakeZone(0, 0, 0, 5.0f, 15.0f, 0, "A"));
    uint32_t idB = mgr.AddZone(MakeZone(20, 0, 0, 5.0f, 15.0f, 0, "B"));

    // ゾーンAの中心に近い
    mgr.Update(1.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), idA);

    // ゾーンBの中心に近い
    mgr.Update(19.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), idB);

    // 両方の外側
    mgr.Update(100.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), 0u);
}

/// 最大アクティブゾーン数の設定と取得
TEST(ReverbZoneTest, MaxActiveZones)
{
    ReverbZoneManager mgr;
    EXPECT_EQ(mgr.GetMaxActiveZones(), 2);

    mgr.SetMaxActiveZones(4);
    EXPECT_EQ(mgr.GetMaxActiveZones(), 4);

    mgr.SetMaxActiveZones(1);
    EXPECT_EQ(mgr.GetMaxActiveZones(), 1);

    // 0以下は1にクランプ
    mgr.SetMaxActiveZones(0);
    EXPECT_EQ(mgr.GetMaxActiveZones(), 1);

    mgr.SetMaxActiveZones(-5);
    EXPECT_EQ(mgr.GetMaxActiveZones(), 1);
}

/// 同じブレンド値の場合、優先度が高いゾーンがアクティブになること
TEST(ReverbZoneTest, PriorityOverlap)
{
    ReverbZoneManager mgr;
    // 同じ位置・同じ半径だがpriorityが異なる2ゾーン
    uint32_t idLow = mgr.AddZone(MakeZone(0, 0, 0, 10.0f, 20.0f, 1, "low"));
    uint32_t idHigh = mgr.AddZone(MakeZone(0, 0, 0, 10.0f, 20.0f, 5, "high"));

    // 中心にいるとblend=1.0で同値、priority=5のhighが勝つ
    mgr.Update(0.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), idHigh);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 1.0f);
}

/// 有効/無効切り替えが機能すること
TEST(ReverbZoneTest, EnableDisable)
{
    ReverbZoneManager mgr;
    uint32_t id = mgr.AddZone(MakeZone(0, 0, 0, 10.0f, 20.0f));

    mgr.Update(0.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), id);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 1.0f);

    // 無効化するとアクティブゾーンがリセットされる
    mgr.SetEnabled(false);
    EXPECT_FALSE(mgr.IsEnabled());
    EXPECT_EQ(mgr.GetActiveZoneId(), 0u);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 0.0f);

    // 無効状態でUpdateしてもアクティブにならない
    mgr.Update(0.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), 0u);

    // 再有効化してUpdateすると復帰
    mgr.SetEnabled(true);
    EXPECT_TRUE(mgr.IsEnabled());
    mgr.Update(0.0f, 0.0f, 0.0f);
    EXPECT_EQ(mgr.GetActiveZoneId(), id);
}

/// ブレンド係数の精度を確認（3D距離計算）
TEST(ReverbZoneTest, BlendFactor)
{
    ReverbZoneManager mgr;
    mgr.AddZone(MakeZone(0, 0, 0, 0.0f, 10.0f));

    // 3D距離: sqrt(3^2 + 4^2) = 5.0, blend = 1 - 5/10 = 0.5
    mgr.Update(3.0f, 4.0f, 0.0f);
    EXPECT_NEAR(mgr.GetCurrentBlendFactor(), 0.5f, 0.001f);

    // 3D距離: sqrt(1^2 + 2^2 + 2^2) = 3.0, blend = 1 - 3/10 = 0.7
    mgr.Update(1.0f, 2.0f, 2.0f);
    EXPECT_NEAR(mgr.GetCurrentBlendFactor(), 0.7f, 0.001f);

    // ちょうど外径上: dist=10, blend=0
    mgr.Update(10.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(mgr.GetCurrentBlendFactor(), 0.0f);
}
