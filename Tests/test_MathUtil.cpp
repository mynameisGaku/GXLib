/// @file test_MathUtil.cpp
/// @brief MathUtil + Random 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/MathUtil.h"
#include "Math/Random.h"

using namespace GX;

// ============================================================================
// MathUtil（汎用数学関数）
// ============================================================================

TEST(MathUtilTest, Lerp)
{
    EXPECT_FLOAT_EQ(MathUtil::Lerp(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(MathUtil::Lerp(0.0f, 10.0f, 0.5f), 5.0f);
    EXPECT_FLOAT_EQ(MathUtil::Lerp(0.0f, 10.0f, 1.0f), 10.0f);
}

TEST(MathUtilTest, ClampFloat)
{
    EXPECT_FLOAT_EQ(MathUtil::Clamp(5.0f, 0.0f, 10.0f), 5.0f);
    EXPECT_FLOAT_EQ(MathUtil::Clamp(-1.0f, 0.0f, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(MathUtil::Clamp(15.0f, 0.0f, 10.0f), 10.0f);
}

TEST(MathUtilTest, ClampInt)
{
    EXPECT_EQ(MathUtil::Clamp(5, 0, 10), 5);
    EXPECT_EQ(MathUtil::Clamp(-1, 0, 10), 0);
    EXPECT_EQ(MathUtil::Clamp(15, 0, 10), 10);
}

TEST(MathUtilTest, SmoothStep)
{
    EXPECT_FLOAT_EQ(MathUtil::SmoothStep(0.0f, 1.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(MathUtil::SmoothStep(0.0f, 1.0f, 1.0f), 1.0f);

    float mid = MathUtil::SmoothStep(0.0f, 1.0f, 0.5f);
    EXPECT_NEAR(mid, 0.5f, 1e-5f);

    // edge0より下は0になる
    EXPECT_FLOAT_EQ(MathUtil::SmoothStep(0.0f, 1.0f, -1.0f), 0.0f);
    // edge1より上は1になる
    EXPECT_FLOAT_EQ(MathUtil::SmoothStep(0.0f, 1.0f, 2.0f), 1.0f);
}

TEST(MathUtilTest, Remap)
{
    float result = MathUtil::Remap(5.0f, 0.0f, 10.0f, 100.0f, 200.0f);
    EXPECT_NEAR(result, 150.0f, 1e-5f);

    result = MathUtil::Remap(0.0f, 0.0f, 10.0f, 100.0f, 200.0f);
    EXPECT_NEAR(result, 100.0f, 1e-5f);
}

TEST(MathUtilTest, InverseLerp)
{
    EXPECT_NEAR(MathUtil::InverseLerp(0.0f, 10.0f, 5.0f), 0.5f, 1e-5f);
    EXPECT_NEAR(MathUtil::InverseLerp(0.0f, 10.0f, 0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::InverseLerp(0.0f, 10.0f, 10.0f), 1.0f, 1e-5f);
}

TEST(MathUtilTest, DegreesRadians)
{
    EXPECT_NEAR(MathUtil::DegreesToRadians(180.0f), MathUtil::PI, 1e-5f);
    EXPECT_NEAR(MathUtil::RadiansToDegrees(MathUtil::PI), 180.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::DegreesToRadians(90.0f), MathUtil::PI / 2.0f, 1e-5f);
}

TEST(MathUtilTest, NormalizeAngle)
{
    EXPECT_NEAR(MathUtil::NormalizeAngle(0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::NormalizeAngle(MathUtil::TAU), 0.0f, 1e-4f);
    EXPECT_NEAR(MathUtil::NormalizeAngle(-MathUtil::PI), -MathUtil::PI, 1e-4f);
}

TEST(MathUtilTest, Sign)
{
    EXPECT_FLOAT_EQ(MathUtil::Sign(5.0f), 1.0f);
    EXPECT_FLOAT_EQ(MathUtil::Sign(-3.0f), -1.0f);
    EXPECT_FLOAT_EQ(MathUtil::Sign(0.0f), 0.0f);
}

TEST(MathUtilTest, IsPowerOfTwo)
{
    EXPECT_TRUE(MathUtil::IsPowerOfTwo(1));
    EXPECT_TRUE(MathUtil::IsPowerOfTwo(2));
    EXPECT_TRUE(MathUtil::IsPowerOfTwo(256));
    EXPECT_FALSE(MathUtil::IsPowerOfTwo(0));
    EXPECT_FALSE(MathUtil::IsPowerOfTwo(3));
    EXPECT_FALSE(MathUtil::IsPowerOfTwo(100));
}

TEST(MathUtilTest, NextPowerOfTwo)
{
    EXPECT_EQ(MathUtil::NextPowerOfTwo(0), 1u);
    EXPECT_EQ(MathUtil::NextPowerOfTwo(1), 1u);
    EXPECT_EQ(MathUtil::NextPowerOfTwo(3), 4u);
    EXPECT_EQ(MathUtil::NextPowerOfTwo(5), 8u);
    EXPECT_EQ(MathUtil::NextPowerOfTwo(128), 128u);
    EXPECT_EQ(MathUtil::NextPowerOfTwo(129), 256u);
}

TEST(MathUtilTest, ApproximatelyEqual)
{
    EXPECT_TRUE(MathUtil::ApproximatelyEqual(1.0f, 1.0f));
    EXPECT_TRUE(MathUtil::ApproximatelyEqual(1.0f, 1.0f + 1e-7f));
    EXPECT_FALSE(MathUtil::ApproximatelyEqual(1.0f, 2.0f));
}

// ============================================================================
// Random（乱数）
// ============================================================================

TEST(RandomTest, IntRange)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        int v = rng.Int(0, 10);
        EXPECT_GE(v, 0);
        EXPECT_LE(v, 10);
    }
}

TEST(RandomTest, FloatRange)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        float v = rng.Float(0.0f, 1.0f);
        EXPECT_GE(v, 0.0f);
        EXPECT_LE(v, 1.0f);
    }
}

TEST(RandomTest, DeterministicSeed)
{
    Random a(12345), b(12345);
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(a.Int(), b.Int());
    }
}

TEST(RandomTest, PointInCircle)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        Vector2 p = rng.PointInCircle(5.0f);
        EXPECT_LE(p.Length(), 5.0f + 1e-5f);
    }
}

TEST(RandomTest, PointInSphere)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        Vector3 p = rng.PointInSphere(3.0f);
        EXPECT_LE(p.Length(), 3.0f + 1e-5f);
    }
}

TEST(RandomTest, Direction2DIsUnit)
{
    Random rng(42);
    for (int i = 0; i < 20; ++i)
    {
        Vector2 d = rng.Direction2D();
        EXPECT_NEAR(d.Length(), 1.0f, 1e-4f);
    }
}

TEST(RandomTest, Direction3DIsUnit)
{
    Random rng(42);
    for (int i = 0; i < 20; ++i)
    {
        Vector3 d = rng.Direction3D();
        EXPECT_NEAR(d.Length(), 1.0f, 1e-4f);
    }
}
