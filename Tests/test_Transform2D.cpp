/// @file test_Transform2D.cpp
/// @brief Transform2D struct + free function tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Transform2D.h"

using namespace GX;

static constexpr float kEps = 1e-5f;
static constexpr float kPi  = 3.14159265f;

// ==================== Identity ====================

TEST(Transform2DTest, Identity_Values)
{
    Transform2D t = Transform2D::Identity();
    EXPECT_NEAR(t.a, 1.0f, kEps);
    EXPECT_NEAR(t.b, 0.0f, kEps);
    EXPECT_NEAR(t.c, 0.0f, kEps);
    EXPECT_NEAR(t.d, 1.0f, kEps);
    EXPECT_NEAR(t.tx, 0.0f, kEps);
    EXPECT_NEAR(t.ty, 0.0f, kEps);
}

TEST(Transform2DTest, Identity_TransformPoint)
{
    Transform2D t = Transform2D::Identity();
    XMFLOAT2 p = TransformPoint(t, 5.0f, 3.0f);
    EXPECT_NEAR(p.x, 5.0f, kEps);
    EXPECT_NEAR(p.y, 3.0f, kEps);
}

// ==================== Translation ====================

TEST(Transform2DTest, Translation_Values)
{
    Transform2D t = Transform2D::Translation(10.0f, -5.0f);
    EXPECT_NEAR(t.a, 1.0f, kEps);
    EXPECT_NEAR(t.d, 1.0f, kEps);
    EXPECT_NEAR(t.tx, 10.0f, kEps);
    EXPECT_NEAR(t.ty, -5.0f, kEps);
}

TEST(Transform2DTest, Translation_TransformPoint)
{
    Transform2D t = Transform2D::Translation(3.0f, 7.0f);
    XMFLOAT2 p = TransformPoint(t, 1.0f, 2.0f);
    EXPECT_NEAR(p.x, 4.0f, kEps);
    EXPECT_NEAR(p.y, 9.0f, kEps);
}

TEST(Transform2DTest, Translation_Zero)
{
    Transform2D t = Transform2D::Translation(0.0f, 0.0f);
    XMFLOAT2 p = TransformPoint(t, 5.0f, 3.0f);
    EXPECT_NEAR(p.x, 5.0f, kEps);
    EXPECT_NEAR(p.y, 3.0f, kEps);
}

// ==================== Scale ====================

TEST(Transform2DTest, Scale_Values)
{
    Transform2D t = Transform2D::Scale(2.0f, 3.0f);
    EXPECT_NEAR(t.a, 2.0f, kEps);
    EXPECT_NEAR(t.d, 3.0f, kEps);
    EXPECT_NEAR(t.tx, 0.0f, kEps);
    EXPECT_NEAR(t.ty, 0.0f, kEps);
}

TEST(Transform2DTest, Scale_TransformPoint)
{
    Transform2D t = Transform2D::Scale(2.0f, 0.5f);
    XMFLOAT2 p = TransformPoint(t, 3.0f, 4.0f);
    EXPECT_NEAR(p.x, 6.0f, kEps);
    EXPECT_NEAR(p.y, 2.0f, kEps);
}

TEST(Transform2DTest, Scale_Uniform)
{
    Transform2D t = Transform2D::Scale(5.0f, 5.0f);
    XMFLOAT2 p = TransformPoint(t, 1.0f, 1.0f);
    EXPECT_NEAR(p.x, 5.0f, kEps);
    EXPECT_NEAR(p.y, 5.0f, kEps);
}

TEST(Transform2DTest, Scale_Negative)
{
    Transform2D t = Transform2D::Scale(-1.0f, -1.0f);
    XMFLOAT2 p = TransformPoint(t, 3.0f, 4.0f);
    EXPECT_NEAR(p.x, -3.0f, kEps);
    EXPECT_NEAR(p.y, -4.0f, kEps);
}

// ==================== Rotation ====================

