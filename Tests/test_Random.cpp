/// @file test_Random.cpp
/// @brief Randomクラスのユニットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Random.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Color.h"

using namespace gx;

// ============================================================================
// コンストラクション
// ============================================================================

TEST(RandomTest, DefaultConstructorNoCrash)
{
    EXPECT_NO_THROW(Random rng);
}

TEST(RandomTest, SeededConstructorDeterministic)
{
    Random a(42);
    Random b(42);
    EXPECT_EQ(a.Int(), b.Int());
    EXPECT_EQ(a.Int(), b.Int());
    EXPECT_EQ(a.Int(), b.Int());
}

// ============================================================================
// SetSeed
// ============================================================================

TEST(RandomTest, SetSeedProducesSameSequence)
{
    Random rng(0);
    rng.SetSeed(123);
    int v1 = rng.Int();
    int v2 = rng.Int();

    rng.SetSeed(123);
    EXPECT_EQ(rng.Int(), v1);
    EXPECT_EQ(rng.Int(), v2);
}

// ============================================================================
// Int
// ============================================================================

TEST(RandomTest, IntReturnsValues)
{
    Random rng(99);
    // クラッシュせずに実行され、異なる値を返すことを確認
    int a = rng.Int();
    int b = rng.Int();
    // 良い乱数生成器なら同じ値になる確率は極めて低い
    bool anyDifferent = (a != b);
    for (int i = 0; i < 10 && !anyDifferent; ++i)
        anyDifferent = (rng.Int() != a);
    EXPECT_TRUE(anyDifferent);
}

TEST(RandomTest, IntMinMaxInRange)
{
    Random rng(42);
    for (int i = 0; i < 1000; ++i)
    {
        int v = rng.Int(10, 20);
        EXPECT_GE(v, 10);
        EXPECT_LE(v, 20);
    }
}

// ============================================================================
// Float
// ============================================================================

TEST(RandomTest, FloatInZeroOneRange)
{
    Random rng(42);
    for (int i = 0; i < 1000; ++i)
    {
        float v = rng.Float();
        EXPECT_GE(v, 0.0f);
        EXPECT_LE(v, 1.0f);
    }
}

TEST(RandomTest, FloatMinMaxInRange)
{
    Random rng(42);
    for (int i = 0; i < 1000; ++i)
    {
        float v = rng.Float(-5.0f, 5.0f);
        EXPECT_GE(v, -5.0f);
        EXPECT_LE(v, 5.0f);
    }
}

// ============================================================================
// 幾何分布
// ============================================================================

TEST(RandomTest, PointInCircleWithinRadius)
{
    Random rng(42);
    for (int i = 0; i < 500; ++i)
    {
        Vector2 p = rng.PointInCircle(3.0f);
        float dist = p.Length();
        EXPECT_LE(dist, 3.0f + 1e-5f);
    }
}

TEST(RandomTest, PointInSphereWithinRadius)
{
    Random rng(42);
    for (int i = 0; i < 500; ++i)
    {
        Vector3 p = rng.PointInSphere(2.0f);
        float dist = p.Length();
        EXPECT_LE(dist, 2.0f + 1e-5f);
    }
}

TEST(RandomTest, Direction2DIsUnitVector)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        Vector2 d = rng.Direction2D();
        EXPECT_NEAR(d.Length(), 1.0f, 1e-4f);
    }
}

TEST(RandomTest, Direction3DIsUnitVector)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        Vector3 d = rng.Direction3D();
        EXPECT_NEAR(d.Length(), 1.0f, 1e-4f);
    }
}

// ============================================================================
// RandomColor
// ============================================================================

TEST(RandomTest, RandomColorComponentsInRange)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        Color c = rng.RandomColor(0.8f);
        EXPECT_GE(c.r, 0.0f); EXPECT_LE(c.r, 1.0f);
        EXPECT_GE(c.g, 0.0f); EXPECT_LE(c.g, 1.0f);
        EXPECT_GE(c.b, 0.0f); EXPECT_LE(c.b, 1.0f);
        EXPECT_FLOAT_EQ(c.a, 0.8f);
    }
}

// ============================================================================
// ベクトル範囲
// ============================================================================

TEST(RandomTest, Vector2InRangeComponents)
{
    Random rng(42);
    for (int i = 0; i < 500; ++i)
    {
        Vector2 v = rng.Vector2InRange(-3.0f, 3.0f, -5.0f, 5.0f);
        EXPECT_GE(v.x, -3.0f); EXPECT_LE(v.x, 3.0f);
        EXPECT_GE(v.y, -5.0f); EXPECT_LE(v.y, 5.0f);
    }
}

TEST(RandomTest, Vector3InRangeComponents)
{
    Random rng(42);
    for (int i = 0; i < 500; ++i)
    {
        Vector3 v = rng.Vector3InRange(-1.0f, 1.0f, -2.0f, 2.0f, -3.0f, 3.0f);
        EXPECT_GE(v.x, -1.0f); EXPECT_LE(v.x, 1.0f);
        EXPECT_GE(v.y, -2.0f); EXPECT_LE(v.y, 2.0f);
        EXPECT_GE(v.z, -3.0f); EXPECT_LE(v.z, 3.0f);
    }
}

// ============================================================================
// グローバルインスタンス
// ============================================================================

TEST(RandomTest, GlobalReturnsSameInstance)
{
    Random& a = Random::Global();
    Random& b = Random::Global();
    EXPECT_EQ(&a, &b);
}
