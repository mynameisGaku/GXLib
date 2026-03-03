/// @file test_Transform3D.cpp
/// @brief Transform3D 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Transform3D.h"
#include "Math/MathConvert.h"

using namespace gx;

TEST(Transform3DTest, DefaultState)
{
    Transform3D t;
    const auto& pos = t.GetPosition();
    const auto& rot = t.GetRotation();
    const auto& scl = t.GetScale();
    EXPECT_NEAR(pos.x, 0.0f, 1e-5f);
    EXPECT_NEAR(pos.y, 0.0f, 1e-5f);
    EXPECT_NEAR(pos.z, 0.0f, 1e-5f);
    EXPECT_NEAR(rot.x, 0.0f, 1e-5f);
    EXPECT_NEAR(rot.y, 0.0f, 1e-5f);
    EXPECT_NEAR(rot.z, 0.0f, 1e-5f);
    EXPECT_NEAR(scl.x, 1.0f, 1e-5f);
    EXPECT_NEAR(scl.y, 1.0f, 1e-5f);
    EXPECT_NEAR(scl.z, 1.0f, 1e-5f);
}

TEST(Transform3DTest, SetPosition_Float3)
{
    Transform3D t;
    t.SetPosition(1.0f, 2.0f, 3.0f);
    const auto& p = t.GetPosition();
    EXPECT_NEAR(p.x, 1.0f, 1e-5f);
    EXPECT_NEAR(p.y, 2.0f, 1e-5f);
    EXPECT_NEAR(p.z, 3.0f, 1e-5f);
}

TEST(Transform3DTest, SetPosition_Vector3)
{
    Transform3D t;
    t.SetPosition(Vector3{4.0f, 5.0f, 6.0f});
    const auto& p = t.GetPosition();
    EXPECT_NEAR(p.x, 4.0f, 1e-5f);
    EXPECT_NEAR(p.y, 5.0f, 1e-5f);
    EXPECT_NEAR(p.z, 6.0f, 1e-5f);
}

TEST(Transform3DTest, SetRotation_Radians)
{
    Transform3D t;
    t.SetRotation(0.1f, 0.2f, 0.3f);
    const auto& r = t.GetRotation();
    EXPECT_NEAR(r.x, 0.1f, 1e-5f);
    EXPECT_NEAR(r.y, 0.2f, 1e-5f);
    EXPECT_NEAR(r.z, 0.3f, 1e-5f);
}

TEST(Transform3DTest, SetScale_Uniform)
{
    Transform3D t;
    t.SetScale(2.0f);
    const auto& s = t.GetScale();
    EXPECT_NEAR(s.x, 2.0f, 1e-5f);
    EXPECT_NEAR(s.y, 2.0f, 1e-5f);
    EXPECT_NEAR(s.z, 2.0f, 1e-5f);
}

TEST(Transform3DTest, SetScale_NonUniform)
{
    Transform3D t;
    t.SetScale(1.0f, 2.0f, 3.0f);
    const auto& s = t.GetScale();
    EXPECT_NEAR(s.x, 1.0f, 1e-5f);
    EXPECT_NEAR(s.y, 2.0f, 1e-5f);
    EXPECT_NEAR(s.z, 3.0f, 1e-5f);
}

TEST(Transform3DTest, WorldMatrix_IdentityTransform)
{
    Transform3D t;
    Matrix4x4 mat;
    mat = t.GetWorldMatrix();
    EXPECT_NEAR(mat._11, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._22, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._33, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._44, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._41, 0.0f, 1e-5f);
    EXPECT_NEAR(mat._42, 0.0f, 1e-5f);
    EXPECT_NEAR(mat._43, 0.0f, 1e-5f);
}

TEST(Transform3DTest, WorldMatrix_TranslationOnly)
{
    Transform3D t;
    t.SetPosition(5.0f, 0.0f, 0.0f);
    Matrix4x4 mat;
    mat = t.GetWorldMatrix();
    EXPECT_NEAR(mat._41, 5.0f, 1e-5f);
    EXPECT_NEAR(mat._42, 0.0f, 1e-5f);
    EXPECT_NEAR(mat._43, 0.0f, 1e-5f);
}

TEST(Transform3DTest, WorldMatrix_ScaleOnly)
{
    Transform3D t;
    t.SetScale(2.0f);
    Matrix4x4 mat;
    mat = t.GetWorldMatrix();
    EXPECT_NEAR(mat._11, 2.0f, 1e-5f);
    EXPECT_NEAR(mat._22, 2.0f, 1e-5f);
    EXPECT_NEAR(mat._33, 2.0f, 1e-5f);
}

TEST(Transform3DTest, WorldInverseTranspose_Exists)
{
    Transform3D t;
    t.SetPosition(1.0f, 2.0f, 3.0f);
    t.SetScale(2.0f, 1.0f, 0.5f);
    Matrix4x4 mat = t.GetWorldInverseTranspose();
    // NaN値がないことを確認
    EXPECT_FALSE(std::isnan(mat._11));
    EXPECT_FALSE(std::isnan(mat._22));
    EXPECT_FALSE(std::isnan(mat._33));
    EXPECT_FALSE(std::isnan(mat._44));
}