TEST(Transform2DTest, Rotation_Zero)
{
    Transform2D t = Transform2D::Rotation(0.0f);
    EXPECT_NEAR(t.a, 1.0f, kEps);
    EXPECT_NEAR(t.b, 0.0f, kEps);
    EXPECT_NEAR(t.c, 0.0f, kEps);
    EXPECT_NEAR(t.d, 1.0f, kEps);
}

TEST(Transform2DTest, Rotation_90Degrees)
{
    Transform2D t = Transform2D::Rotation(kPi / 2.0f);
    XMFLOAT2 p = TransformPoint(t, 1.0f, 0.0f);
    EXPECT_NEAR(p.x, 0.0f, 1e-4f);
    EXPECT_NEAR(p.y, 1.0f, 1e-4f);
}

TEST(Transform2DTest, Rotation_180Degrees)
{
    Transform2D t = Transform2D::Rotation(kPi);
    XMFLOAT2 p = TransformPoint(t, 1.0f, 0.0f);
    EXPECT_NEAR(p.x, -1.0f, 1e-4f);
    EXPECT_NEAR(p.y, 0.0f, 1e-4f);
}

TEST(Transform2DTest, Rotation_360Degrees)
{
    Transform2D t = Transform2D::Rotation(2.0f * kPi);
    XMFLOAT2 p = TransformPoint(t, 3.0f, 4.0f);
    EXPECT_NEAR(p.x, 3.0f, 1e-4f);
    EXPECT_NEAR(p.y, 4.0f, 1e-4f);
}

TEST(Transform2DTest, Rotation_Negative)
{
    Transform2D t = Transform2D::Rotation(-kPi / 2.0f);
    XMFLOAT2 p = TransformPoint(t, 0.0f, 1.0f);
    EXPECT_NEAR(p.x, 1.0f, 1e-4f);
    EXPECT_NEAR(p.y, 0.0f, 1e-4f);
}

// ==================== Multiply ====================

TEST(Transform2DTest, Multiply_IdentityLeft)
{
    Transform2D id = Transform2D::Identity();
    Transform2D tr = Transform2D::Translation(5.0f, 10.0f);
    Transform2D result = Multiply(id, tr);
    EXPECT_NEAR(result.tx, 5.0f, kEps);
    EXPECT_NEAR(result.ty, 10.0f, kEps);
}

TEST(Transform2DTest, Multiply_IdentityRight)
{
    Transform2D tr = Transform2D::Translation(5.0f, 10.0f);
    Transform2D id = Transform2D::Identity();
    Transform2D result = Multiply(tr, id);
    EXPECT_NEAR(result.tx, 5.0f, kEps);
    EXPECT_NEAR(result.ty, 10.0f, kEps);
}

TEST(Transform2DTest, Multiply_ScaleThenTranslate)
{
    Transform2D s = Transform2D::Scale(2.0f, 2.0f);
    Transform2D t = Transform2D::Translation(3.0f, 4.0f);
    // Scale applied first (rhs), then translate (lhs)
    Transform2D result = Multiply(t, s);
    XMFLOAT2 p = TransformPoint(result, 1.0f, 1.0f);
    EXPECT_NEAR(p.x, 5.0f, kEps);  // 1*2 + 3
    EXPECT_NEAR(p.y, 6.0f, kEps);  // 1*2 + 4
}

TEST(Transform2DTest, Multiply_TranslateThenScale)
{
    Transform2D t = Transform2D::Translation(3.0f, 4.0f);
    Transform2D s = Transform2D::Scale(2.0f, 2.0f);
    // Translate first (rhs), then scale (lhs)
    Transform2D result = Multiply(s, t);
    XMFLOAT2 p = TransformPoint(result, 1.0f, 1.0f);
    EXPECT_NEAR(p.x, 8.0f, kEps);  // (1+3)*2
    EXPECT_NEAR(p.y, 10.0f, kEps); // (1+4)*2
}

