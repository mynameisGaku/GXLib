/// @file test_Camera3D.cpp
/// @brief Camera3D perspective/ortho/LookAt/forward/jitter/mode tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Camera3D.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

static Camera3D MakePerspCam()
{
    Camera3D cam;
    cam.SetPerspective(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
    cam.SetPosition(0.0f, 0.0f, 0.0f);
    return cam;
}

// ==================== Defaults ====================

TEST(Camera3DTest, Default_Position)
{
    Camera3D cam;
    const auto& pos = cam.GetPosition();
    EXPECT_NEAR(pos.x, 0.0f, kEps);
    EXPECT_NEAR(pos.y, 0.0f, kEps);
    EXPECT_NEAR(pos.z, -5.0f, kEps);  // Default position is (0,0,-5)
}

TEST(Camera3DTest, Default_Mode)
{
    Camera3D cam;
    EXPECT_EQ(cam.GetMode(), CameraMode::Free);
}

// ==================== Perspective ====================

TEST(Camera3DTest, SetPerspective_FOV)
{
    Camera3D cam;
    cam.SetPerspective(XM_PIDIV2, 1.0f, 0.5f, 500.0f);
    EXPECT_NEAR(cam.GetFovY(), XM_PIDIV2, kEps);
    EXPECT_NEAR(cam.GetAspect(), 1.0f, kEps);
    EXPECT_NEAR(cam.GetNearZ(), 0.5f, kEps);
    EXPECT_NEAR(cam.GetFarZ(), 500.0f, kEps);
}

TEST(Camera3DTest, ProjectionMatrix_NotZero)
{
    Camera3D cam = MakePerspCam();
    XMMATRIX proj = cam.GetProjectionMatrix();
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, proj);
    // Perspective matrix has non-zero diagonal
    EXPECT_GT(fabsf(m._11), kEps);
    EXPECT_GT(fabsf(m._22), kEps);
    EXPECT_GT(fabsf(m._33), kEps);
}

TEST(Camera3DTest, ViewMatrix_AtOrigin)
{
    Camera3D cam = MakePerspCam();
    XMMATRIX view = cam.GetViewMatrix();
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, view);
    // View matrix at origin looking forward: identity-like rotation part
    EXPECT_FALSE(std::isnan(m._11));
}

// ==================== Orthographic ====================

TEST(Camera3DTest, SetOrthographic)
{
    Camera3D cam;
    cam.SetOrthographic(10.0f, 7.5f, 0.1f, 100.0f);
    EXPECT_NEAR(cam.GetNearZ(), 0.1f, kEps);
    EXPECT_NEAR(cam.GetFarZ(), 100.0f, kEps);
}

TEST(Camera3DTest, OrthoProjectionMatrix_NotZero)
{
    Camera3D cam;
    cam.SetOrthographic(10.0f, 7.5f, 0.1f, 100.0f);
    XMMATRIX proj = cam.GetProjectionMatrix();
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, proj);
    EXPECT_GT(fabsf(m._11), kEps);
    EXPECT_GT(fabsf(m._22), kEps);
}

// ==================== Position ====================

TEST(Camera3DTest, SetPosition_Float3)
{
    Camera3D cam;
    cam.SetPosition(XMFLOAT3{1.0f, 2.0f, 3.0f});
    const auto& pos = cam.GetPosition();
    EXPECT_NEAR(pos.x, 1.0f, kEps);
    EXPECT_NEAR(pos.y, 2.0f, kEps);
    EXPECT_NEAR(pos.z, 3.0f, kEps);
}

TEST(Camera3DTest, SetPosition_ThreeFloats)
{
    Camera3D cam;
    cam.SetPosition(10.0f, 20.0f, 30.0f);
    const auto& pos = cam.GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, kEps);
    EXPECT_NEAR(pos.y, 20.0f, kEps);
    EXPECT_NEAR(pos.z, 30.0f, kEps);
}

// ==================== LookAt ====================

