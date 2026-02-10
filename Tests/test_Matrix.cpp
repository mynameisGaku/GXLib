/// @file test_Matrix.cpp
/// @brief Matrix4x4 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Matrix4x4.h"

using namespace GX;

TEST(Matrix4x4Test, DefaultIsIdentity)
{
    Matrix4x4 m;
    EXPECT_NEAR(m._11, 1.0f, 1e-5f);
    EXPECT_NEAR(m._22, 1.0f, 1e-5f);
    EXPECT_NEAR(m._33, 1.0f, 1e-5f);
    EXPECT_NEAR(m._44, 1.0f, 1e-5f);
    EXPECT_NEAR(m._12, 0.0f, 1e-5f);
    EXPECT_NEAR(m._21, 0.0f, 1e-5f);
}

TEST(Matrix4x4Test, IdentityMultiply)
{
    Matrix4x4 a = Matrix4x4::Translation(1, 2, 3);
    Matrix4x4 id = Matrix4x4::Identity();
    Matrix4x4 result = a * id;
    EXPECT_NEAR(result._41, 1.0f, 1e-5f);
    EXPECT_NEAR(result._42, 2.0f, 1e-5f);
    EXPECT_NEAR(result._43, 3.0f, 1e-5f);
}

TEST(Matrix4x4Test, Translation)
{
    Matrix4x4 t = Matrix4x4::Translation(5, 10, 15);
    Vector3 origin(0, 0, 0);
    Vector3 result = t.TransformPoint(origin);
    EXPECT_NEAR(result.x, 5.0f, 1e-5f);
    EXPECT_NEAR(result.y, 10.0f, 1e-5f);
    EXPECT_NEAR(result.z, 15.0f, 1e-5f);
}

TEST(Matrix4x4Test, Scaling)
{
    Matrix4x4 s = Matrix4x4::Scaling(2, 3, 4);
    Vector3 v(1, 1, 1);
    Vector3 result = s.TransformPoint(v);
    EXPECT_NEAR(result.x, 2.0f, 1e-5f);
    EXPECT_NEAR(result.y, 3.0f, 1e-5f);
    EXPECT_NEAR(result.z, 4.0f, 1e-5f);
}

TEST(Matrix4x4Test, UniformScaling)
{
    Matrix4x4 s = Matrix4x4::Scaling(3.0f);
    Vector3 v(1, 2, 3);
    Vector3 result = s.TransformPoint(v);
    EXPECT_NEAR(result.x, 3.0f, 1e-5f);
    EXPECT_NEAR(result.y, 6.0f, 1e-5f);
    EXPECT_NEAR(result.z, 9.0f, 1e-5f);
}

TEST(Matrix4x4Test, RotationZ90)
{
    Matrix4x4 r = Matrix4x4::RotationZ(MathUtil::PI / 2.0f);
    Vector3 v(1, 0, 0);
    Vector3 result = r.TransformPoint(v);
    EXPECT_NEAR(result.x, 0.0f, 1e-4f);
    EXPECT_NEAR(result.y, 1.0f, 1e-4f);
}

TEST(Matrix4x4Test, Inverse)
{
    Matrix4x4 t = Matrix4x4::Translation(3, 4, 5);
    Matrix4x4 inv = t.Inverse();
    Matrix4x4 product = t * inv;

    // 単位行列に近いはず（逆行列の検証）
    EXPECT_NEAR(product._11, 1.0f, 1e-4f);
    EXPECT_NEAR(product._22, 1.0f, 1e-4f);
    EXPECT_NEAR(product._33, 1.0f, 1e-4f);
    EXPECT_NEAR(product._44, 1.0f, 1e-4f);
    EXPECT_NEAR(product._41, 0.0f, 1e-4f);
    EXPECT_NEAR(product._42, 0.0f, 1e-4f);
    EXPECT_NEAR(product._43, 0.0f, 1e-4f);
}

TEST(Matrix4x4Test, Transpose)
{
    Matrix4x4 m = Matrix4x4::Translation(1, 2, 3);
    Matrix4x4 t = m.Transpose();
    // 転置すると移動成分が行→列に移る
    EXPECT_NEAR(t._14, 1.0f, 1e-5f);
    EXPECT_NEAR(t._24, 2.0f, 1e-5f);
    EXPECT_NEAR(t._34, 3.0f, 1e-5f);
}

TEST(Matrix4x4Test, Determinant)
{
    Matrix4x4 id = Matrix4x4::Identity();
    EXPECT_NEAR(id.Determinant(), 1.0f, 1e-5f);

    Matrix4x4 s = Matrix4x4::Scaling(2, 3, 4);
    EXPECT_NEAR(s.Determinant(), 24.0f, 1e-4f);
}

TEST(Matrix4x4Test, ToFromXMMATRIX)
{
    Matrix4x4 original = Matrix4x4::Translation(7, 8, 9);
    XMMATRIX xm = original.ToXMMATRIX();
    Matrix4x4 restored = Matrix4x4::FromXMMATRIX(xm);

    EXPECT_NEAR(restored._41, 7.0f, 1e-5f);
    EXPECT_NEAR(restored._42, 8.0f, 1e-5f);
    EXPECT_NEAR(restored._43, 9.0f, 1e-5f);
}

TEST(Matrix4x4Test, ComposeTransform)
{
    // スケール → 回転 → 平行移動の順で合成
    Matrix4x4 s = Matrix4x4::Scaling(2.0f);
    Matrix4x4 r = Matrix4x4::RotationY(MathUtil::PI);
    Matrix4x4 t = Matrix4x4::Translation(0, 0, 5);

    Matrix4x4 world = s * r * t;
    Vector3 origin(0, 0, 0);
    Vector3 result = world.TransformPoint(origin);

    // 原点は拡縮・回転で動かず、最後の平行移動で(0,0,5)に移る
    EXPECT_NEAR(result.z, 5.0f, 1e-3f);
}
