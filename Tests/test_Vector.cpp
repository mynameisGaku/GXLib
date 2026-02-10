/// @file test_Vector.cpp
/// @brief Vector2/3/4 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

using namespace GX;

// ============================================================================
// Vector2（2次元ベクトル）
// ============================================================================

TEST(Vector2Test, DefaultConstructor)
{
    Vector2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vector2Test, ParameterizedConstructor)
{
    Vector2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST(Vector2Test, Addition)
{
    Vector2 a(1.0f, 2.0f), b(3.0f, 4.0f);
    Vector2 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 6.0f);
}

TEST(Vector2Test, Subtraction)
{
    Vector2 a(5.0f, 7.0f), b(2.0f, 3.0f);
    Vector2 c = a - b;
    EXPECT_FLOAT_EQ(c.x, 3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
}

TEST(Vector2Test, ScalarMultiply)
{
    Vector2 v(2.0f, 3.0f);
    Vector2 r = v * 2.0f;
    EXPECT_FLOAT_EQ(r.x, 4.0f);
    EXPECT_FLOAT_EQ(r.y, 6.0f);

    // 交換法則の確認（スカラー×ベクトルでも同じ結果になる）
    Vector2 r2 = 2.0f * v;
    EXPECT_FLOAT_EQ(r2.x, 4.0f);
    EXPECT_FLOAT_EQ(r2.y, 6.0f);
}

TEST(Vector2Test, ScalarDivide)
{
    Vector2 v(6.0f, 8.0f);
    Vector2 r = v / 2.0f;
    EXPECT_FLOAT_EQ(r.x, 3.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
}

TEST(Vector2Test, DotProduct)
{
    Vector2 a(1.0f, 0.0f), b(0.0f, 1.0f);
    EXPECT_NEAR(a.Dot(b), 0.0f, 1e-5f);

    Vector2 c(2.0f, 3.0f), d(4.0f, 5.0f);
    EXPECT_NEAR(c.Dot(d), 23.0f, 1e-5f);
}

TEST(Vector2Test, CrossProduct)
{
    Vector2 a(1.0f, 0.0f), b(0.0f, 1.0f);
    EXPECT_NEAR(a.Cross(b), 1.0f, 1e-5f);
}

TEST(Vector2Test, Length)
{
    Vector2 v(3.0f, 4.0f);
    EXPECT_NEAR(v.Length(), 5.0f, 1e-5f);
    EXPECT_NEAR(v.LengthSquared(), 25.0f, 1e-5f);
}

TEST(Vector2Test, Normalize)
{
    Vector2 v(3.0f, 4.0f);
    Vector2 n = v.Normalized();
    EXPECT_NEAR(n.Length(), 1.0f, 1e-5f);
    EXPECT_NEAR(n.x, 0.6f, 1e-5f);
    EXPECT_NEAR(n.y, 0.8f, 1e-5f);
}

TEST(Vector2Test, Distance)
{
    Vector2 a(0.0f, 0.0f), b(3.0f, 4.0f);
    EXPECT_NEAR(a.Distance(b), 5.0f, 1e-5f);
}

TEST(Vector2Test, Lerp)
{
    Vector2 a(0.0f, 0.0f), b(10.0f, 20.0f);
    Vector2 mid = Vector2::Lerp(a, b, 0.5f);
    EXPECT_NEAR(mid.x, 5.0f, 1e-5f);
    EXPECT_NEAR(mid.y, 10.0f, 1e-5f);

    Vector2 start = Vector2::Lerp(a, b, 0.0f);
    EXPECT_FLOAT_EQ(start.x, 0.0f);

    Vector2 end = Vector2::Lerp(a, b, 1.0f);
    EXPECT_NEAR(end.x, 10.0f, 1e-5f);
}

TEST(Vector2Test, Negation)
{
    Vector2 v(3.0f, -4.0f);
    Vector2 n = -v;
    EXPECT_FLOAT_EQ(n.x, -3.0f);
    EXPECT_FLOAT_EQ(n.y, 4.0f);
}

TEST(Vector2Test, StaticConstants)
{
    EXPECT_EQ(Vector2::Zero(), Vector2(0.0f, 0.0f));
    EXPECT_EQ(Vector2::One(), Vector2(1.0f, 1.0f));
    EXPECT_EQ(Vector2::UnitX(), Vector2(1.0f, 0.0f));
    EXPECT_EQ(Vector2::UnitY(), Vector2(0.0f, 1.0f));
}

// ============================================================================
// Vector3（3次元ベクトル）
// ============================================================================

TEST(Vector3Test, DefaultConstructor)
{
    Vector3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vector3Test, Addition)
{
    Vector3 a(1, 2, 3), b(4, 5, 6);
    Vector3 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 5.0f);
    EXPECT_FLOAT_EQ(c.y, 7.0f);
    EXPECT_FLOAT_EQ(c.z, 9.0f);
}

TEST(Vector3Test, CrossProduct)
{
    Vector3 x(1, 0, 0), y(0, 1, 0);
    Vector3 z = x.Cross(y);
    EXPECT_NEAR(z.x, 0.0f, 1e-5f);
    EXPECT_NEAR(z.y, 0.0f, 1e-5f);
    EXPECT_NEAR(z.z, 1.0f, 1e-5f);
}

TEST(Vector3Test, DotProduct)
{
    Vector3 a(1, 0, 0), b(0, 1, 0);
    EXPECT_NEAR(a.Dot(b), 0.0f, 1e-5f);

    Vector3 c(1, 2, 3), d(4, 5, 6);
    EXPECT_NEAR(c.Dot(d), 32.0f, 1e-5f);
}

TEST(Vector3Test, Length)
{
    Vector3 v(1, 2, 2);
    EXPECT_NEAR(v.Length(), 3.0f, 1e-5f);
}

TEST(Vector3Test, Normalize)
{
    Vector3 v(0, 0, 5);
    Vector3 n = v.Normalized();
    EXPECT_NEAR(n.z, 1.0f, 1e-5f);
    EXPECT_NEAR(n.Length(), 1.0f, 1e-5f);
}

TEST(Vector3Test, Lerp)
{
    Vector3 a(0, 0, 0), b(10, 20, 30);
    Vector3 mid = Vector3::Lerp(a, b, 0.5f);
    EXPECT_NEAR(mid.x, 5.0f, 1e-5f);
    EXPECT_NEAR(mid.y, 10.0f, 1e-5f);
    EXPECT_NEAR(mid.z, 15.0f, 1e-5f);
}

TEST(Vector3Test, Reflect)
{
    Vector3 dir(1, -1, 0);
    Vector3 normal(0, 1, 0);
    Vector3 reflected = Vector3::Reflect(dir, normal);
    EXPECT_NEAR(reflected.x, 1.0f, 1e-5f);
    EXPECT_NEAR(reflected.y, 1.0f, 1e-5f);
}

TEST(Vector3Test, StaticConstants)
{
    EXPECT_EQ(Vector3::Up(), Vector3(0, 1, 0));
    EXPECT_EQ(Vector3::Forward(), Vector3(0, 0, 1));
    EXPECT_EQ(Vector3::Right(), Vector3(1, 0, 0));
}

// ============================================================================
// Vector4（4次元ベクトル）
// ============================================================================

TEST(Vector4Test, DefaultConstructor)
{
    Vector4 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
    EXPECT_FLOAT_EQ(v.w, 0.0f);
}

TEST(Vector4Test, ConstructFromVector3)
{
    Vector3 v3(1, 2, 3);
    Vector4 v4(v3, 4.0f);
    EXPECT_FLOAT_EQ(v4.x, 1.0f);
    EXPECT_FLOAT_EQ(v4.y, 2.0f);
    EXPECT_FLOAT_EQ(v4.z, 3.0f);
    EXPECT_FLOAT_EQ(v4.w, 4.0f);
}

TEST(Vector4Test, DotProduct)
{
    Vector4 a(1, 2, 3, 4), b(5, 6, 7, 8);
    EXPECT_NEAR(a.Dot(b), 70.0f, 1e-5f); // 5+12+21+32
}

TEST(Vector4Test, Length)
{
    Vector4 v(1, 0, 0, 0);
    EXPECT_NEAR(v.Length(), 1.0f, 1e-5f);
}

TEST(Vector4Test, Normalize)
{
    Vector4 v(2, 0, 0, 0);
    Vector4 n = v.Normalized();
    EXPECT_NEAR(n.x, 1.0f, 1e-5f);
    EXPECT_NEAR(n.Length(), 1.0f, 1e-5f);
}

TEST(Vector4Test, Lerp)
{
    Vector4 a(0, 0, 0, 0), b(4, 8, 12, 16);
    Vector4 mid = Vector4::Lerp(a, b, 0.25f);
    EXPECT_NEAR(mid.x, 1.0f, 1e-5f);
    EXPECT_NEAR(mid.y, 2.0f, 1e-5f);
    EXPECT_NEAR(mid.z, 3.0f, 1e-5f);
    EXPECT_NEAR(mid.w, 4.0f, 1e-5f);
}