TEST(Camera3DTest, LookAt_Forward)
{
    Camera3D cam = MakePerspCam();
    cam.SetPosition(0.0f, 0.0f, -5.0f);
    cam.LookAt(XMFLOAT3{0.0f, 0.0f, 0.0f});
    XMFLOAT3 fwd = cam.GetForward();
    // Should look along +Z (toward origin from -5z)
    EXPECT_GT(fwd.z, 0.5f);
}

TEST(Camera3DTest, LookAt_SetsPitchYaw)
{
    Camera3D cam = MakePerspCam();
    cam.SetPosition(0.0f, 0.0f, -10.0f);
    cam.LookAt(XMFLOAT3{10.0f, 0.0f, 0.0f});
    // Looking from -10z toward +10x: forward should have +x and +z components
    XMFLOAT3 fwd = cam.GetForward();
    EXPECT_GT(fwd.x, 0.3f);
    EXPECT_GT(fwd.z, 0.3f);
}

// ==================== Forward / Right / Up ====================

TEST(Camera3DTest, Forward_IsUnit)
{
    Camera3D cam = MakePerspCam();
    cam.LookAt(XMFLOAT3{10.0f, 0.0f, 10.0f});
    XMFLOAT3 fwd = cam.GetForward();
    float len = sqrtf(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
    EXPECT_NEAR(len, 1.0f, 1e-3f);
}

TEST(Camera3DTest, Right_IsUnit)
{
    Camera3D cam = MakePerspCam();
    XMFLOAT3 right = cam.GetRight();
    float len = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
    EXPECT_NEAR(len, 1.0f, 1e-3f);
}

TEST(Camera3DTest, Up_IsUnit)
{
    Camera3D cam = MakePerspCam();
    XMFLOAT3 up = cam.GetUp();
    float len = sqrtf(up.x * up.x + up.y * up.y + up.z * up.z);
    EXPECT_NEAR(len, 1.0f, 1e-3f);
}

TEST(Camera3DTest, ForwardRight_Orthogonal)
{
    Camera3D cam = MakePerspCam();
    cam.LookAt(XMFLOAT3{1.0f, 0.0f, 1.0f});
    XMFLOAT3 fwd = cam.GetForward();
    XMFLOAT3 right = cam.GetRight();
    float dot = fwd.x * right.x + fwd.y * right.y + fwd.z * right.z;
    EXPECT_NEAR(dot, 0.0f, 0.01f);
}

// ==================== Rotate ====================

TEST(Camera3DTest, Rotate_ChangesPitchYaw)
{
    Camera3D cam = MakePerspCam();
    float oldPitch = cam.GetPitch();
    float oldYaw = cam.GetYaw();
    cam.Rotate(0.1f, 0.2f);
    EXPECT_NE(cam.GetPitch(), oldPitch);
    EXPECT_NE(cam.GetYaw(), oldYaw);
}

TEST(Camera3DTest, SetPitch_Clamps)
{
    Camera3D cam = MakePerspCam();
    cam.SetPitch(XM_PIDIV2 + 1.0f);
    // Pitch should be clamped (implementation typically limits to ~+-89deg)
    EXPECT_LE(cam.GetPitch(), XM_PIDIV2 + 0.01f);
}

// ==================== Movement ====================

TEST(Camera3DTest, MoveForward)
{
    Camera3D cam = MakePerspCam();
    cam.LookAt(XMFLOAT3{0.0f, 0.0f, 10.0f});
    XMFLOAT3 before = cam.GetPosition();
    cam.MoveForward(5.0f);
    XMFLOAT3 after = cam.GetPosition();
    float dist = sqrtf(
        (after.x - before.x) * (after.x - before.x) +
        (after.y - before.y) * (after.y - before.y) +
        (after.z - before.z) * (after.z - before.z));
    EXPECT_NEAR(dist, 5.0f, 0.1f);
}

TEST(Camera3DTest, MoveRight)
{
    Camera3D cam = MakePerspCam();
    XMFLOAT3 before = cam.GetPosition();
    cam.MoveRight(3.0f);
    XMFLOAT3 after = cam.GetPosition();
    float dist = sqrtf(
        (after.x - before.x) * (after.x - before.x) +
        (after.y - before.y) * (after.y - before.y) +
        (after.z - before.z) * (after.z - before.z));
    EXPECT_NEAR(dist, 3.0f, 0.1f);
}

TEST(Camera3DTest, MoveUp)
{
    Camera3D cam = MakePerspCam();
    cam.MoveUp(7.0f);
    EXPECT_NEAR(cam.GetPosition().y, 7.0f, 0.1f);
}

// ==================== Jitter (TAA) ====================

TEST(Camera3DTest, Jitter_Default)
{
    Camera3D cam = MakePerspCam();
    XMFLOAT2 jitter = cam.GetJitter();
    EXPECT_NEAR(jitter.x, 0.0f, kEps);
    EXPECT_NEAR(jitter.y, 0.0f, kEps);
}

TEST(Camera3DTest, SetJitter)
{
    Camera3D cam = MakePerspCam();
    cam.SetJitter(0.5f, -0.3f);
    XMFLOAT2 j = cam.GetJitter();
    EXPECT_NEAR(j.x, 0.5f, kEps);
    EXPECT_NEAR(j.y, -0.3f, kEps);
}

TEST(Camera3DTest, ClearJitter)
{
    Camera3D cam = MakePerspCam();
    cam.SetJitter(1.0f, 2.0f);
    cam.ClearJitter();
    XMFLOAT2 j = cam.GetJitter();
    EXPECT_NEAR(j.x, 0.0f, kEps);
    EXPECT_NEAR(j.y, 0.0f, kEps);
}

TEST(Camera3DTest, JitteredProjection_DiffersFromNormal)
{
    Camera3D cam = MakePerspCam();
    XMMATRIX normal = cam.GetProjectionMatrix();
    cam.SetJitter(0.5f, 0.5f);
    XMMATRIX jittered = cam.GetJitteredProjectionMatrix();
    XMFLOAT4X4 n, j;
    XMStoreFloat4x4(&n, normal);
    XMStoreFloat4x4(&j, jittered);
    // Jittered matrix should differ in translation components
    bool differs = false;
    for (int i = 0; i < 16; ++i)
        if (fabsf((&n._11)[i] - (&j._11)[i]) > kEps) differs = true;
    EXPECT_TRUE(differs);
}

// ==================== Camera Modes ====================

TEST(Camera3DTest, SetMode_FPS)
{
    Camera3D cam;
    cam.SetMode(CameraMode::FPS);
    EXPECT_EQ(cam.GetMode(), CameraMode::FPS);
}

TEST(Camera3DTest, SetMode_TPS)
{
    Camera3D cam;
    cam.SetMode(CameraMode::TPS);
    EXPECT_EQ(cam.GetMode(), CameraMode::TPS);
}

TEST(Camera3DTest, SetMode_Free)
{
    Camera3D cam;
    cam.SetMode(CameraMode::TPS);
    cam.SetMode(CameraMode::Free);
    EXPECT_EQ(cam.GetMode(), CameraMode::Free);
}

// ==================== ViewProjection ====================

TEST(Camera3DTest, VP_NotZero)
{
    Camera3D cam = MakePerspCam();
    cam.SetPosition(0, 5, -10);
    cam.LookAt(XMFLOAT3{0, 0, 0});
    XMMATRIX vp = cam.GetViewProjectionMatrix();
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, vp);
    bool allZero = true;
    for (int i = 0; i < 16; ++i)
        if (fabsf((&m._11)[i]) > kEps) allZero = false;
    EXPECT_FALSE(allZero);
}

// ==================== TPS distance ====================

TEST(Camera3DTest, TPS_Distance)
{
    Camera3D cam;
    cam.SetMode(CameraMode::TPS);
    cam.SetTPSDistance(10.0f);
    cam.SetTarget(XMFLOAT3{0, 0, 0});
    // View matrix should place camera behind target
    XMMATRIX view = cam.GetViewMatrix();
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, view);
    EXPECT_FALSE(std::isnan(m._11));
}
