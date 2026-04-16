/// @file test_BuoyancySystem.cpp
/// @brief BuoyancySystem 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Physics/BuoyancySystem.h"

using namespace gx;

// ==================== 水面構成 ====================

TEST(BuoyancySystemTest, DefaultWaterConfig)
{
    BuoyancySystem sys;
    const auto& cfg = sys.GetWaterConfig();
    EXPECT_NEAR(cfg.waterLevel, 0.0f, 1e-5f);
    EXPECT_NEAR(cfg.density, 1000.0f, 1e-3f);
    EXPECT_NEAR(cfg.waveAmplitude, 0.5f, 1e-5f);
    EXPECT_NEAR(cfg.dragCoefficient, 0.5f, 1e-5f);
}

TEST(BuoyancySystemTest, SetWaterConfig)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waterLevel    = 5.0f;
    cfg.density       = 1025.0f; // 海水
    cfg.waveAmplitude = 1.0f;
    sys.SetWaterConfig(cfg);
    EXPECT_NEAR(sys.GetWaterConfig().waterLevel, 5.0f, 1e-5f);
    EXPECT_NEAR(sys.GetWaterConfig().density, 1025.0f, 1e-3f);
}

// ==================== ボディ管理 ====================

TEST(BuoyancySystemTest, AddBody)
{
    BuoyancySystem sys;
    sys.AddBody(1, 100.0f, 0.5f, { Vector3(0, -0.5f, 0), Vector3(0, 0.5f, 0) });
    EXPECT_EQ(sys.GetBodyCount(), 1u);
}

TEST(BuoyancySystemTest, RemoveBody)
{
    BuoyancySystem sys;
    sys.AddBody(1, 100.0f, 0.5f, {});
    sys.AddBody(2, 200.0f, 1.0f, {});
    EXPECT_EQ(sys.GetBodyCount(), 2u);
    sys.RemoveBody(1);
    EXPECT_EQ(sys.GetBodyCount(), 1u);
    EXPECT_FALSE(sys.HasBody(1));
    EXPECT_TRUE(sys.HasBody(2));
}

TEST(BuoyancySystemTest, BodyCount)
{
    BuoyancySystem sys;
    EXPECT_EQ(sys.GetBodyCount(), 0u);
    sys.AddBody(1, 10.0f, 0.1f, {});
    sys.AddBody(2, 20.0f, 0.2f, {});
    sys.AddBody(3, 30.0f, 0.3f, {});
    EXPECT_EQ(sys.GetBodyCount(), 3u);
}

TEST(BuoyancySystemTest, HasBody)
{
    BuoyancySystem sys;
    EXPECT_FALSE(sys.HasBody(42));
    sys.AddBody(42, 10.0f, 0.1f, {});
    EXPECT_TRUE(sys.HasBody(42));
    EXPECT_FALSE(sys.HasBody(99));
}

TEST(BuoyancySystemTest, GetBody)
{
    BuoyancySystem sys;
    sys.AddBody(1, 100.0f, 0.5f, { Vector3(0, 0, 0) });
    const auto* body = sys.GetBody(1);
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->id, 1u);
    EXPECT_NEAR(body->mass, 100.0f, 1e-3f);
    EXPECT_NEAR(body->volume, 0.5f, 1e-3f);

    EXPECT_EQ(sys.GetBody(999), nullptr);
}

// ==================== 水面高さ ====================

TEST(BuoyancySystemTest, WaterHeightFlat)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f; // 波なし
    cfg.waterLevel    = 3.0f;
    sys.SetWaterConfig(cfg);

    float h = sys.GetWaterHeight(5.0f, 5.0f, 0.0f);
    EXPECT_NEAR(h, 3.0f, 1e-5f);
}

TEST(BuoyancySystemTest, WaterHeightWave)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 1.0f;
    cfg.waveFrequency = 1.0f;
    cfg.waveSpeed     = 1.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    // 波がある場合、異なる座標/時刻で高さが異なるべき
    float h1 = sys.GetWaterHeight(0.0f, 0.0f, 0.0f);
    float h2 = sys.GetWaterHeight(3.14f, 0.0f, 0.0f);
    // 両者が異なることを確認（厳密に同じになる確率は極めて低い）
    EXPECT_NE(h1, h2);
}

TEST(BuoyancySystemTest, WaterNormal)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f; // 波なし
    sys.SetWaterConfig(cfg);

    Vector3 normal = sys.GetWaterNormal(0.0f, 0.0f, 0.0f);
    // 波なしでは法線は真上
    EXPECT_NEAR(normal.x, 0.0f, 1e-3f);
    EXPECT_NEAR(normal.y, 1.0f, 1e-3f);
    EXPECT_NEAR(normal.z, 0.0f, 1e-3f);
}

// ==================== 浸水率 ====================

TEST(BuoyancySystemTest, SubmergedFractionAbove)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    sys.AddBody(1, 100.0f, 1.0f, { Vector3(0, 0, 0) });
    sys.SetBodyTransform(1, Vector3(0, 10, 0), Quaternion::Identity()); // 水面より上

    auto force = sys.ComputeBuoyancy(1, 0.0f);
    EXPECT_NEAR(force.submergedFraction, 0.0f, 1e-5f);
}

TEST(BuoyancySystemTest, SubmergedFractionBelow)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    sys.AddBody(1, 100.0f, 1.0f, { Vector3(0, 0, 0) });
    sys.SetBodyTransform(1, Vector3(0, -10, 0), Quaternion::Identity()); // 水面より深い

    auto force = sys.ComputeBuoyancy(1, 0.0f);
    EXPECT_GT(force.submergedFraction, 0.0f);
}

