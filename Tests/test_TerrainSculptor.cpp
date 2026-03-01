/// @file test_TerrainSculptor.cpp
/// @brief 地形スカルプトツールのテスト

#include <gtest/gtest.h>
#include "Editor/TerrainSculptor.h"
#include <cmath>
#include <numeric>

using namespace gx;

// ============================================================================
// TerrainSculptor テスト
// ============================================================================

class TerrainSculptorTest : public ::testing::Test
{
protected:
    TerrainSculptor sculptor;
    static constexpr int kSize = 16;
    float heightmap[kSize * kSize];

    void SetUp() override
    {
        // 全セルを 0.5 で初期化
        std::fill(std::begin(heightmap), std::end(heightmap), 0.5f);
        sculptor.SetTargetHeightmap(heightmap, kSize, kSize);
    }
};

TEST_F(TerrainSculptorTest, DefaultBrush)
{
    const auto& brush = sculptor.GetBrush();
    EXPECT_FLOAT_EQ(brush.radius, 5.0f);
    EXPECT_FLOAT_EQ(brush.strength, 0.5f);
    EXPECT_FLOAT_EQ(brush.falloff, 0.5f);
    EXPECT_FLOAT_EQ(brush.opacity, 1.0f);
    EXPECT_EQ(brush.type, BrushType::Circle);
}

TEST_F(TerrainSculptorTest, SetBrush)
{
    BrushSettings bs;
    bs.radius   = 10.0f;
    bs.strength = 0.8f;
    bs.type     = BrushType::Gaussian;
    sculptor.SetBrush(bs);

    EXPECT_FLOAT_EQ(sculptor.GetBrush().radius, 10.0f);
    EXPECT_FLOAT_EQ(sculptor.GetBrush().strength, 0.8f);
    EXPECT_EQ(sculptor.GetBrush().type, BrushType::Gaussian);
}

TEST_F(TerrainSculptorTest, DefaultMode)
{
    EXPECT_EQ(sculptor.GetMode(), SculptMode::Raise);
}

TEST_F(TerrainSculptorTest, SetMode)
{
    sculptor.SetMode(SculptMode::Smooth);
    EXPECT_EQ(sculptor.GetMode(), SculptMode::Smooth);

    sculptor.SetMode(SculptMode::Flatten);
    EXPECT_EQ(sculptor.GetMode(), SculptMode::Flatten);
}

TEST_F(TerrainSculptorTest, NoTarget)
{
    TerrainSculptor fresh;
    EXPECT_FALSE(fresh.HasTarget());
    EXPECT_EQ(fresh.GetTargetWidth(), 0);
    EXPECT_EQ(fresh.GetTargetHeight(), 0);

    // ターゲットなしでブラシ適用してもクラッシュしない
    auto action = fresh.ApplyBrush(5.0f, 5.0f);
    EXPECT_TRUE(action.affectedCells.empty());
}

TEST_F(TerrainSculptorTest, SetTarget)
{
    EXPECT_TRUE(sculptor.HasTarget());
    EXPECT_EQ(sculptor.GetTargetWidth(), kSize);
    EXPECT_EQ(sculptor.GetTargetHeight(), kSize);
}

TEST_F(TerrainSculptorTest, HasTarget)
{
    EXPECT_TRUE(sculptor.HasTarget());
}

TEST_F(TerrainSculptorTest, ClearTarget)
{
    sculptor.ClearTarget();
    EXPECT_FALSE(sculptor.HasTarget());
    EXPECT_EQ(sculptor.GetTargetWidth(), 0);
    EXPECT_EQ(sculptor.GetTargetHeight(), 0);
}

TEST_F(TerrainSculptorTest, EvaluateBrushCircle)
{
    BrushSettings bs;
    bs.radius  = 10.0f;
    bs.falloff = 0.5f;
    bs.type    = BrushType::Circle;

    // 中心では1.0
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(0.0f, bs), 1.0f);

    // 半径外では0.0
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(11.0f, bs), 0.0f);

    // 中間距離は0〜1の間
    float mid = TerrainSculptor::EvaluateBrush(7.5f, bs);
    EXPECT_GT(mid, 0.0f);
    EXPECT_LE(mid, 1.0f);
}

TEST_F(TerrainSculptorTest, EvaluateBrushSquare)
{
    BrushSettings bs;
    bs.radius = 10.0f;
    bs.type   = BrushType::Square;

    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(0.0f, bs), 1.0f);
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(5.0f, bs), 1.0f);
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(11.0f, bs), 0.0f);
}