TEST(Transform2DTest, Multiply_RotateThenTranslate)
{
    Transform2D r = Transform2D::Rotation(kPi / 2.0f);
    Transform2D t = Transform2D::Translation(5.0f, 0.0f);
    Transform2D result = Multiply(t, r);
    XMFLOAT2 p = TransformPoint(result, 1.0f, 0.0f);
    EXPECT_NEAR(p.x, 5.0f, 1e-4f);  // rotated (0,1) + translate (5,0)
    EXPECT_NEAR(p.y, 1.0f, 1e-4f);
}

// ==================== Inverse ====================

TEST(Transform2DTest, Inverse_Identity)
{
    Transform2D t = Transform2D::Identity();
    Transform2D inv = Inverse(t);
    EXPECT_NEAR(inv.a, 1.0f, kEps);
    EXPECT_NEAR(inv.d, 1.0f, kEps);
    EXPECT_NEAR(inv.tx, 0.0f, kEps);
}

TEST(Transform2DTest, Inverse_Translation)
{
    Transform2D t = Transform2D::Translation(5.0f, -3.0f);
    Transform2D inv = Inverse(t);
    EXPECT_NEAR(inv.tx, -5.0f, kEps);
    EXPECT_NEAR(inv.ty, 3.0f, kEps);
}

TEST(Transform2DTest, Inverse_Scale)
{
    Transform2D t = Transform2D::Scale(4.0f, 2.0f);
    Transform2D inv = Inverse(t);
    EXPECT_NEAR(inv.a, 0.25f, kEps);
    EXPECT_NEAR(inv.d, 0.5f, kEps);
}

TEST(Transform2DTest, Inverse_RoundTrip)
{
    Transform2D t = Multiply(
        Transform2D::Translation(3.0f, -7.0f),
        Transform2D::Rotation(1.23f));
    t = Multiply(t, Transform2D::Scale(2.0f, 0.5f));

    Transform2D inv = Inverse(t);
    Transform2D product = Multiply(t, inv);

    // Product should be identity
    EXPECT_NEAR(product.a, 1.0f, 1e-4f);
    EXPECT_NEAR(product.b, 0.0f, 1e-4f);
    EXPECT_NEAR(product.c, 0.0f, 1e-4f);
    EXPECT_NEAR(product.d, 1.0f, 1e-4f);
    EXPECT_NEAR(product.tx, 0.0f, 1e-4f);
    EXPECT_NEAR(product.ty, 0.0f, 1e-4f);
}

TEST(Transform2DTest, Inverse_SingularReturnsIdentity)
{
    Transform2D t;
    t.a = 0; t.b = 0; t.c = 0; t.d = 0; t.tx = 5; t.ty = 5;
    Transform2D inv = Inverse(t);
    EXPECT_NEAR(inv.a, 1.0f, kEps);
    EXPECT_NEAR(inv.d, 1.0f, kEps);
    EXPECT_NEAR(inv.tx, 0.0f, kEps);
}

TEST(Transform2DTest, Inverse_TransformPointRoundTrip)
{
    Transform2D t = Multiply(Transform2D::Rotation(0.7f), Transform2D::Scale(3.0f, 2.0f));
    Transform2D inv = Inverse(t);
    XMFLOAT2 original = {7.0f, -4.0f};
    XMFLOAT2 transformed = TransformPoint(t, original.x, original.y);
    XMFLOAT2 restored = TransformPoint(inv, transformed.x, transformed.y);
    EXPECT_NEAR(restored.x, original.x, 1e-3f);
    EXPECT_NEAR(restored.y, original.y, 1e-3f);
}

// ==================== Default Constructor ====================

TEST(Transform2DTest, DefaultConstructor_IsIdentity)
{
    Transform2D t;
    EXPECT_NEAR(t.a, 1.0f, kEps);
    EXPECT_NEAR(t.b, 0.0f, kEps);
    EXPECT_NEAR(t.c, 0.0f, kEps);
    EXPECT_NEAR(t.d, 1.0f, kEps);
    EXPECT_NEAR(t.tx, 0.0f, kEps);
    EXPECT_NEAR(t.ty, 0.0f, kEps);
}
