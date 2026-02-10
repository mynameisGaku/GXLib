/// @file test_Quaternion.cpp
/// @brief Quaternion 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Quaternion.h"

using namespace GX;

TEST(QuaternionTest, IdentityIsDefault)
{
    Quaternion q;
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST(QuaternionTest, IdentityLength)
{
    Quaternion q = Quaternion::Identity();
    EXPECT_NEAR(q.Length(), 1.0f, 1e-5f);
}

TEST(QuaternionTest, MultiplyIdentity)
{
    Quaternion a = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI / 4.0f);
    Quaternion id = Quaternion::Identity();
    Quaternion result = a * id;

    EXPECT_NEAR(result.x, a.x, 1e-5f);
    EXPECT_NEAR(result.y, a.y, 1e-5f);
    EXPECT_NEAR(result.z, a.z, 1e-5f);
    EXPECT_NEAR(result.w, a.w, 1e-5f);
}

TEST(QuaternionTest, Inverse)
{
    Quaternion q = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI / 3.0f);
    Quaternion inv = q.Inverse();
    Quaternion product = q * inv;

    EXPECT_NEAR(product.x, 0.0f, 1e-4f);
    EXPECT_NEAR(product.y, 0.0f, 1e-4f);
    EXPECT_NEAR(product.z, 0.0f, 1e-4f);
    EXPECT_NEAR(product.w, 1.0f, 1e-4f);
}

TEST(QuaternionTest, Conjugate)
{
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion c = q.Conjugate();
    EXPECT_FLOAT_EQ(c.x, -1.0f);
    EXPECT_FLOAT_EQ(c.y, -2.0f);
    EXPECT_FLOAT_EQ(c.z, -3.0f);
    EXPECT_FLOAT_EQ(c.w, 4.0f);
}

TEST(QuaternionTest, FromAxisAngle)
{
    // Y軸まわりに90度回転
    Quaternion q = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI / 2.0f);
    EXPECT_NEAR(q.Length(), 1.0f, 1e-5f);

    // X軸方向を回すとZ方向を向くはず（右手系の基本確認）
    Vector3 rotated = q.RotateVector(Vector3(1, 0, 0));
    EXPECT_NEAR(rotated.x, 0.0f, 1e-4f);
    EXPECT_NEAR(rotated.z, -1.0f, 1e-4f);
}

TEST(QuaternionTest, EulerRoundTrip)
{
    float pitch = 0.3f, yaw = 0.5f, roll = 0.1f;
    Quaternion q = Quaternion::FromEuler(pitch, yaw, roll);
    Vector3 euler = q.ToEuler();

    EXPECT_NEAR(euler.x, pitch, 1e-3f);
    EXPECT_NEAR(euler.y, yaw, 1e-3f);
    EXPECT_NEAR(euler.z, roll, 1e-3f);
}

TEST(QuaternionTest, Slerp)
{
    Quaternion a = Quaternion::Identity();
    Quaternion b = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI / 2.0f);

    // Slerpのt=0は始点aになる
    Quaternion s0 = Quaternion::Slerp(a, b, 0.0f);
    EXPECT_NEAR(s0.x, a.x, 1e-4f);
    EXPECT_NEAR(s0.y, a.y, 1e-4f);
    EXPECT_NEAR(s0.z, a.z, 1e-4f);
    EXPECT_NEAR(s0.w, a.w, 1e-4f);

    // Slerpのt=1は終点bになる
    Quaternion s1 = Quaternion::Slerp(a, b, 1.0f);
    EXPECT_NEAR(s1.x, b.x, 1e-4f);
    EXPECT_NEAR(s1.y, b.y, 1e-4f);
    EXPECT_NEAR(s1.z, b.z, 1e-4f);
    EXPECT_NEAR(s1.w, b.w, 1e-4f);

    // 中間も単位クォータニオンになる（正規化が保たれる）
    Quaternion s05 = Quaternion::Slerp(a, b, 0.5f);
    EXPECT_NEAR(s05.Length(), 1.0f, 1e-4f);
}

TEST(QuaternionTest, Normalize)
{
    Quaternion q(1, 2, 3, 4);
    Quaternion n = q.Normalized();
    EXPECT_NEAR(n.Length(), 1.0f, 1e-5f);
}

TEST(QuaternionTest, RotateVector)
{
    // Y軸まわりに180度回転： (1,0,0) -> (-1,0,0)
    Quaternion q = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI);
    Vector3 result = q.RotateVector(Vector3(1, 0, 0));
    EXPECT_NEAR(result.x, -1.0f, 1e-4f);
    EXPECT_NEAR(result.y, 0.0f, 1e-4f);
}