TEST_F(TerrainSculptorTest, EvaluateBrushGaussian)
{
    BrushSettings bs;
    bs.radius  = 10.0f;
    bs.falloff = 0.5f;
    bs.type    = BrushType::Gaussian;

    float center = TerrainSculptor::EvaluateBrush(0.0f, bs);
    float edge   = TerrainSculptor::EvaluateBrush(9.0f, bs);

    EXPECT_GT(center, edge);
    EXPECT_GT(center, 0.9f); // ガウシアンの中心は1に近い
    EXPECT_GT(edge, 0.0f);
}

TEST_F(TerrainSculptorTest, ApplyBrushRaise)
{
    BrushSettings bs;
    bs.radius   = 3.0f;
    bs.strength = 0.1f;
    bs.type     = BrushType::Circle;
    sculptor.SetBrush(bs);
    sculptor.SetMode(SculptMode::Raise);

    float before = heightmap[8 * kSize + 8]; // 中心
    auto action = sculptor.ApplyBrush(8.0f, 8.0f);

    EXPECT_GT(heightmap[8 * kSize + 8], before);
    EXPECT_GT(action.affectedCells.size(), 0u);
}

TEST_F(TerrainSculptorTest, ApplyBrushLower)
{
    BrushSettings bs;
    bs.radius   = 3.0f;
    bs.strength = 0.1f;
    bs.type     = BrushType::Circle;
    sculptor.SetBrush(bs);
    sculptor.SetMode(SculptMode::Lower);

    float before = heightmap[8 * kSize + 8];
    sculptor.ApplyBrush(8.0f, 8.0f);

    EXPECT_LT(heightmap[8 * kSize + 8], before);
}

TEST_F(TerrainSculptorTest, ApplyBrushFlatten)
{
    // 一部を高くする
    heightmap[8 * kSize + 8] = 2.0f;

    BrushSettings bs;
    bs.radius   = 3.0f;
    bs.strength = 1.0f; // 強い適用
    bs.falloff  = 0.0f;
    bs.type     = BrushType::Circle;
    sculptor.SetBrush(bs);
    sculptor.SetMode(SculptMode::Flatten);
    sculptor.SetFlattenHeight(0.5f);

    sculptor.ApplyBrush(8.0f, 8.0f);

    // フラットターゲット 0.5 に近づいているはず
    EXPECT_LT(heightmap[8 * kSize + 8], 2.0f);
}

TEST_F(TerrainSculptorTest, ApplyBrushSmooth)
{
    // スパイクを作成
    heightmap[8 * kSize + 8] = 10.0f;

    BrushSettings bs;
    bs.radius   = 3.0f;
    bs.strength = 1.0f;
    bs.falloff  = 0.0f;
    bs.type     = BrushType::Circle;
    sculptor.SetBrush(bs);
    sculptor.SetMode(SculptMode::Smooth);

    sculptor.ApplyBrush(8.0f, 8.0f);

    // スパイクが周囲の平均に近づいているはず
    EXPECT_LT(heightmap[8 * kSize + 8], 10.0f);
}

TEST_F(TerrainSculptorTest, GetBrushMask)
{
    BrushSettings bs;
    bs.radius = 2.0f;
    bs.type   = BrushType::Circle;
    sculptor.SetBrush(bs);

    auto mask = sculptor.GetBrushMask(8.0f, 8.0f);
    EXPECT_GT(mask.size(), 0u);

    // 全ての重みが 0〜1 の範囲
    for (const auto& [pos, weight] : mask)
    {
        EXPECT_GE(weight, 0.0f);
        EXPECT_LE(weight, 1.0f);
    }
}

TEST_F(TerrainSculptorTest, UndoAction)
{
    BrushSettings bs;
    bs.radius   = 3.0f;
    bs.strength = 0.5f;
    bs.type     = BrushType::Circle;
    sculptor.SetBrush(bs);
    sculptor.SetMode(SculptMode::Raise);

    float before = heightmap[8 * kSize + 8];
    auto action = sculptor.ApplyBrush(8.0f, 8.0f);

    // 値が変わったことを確認
    EXPECT_NE(heightmap[8 * kSize + 8], before);

    // Undo
    sculptor.Undo(action);
    EXPECT_FLOAT_EQ(heightmap[8 * kSize + 8], before);
}