TEST(BuoyancySystemTest, SubmergedFractionPartial)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    // 上下にサンプルポイントを配置
    sys.AddBody(1, 100.0f, 1.0f,
        { Vector3(0, -0.5f, 0), Vector3(0, 0.5f, 0) });
    sys.SetBodyTransform(1, Vector3(0, 0, 0), Quaternion::Identity()); // 水面にまたがる

    auto force = sys.ComputeBuoyancy(1, 0.0f);
    // 下のサンプルは水中、上のサンプルは水面上 → 約50%
    EXPECT_GT(force.submergedFraction, 0.0f);
    EXPECT_LT(force.submergedFraction, 1.0f);
}

// ==================== 浮力計算 ====================

TEST(BuoyancySystemTest, ComputeBuoyancyFloating)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    sys.AddBody(1, 100.0f, 1.0f, { Vector3(0, 0, 0) });
    sys.SetBodyTransform(1, Vector3(0, -0.5f, 0), Quaternion::Identity());

    auto result = sys.ComputeBuoyancy(1, 0.0f);
    // 浮力は上向きであるべき
    EXPECT_GT(result.force.y, 0.0f);
}

TEST(BuoyancySystemTest, ComputeBuoyancySinking)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 10.0f; // 深い水
    sys.SetWaterConfig(cfg);

    sys.AddBody(1, 100.0f, 0.01f, { Vector3(0, 0, 0) }); // 小さな体積 → 浮力小
    sys.SetBodyTransform(1, Vector3(0, 0, 0), Quaternion::Identity());

    auto result = sys.ComputeBuoyancy(1, 0.0f);
    // 体積が小さいので浮力も小さい
    // ただし浮力自体は発生する
    EXPECT_GT(result.force.y, 0.0f);
}

TEST(BuoyancySystemTest, DragForce)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude    = 0.0f;
    cfg.waterLevel       = 10.0f;
    cfg.dragCoefficient  = 2.0f;
    sys.SetWaterConfig(cfg);

    sys.AddBody(1, 100.0f, 1.0f, { Vector3(0, 0, 0) });
    sys.SetBodyTransform(1, Vector3(0, 0, 0), Quaternion::Identity());
    sys.SetBodyVelocity(1, Vector3(10, 0, 0), Vector3(0, 0, 0)); // 水中で高速移動

    auto result = sys.ComputeBuoyancy(1, 0.0f);
    // ドラッグは速度と逆方向
    EXPECT_LT(result.force.x, 0.0f); // X方向のドラッグ
}

// ==================== 積分 ====================

TEST(BuoyancySystemTest, UpdateIntegration)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    sys.AddBody(1, 100.0f, 2.0f, { Vector3(0, 0, 0) });
    sys.SetBodyTransform(1, Vector3(0, -1, 0), Quaternion::Identity());

    Vector3 posBefore = sys.GetBody(1)->position;
    sys.Update(1.0f / 60.0f, 0.0f);
    Vector3 posAfter = sys.GetBody(1)->position;

    // 積分後に位置が変化しているはず
    float moved = (posAfter - posBefore).Length();
    EXPECT_GT(moved, 0.0f);
}

TEST(BuoyancySystemTest, ClearBodies)
{
    BuoyancySystem sys;
    sys.AddBody(1, 10.0f, 0.1f, {});
    sys.AddBody(2, 20.0f, 0.2f, {});
    EXPECT_EQ(sys.GetBodyCount(), 2u);
    sys.Clear();
    EXPECT_EQ(sys.GetBodyCount(), 0u);
}

TEST(BuoyancySystemTest, GravitySetting)
{
    BuoyancySystem sys;
    EXPECT_NEAR(sys.GetGravity(), 9.81f, 1e-3f);
    sys.SetGravity(1.62f); // 月
    EXPECT_NEAR(sys.GetGravity(), 1.62f, 1e-3f);
}

TEST(BuoyancySystemTest, MultipleBodySimulation)
{
    BuoyancySystem sys;
    WaterSurfaceConfig cfg;
    cfg.waveAmplitude = 0.0f;
    cfg.waterLevel    = 0.0f;
    sys.SetWaterConfig(cfg);

    // 軽い物体（浮く）と重い物体（沈む）
    sys.AddBody(1, 10.0f, 2.0f, { Vector3(0, 0, 0) });   // 軽い / 大きい体積
    sys.AddBody(2, 5000.0f, 0.1f, { Vector3(0, 0, 0) }); // 重い / 小さい体積

    sys.SetBodyTransform(1, Vector3(-5, -0.5f, 0), Quaternion::Identity());
    sys.SetBodyTransform(2, Vector3(5, -0.5f, 0), Quaternion::Identity());

    // 何ステップかシミュレーション
    for (int i = 0; i < 120; ++i)
        sys.Update(1.0f / 60.0f, static_cast<float>(i) / 60.0f);

    // 軽い物体は浮上、重い物体は沈降する傾向
    const auto* light = sys.GetBody(1);
    const auto* heavy = sys.GetBody(2);
    ASSERT_NE(light, nullptr);
    ASSERT_NE(heavy, nullptr);

    // 軽い物体のY座標 > 重い物体のY座標
    EXPECT_GT(light->position.y, heavy->position.y);
}
