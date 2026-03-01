/// @file test_Noise.cpp
/// @brief Noiseクラスのユニットテスト（Perlin、FBM、Turbulence、Ridge、Remap）

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Noise.h"

using namespace gx;

// ============================================================================
// Perlin1D
// ============================================================================

TEST(NoiseTest, Perlin1D_Deterministic)
{
    float a = Noise::Perlin1D(1.23f);
    float b = Noise::Perlin1D(1.23f);
    EXPECT_FLOAT_EQ(a, b);
}

TEST(NoiseTest, Perlin1D_DifferentInputsDifferentOutput)
{
    float a = Noise::Perlin1D(0.5f);
    float b = Noise::Perlin1D(7.3f);
    EXPECT_NE(a, b);
}

TEST(NoiseTest, Perlin1D_RangeCheck)
{
    for (float x = -50.0f; x <= 50.0f; x += 0.37f)
    {
        float v = Noise::Perlin1D(x);
        EXPECT_GE(v, -1.0f);
        EXPECT_LE(v, 1.0f);
    }
}

// ============================================================================
// Perlin2D
// ============================================================================

TEST(NoiseTest, Perlin2D_Deterministic)
{
    float a = Noise::Perlin2D(3.14f, 2.71f);
    float b = Noise::Perlin2D(3.14f, 2.71f);
    EXPECT_FLOAT_EQ(a, b);
}

TEST(NoiseTest, Perlin2D_DifferentInputsDifferentOutput)
{
    float a = Noise::Perlin2D(1.3f, 2.7f);
    float b = Noise::Perlin2D(5.1f, 8.4f);
    EXPECT_NE(a, b);
}

TEST(NoiseTest, Perlin2D_RangeCheck)
{
    for (float x = -10.0f; x <= 10.0f; x += 1.13f)
    {
        for (float y = -10.0f; y <= 10.0f; y += 1.17f)
        {
            float v = Noise::Perlin2D(x, y);
            EXPECT_GE(v, -1.0f);
            EXPECT_LE(v, 1.0f);
        }
    }
}

TEST(NoiseTest, Perlin2D_SmoothGradient)
{
    // 近接する入力は類似の値を生成するべき（連続性）
    float a = Noise::Perlin2D(5.0f, 5.0f);
    float b = Noise::Perlin2D(5.001f, 5.0f);
    EXPECT_NEAR(a, b, 0.05f);
}