TEST_F(TerrainSculptorTest, HistoryCount)
{
    EXPECT_EQ(sculptor.GetHistoryCount(), 0u);

    BrushSettings bs;
    bs.radius = 2.0f;
    bs.type   = BrushType::Circle;
    sculptor.SetBrush(bs);

    sculptor.ApplyBrush(8.0f, 8.0f);
    EXPECT_EQ(sculptor.GetHistoryCount(), 1u);

    sculptor.ApplyBrush(4.0f, 4.0f);
    EXPECT_EQ(sculptor.GetHistoryCount(), 2u);
}

TEST_F(TerrainSculptorTest, ClearHistory)
{
    BrushSettings bs;
    bs.radius = 2.0f;
    bs.type   = BrushType::Circle;
    sculptor.SetBrush(bs);

    sculptor.ApplyBrush(8.0f, 8.0f);
    sculptor.ApplyBrush(4.0f, 4.0f);
    EXPECT_EQ(sculptor.GetHistoryCount(), 2u);

    sculptor.ClearHistory();
    EXPECT_EQ(sculptor.GetHistoryCount(), 0u);
    EXPECT_FALSE(sculptor.CanUndo());
}

TEST_F(TerrainSculptorTest, SampleHeight)
{
    // 特定セルに値を設定
    heightmap[0 * kSize + 0] = 1.0f;
    heightmap[0 * kSize + 1] = 3.0f;
    heightmap[1 * kSize + 0] = 1.0f;
    heightmap[1 * kSize + 1] = 3.0f;

    // 整数座標ではそのセルの値
    EXPECT_FLOAT_EQ(sculptor.SampleHeight(0.0f, 0.0f), 1.0f);
    EXPECT_FLOAT_EQ(sculptor.SampleHeight(1.0f, 0.0f), 3.0f);

    // 0.5 では中間値
    float mid = sculptor.SampleHeight(0.5f, 0.0f);
    EXPECT_NEAR(mid, 2.0f, 0.01f);
}

TEST_F(TerrainSculptorTest, ComputeNormals)
{
    // 平坦なハイトマップ（全て 0.5）
    auto normals = TerrainSculptor::ComputeNormals(heightmap, kSize, kSize);
    ASSERT_EQ(normals.size(), static_cast<size_t>(kSize * kSize));

    // 平坦面の法線は概ね (0, 1, 0)
    for (const auto& n : normals)
    {
        EXPECT_NEAR(n.y, 1.0f, 0.1f);
        EXPECT_NEAR(n.x, 0.0f, 0.1f);
        EXPECT_NEAR(n.z, 0.0f, 0.1f);
    }
}

TEST_F(TerrainSculptorTest, PaintLayer)
{
    PaintLayer layer;
    layer.layerIndex = 1;
    layer.textureId  = 42;
    layer.name       = "Grass";
    sculptor.SetPaintLayer(layer);

    const auto& result = sculptor.GetPaintLayer();
    EXPECT_EQ(result.layerIndex, 1u);
    EXPECT_EQ(result.textureId, 42);
    EXPECT_EQ(result.name, "Grass");
}

TEST_F(TerrainSculptorTest, BrushFalloff)
{
    BrushSettings bs;
    bs.radius  = 10.0f;
    bs.falloff = 0.0f; // フォールオフなし
    bs.type    = BrushType::Circle;

    // フォールオフ 0 の場合、半径内すべてが 1.0
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(5.0f, bs), 1.0f);
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(9.9f, bs), 1.0f);

    // 半径外
    EXPECT_FLOAT_EQ(TerrainSculptor::EvaluateBrush(10.1f, bs), 0.0f);
}

TEST_F(TerrainSculptorTest, BrushRadius)
{
    BrushSettings bs;
    bs.radius = 1.0f;
    bs.type   = BrushType::Circle;
    sculptor.SetBrush(bs);

    auto mask = sculptor.GetBrushMask(8.0f, 8.0f);
    // 半径1のブラシは限られたセルだけ影響
    EXPECT_GT(mask.size(), 0u);
    EXPECT_LE(mask.size(), 9u); // 最大で3x3
}

TEST_F(TerrainSculptorTest, SampleHeightNoTarget)
{
    TerrainSculptor fresh;
    EXPECT_FLOAT_EQ(fresh.SampleHeight(0.0f, 0.0f), 0.0f);
}

TEST_F(TerrainSculptorTest, CanUndo)
{
    EXPECT_FALSE(sculptor.CanUndo());

    BrushSettings bs;
    bs.radius = 2.0f;
    bs.type   = BrushType::Circle;
    sculptor.SetBrush(bs);
    sculptor.ApplyBrush(8.0f, 8.0f);

    EXPECT_TRUE(sculptor.CanUndo());
}