TEST(NoiseTest, Perlin2D_NotConstant)
{
    // 多数のポイントをサンプリングした場合、すべて同じ値にはならないべき
    float first = Noise::Perlin2D(0.0f, 0.0f);
    bool anyDifferent = false;
    for (float x = 0.5f; x < 5.0f; x += 0.7f)
    {
        if (Noise::Perlin2D(x, x) != first)
        {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent);
}

// ============================================================================
// Perlin3D
// ============================================================================

TEST(NoiseTest, Perlin3D_Deterministic)
{
    float a = Noise::Perlin3D(1.5f, 2.5f, 3.5f);
    float b = Noise::Perlin3D(1.5f, 2.5f, 3.5f);
    EXPECT_FLOAT_EQ(a, b);
}

TEST(NoiseTest, Perlin3D_RangeCheck)
{
    for (float x = -5.0f; x <= 5.0f; x += 1.3f)
    {
        for (float y = -5.0f; y <= 5.0f; y += 1.7f)
        {
            float v = Noise::Perlin3D(x, y, x + y);
            EXPECT_GE(v, -1.0f);
            EXPECT_LE(v, 1.0f);
        }
    }
}

TEST(NoiseTest, Perlin3D_DifferentInputsDifferentOutput)
{
    float a = Noise::Perlin3D(1.3f, 2.7f, 3.1f);
    float b = Noise::Perlin3D(7.4f, 8.2f, 9.6f);
    EXPECT_NE(a, b);
}

// ============================================================================
// FBM (2D)
// ============================================================================

TEST(NoiseTest, FBM_Deterministic)
{
    float a = Noise::FBM(1.0f, 2.0f, 4, 2.0f, 0.5f);
    float b = Noise::FBM(1.0f, 2.0f, 4, 2.0f, 0.5f);
    EXPECT_FLOAT_EQ(a, b);
}

TEST(NoiseTest, FBM_OctavesAffectDetail)
{
    float low  = Noise::FBM(3.7f, 4.2f, 1);
    float high = Noise::FBM(3.7f, 4.2f, 8);
    // オクターブ数が多いほど出力は一般的に変化する
    // 偶然一致する場合もあるので、異なり得ることを確認
    // 安全のため別の座標ペアを使用
    float low2  = Noise::FBM(1.1f, 2.3f, 1);
    float high2 = Noise::FBM(1.1f, 2.3f, 8);
    EXPECT_NE(low2, high2);
}

TEST(NoiseTest, FBM_LacunarityAffectsResult)
{
    float a = Noise::FBM(2.0f, 3.0f, 4, 1.5f, 0.5f);
    float b = Noise::FBM(2.0f, 3.0f, 4, 3.0f, 0.5f);
    EXPECT_NE(a, b);
}

TEST(NoiseTest, FBM_GainAffectsResult)
{
    float a = Noise::FBM(2.3f, 3.7f, 4, 2.0f, 0.3f);
    float b = Noise::FBM(2.3f, 3.7f, 4, 2.0f, 0.7f);
    EXPECT_NE(a, b);
}

TEST(NoiseTest, FBM_RangeReasonable)
{
    // デフォルトパラメータのFBMは概ね[-2, 2]の範囲に収まる
    for (float x = -5.0f; x <= 5.0f; x += 0.9f)
    {
        float v = Noise::FBM(x, x * 0.7f, 6, 2.0f, 0.5f);
        EXPECT_GE(v, -3.0f);
        EXPECT_LE(v, 3.0f);
    }
}

// ============================================================================
// FBM3D
// ============================================================================

TEST(NoiseTest, FBM3D_Deterministic)
{
    float a = Noise::FBM3D(1.0f, 2.0f, 3.0f, 4);
    float b = Noise::FBM3D(1.0f, 2.0f, 3.0f, 4);
    EXPECT_FLOAT_EQ(a, b);
}

TEST(NoiseTest, FBM3D_OctavesAffectResult)
{
    float a = Noise::FBM3D(1.1f, 2.3f, 0.7f, 1);
    float b = Noise::FBM3D(1.1f, 2.3f, 0.7f, 8);
    EXPECT_NE(a, b);
}

TEST(NoiseTest, FBM3D_RangeReasonable)
{
    for (float x = -3.0f; x <= 3.0f; x += 1.1f)
    {
        float v = Noise::FBM3D(x, x * 0.5f, x * 1.3f, 6, 2.0f, 0.5f);
        EXPECT_GE(v, -3.0f);
        EXPECT_LE(v, 3.0f);
    }
}

// ============================================================================
// Turbulence
// ============================================================================

TEST(NoiseTest, Turbulence_AlwaysNonNegative)
{
    for (float x = -10.0f; x <= 10.0f; x += 0.73f)
    {
        for (float y = -5.0f; y <= 5.0f; y += 1.1f)
        {
            float v = Noise::Turbulence(x, y, 4);
            EXPECT_GE(v, 0.0f);
        }
    }
}

TEST(NoiseTest, Turbulence_Deterministic)
{
    float a = Noise::Turbulence(2.5f, 3.5f, 4);
    float b = Noise::Turbulence(2.5f, 3.5f, 4);
    EXPECT_FLOAT_EQ(a, b);
}

// ============================================================================
// Ridge
// ============================================================================

TEST(NoiseTest, Ridge_Deterministic)
{
    float a = Noise::Ridge(1.5f, 2.5f, 4, 1.0f);
    float b = Noise::Ridge(1.5f, 2.5f, 4, 1.0f);
    EXPECT_FLOAT_EQ(a, b);
}

TEST(NoiseTest, Ridge_ProducesVariation)
{
    float a = Noise::Ridge(0.0f, 0.0f);
    float b = Noise::Ridge(3.7f, 8.2f);
    EXPECT_NE(a, b);
}

// ============================================================================
// Remap
// ============================================================================

TEST(NoiseTest, Remap_MinusOneToMin)
{
    float v = Noise::Remap(-1.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(v, 0.0f);
}

TEST(NoiseTest, Remap_PlusOneToMax)
{
    float v = Noise::Remap(1.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(v, 100.0f);
}

TEST(NoiseTest, Remap_ZeroToMidpoint)
{
    float v = Noise::Remap(0.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(v, 50.0f);
}

TEST(NoiseTest, Remap_CustomRange)
{
    float v = Noise::Remap(0.5f, 10.0f, 20.0f);
    EXPECT_FLOAT_EQ(v, 17.5f);
}
