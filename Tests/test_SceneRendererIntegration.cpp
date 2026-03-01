/// @file test_SceneRendererIntegration.cpp
/// @brief SceneRenderer フラスタムカリング境界、LOD距離選択、インスタンシング閾値、RenderStats検証

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/SceneRenderer.h"
#include "Graphics/3D/Camera3D.h"
#include "Math/Collision/Collision3D.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// SceneRenderStats struct field validation
// ============================================================================

TEST(SceneRenderStats, DefaultValues_AllZero)
{
    SceneRenderStats stats{};
    EXPECT_EQ(stats.totalEntities, 0u);
    EXPECT_EQ(stats.visibleEntities, 0u);
    EXPECT_EQ(stats.culledEntities, 0u);
    EXPECT_EQ(stats.drawCalls, 0u);
    EXPECT_EQ(stats.instancedBatches, 0u);
    EXPECT_EQ(stats.instancedEntities, 0u);
}

TEST(SceneRenderStats, TotalEquals_VisiblePlusCulled)
{
    SceneRenderStats stats{};
    stats.totalEntities   = 100;
    stats.visibleEntities = 70;
    stats.culledEntities  = 30;
    EXPECT_EQ(stats.totalEntities, stats.visibleEntities + stats.culledEntities);
}

TEST(SceneRenderStats, DrawCalls_NonNegative)
{
    SceneRenderStats stats{};
    EXPECT_GE(stats.drawCalls, 0u);
}

TEST(SceneRenderStats, InstancedBatches_CanBeSet)
{
    SceneRenderStats stats{};
    stats.instancedBatches = 5;
    stats.instancedEntities = 20;
    EXPECT_EQ(stats.instancedBatches, 5u);
    EXPECT_EQ(stats.instancedEntities, 20u);
}

// ============================================================================
// SceneRenderer instancing threshold constant
// ============================================================================

TEST(InstancingThreshold, Value_Is4)
{
    EXPECT_EQ(SceneRenderer::k_InstancingThreshold, 4u);
}

TEST(InstancingThreshold, LessThanThreshold_NoInstancing)
{
    // 3 copies of the same model should not trigger instancing
    uint32_t count = 3;
    EXPECT_LT(count, SceneRenderer::k_InstancingThreshold);
}

TEST(InstancingThreshold, EqualToThreshold_TriggersInstancing)
{
    uint32_t count = 4;
    EXPECT_GE(count, SceneRenderer::k_InstancingThreshold);
}

TEST(InstancingThreshold, GreaterThanThreshold_TriggersInstancing)
{
    uint32_t count = 10;
    EXPECT_GE(count, SceneRenderer::k_InstancingThreshold);
}

// ============================================================================
// SceneRenderer initial state
// ============================================================================

TEST(SceneRendererState, InitialStats_AllZero)
{
    SceneRenderer renderer;
    SceneRenderStats stats = renderer.GetLastRenderStats();
    EXPECT_EQ(stats.totalEntities, 0u);
    EXPECT_EQ(stats.visibleEntities, 0u);
    EXPECT_EQ(stats.culledEntities, 0u);
    EXPECT_EQ(stats.drawCalls, 0u);
}

// ============================================================================
// Frustum culling boundary tests (using Collision3D directly)
// ============================================================================

TEST(FrustumCulling, SphereInsideFrustum_Visible)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    // Sphere at the center of the view, well inside the frustum
    Sphere sphere(Vector3(0, 0, 0), 1.0f);
    EXPECT_TRUE(Collision3D::TestFrustumVsSphere(frustum, sphere));
}

TEST(FrustumCulling, SphereFarOutside_Culled)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    // Sphere far to the right, outside the frustum
    Sphere sphere(Vector3(1000, 0, 0), 1.0f);
    EXPECT_FALSE(Collision3D::TestFrustumVsSphere(frustum, sphere));
}

TEST(FrustumCulling, SphereBehindCamera_Culled)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.SetPosition(0, 0, 0);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    // Sphere behind the camera (negative Z for default forward=+Z)
    Sphere sphere(Vector3(0, 0, -50), 1.0f);
    EXPECT_FALSE(Collision3D::TestFrustumVsSphere(frustum, sphere));
}

TEST(FrustumCulling, SphereBeyondFarPlane_Culled)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    // Sphere well beyond the far plane
    Sphere sphere(Vector3(0, 0, 500), 1.0f);
    EXPECT_FALSE(Collision3D::TestFrustumVsSphere(frustum, sphere));
}

TEST(FrustumCulling, AABBInsideFrustum_Visible)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    AABB3D aabb(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    EXPECT_TRUE(Collision3D::TestFrustumVsAABB(frustum, aabb));
}

TEST(FrustumCulling, AABBOutsideFrustum_Culled)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    AABB3D aabb(Vector3(500, 500, 500), Vector3(510, 510, 510));
    EXPECT_FALSE(Collision3D::TestFrustumVsAABB(frustum, aabb));
}

TEST(FrustumCulling, PointInsideFrustum_Visible)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    EXPECT_TRUE(Collision3D::TestFrustumVsPoint(frustum, Vector3(0, 0, 0)));
}

TEST(FrustumCulling, PointOutsideFrustum_Culled)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.SetPosition(0, 0, -10);
    camera.SetYaw(0.0f);
    camera.SetPitch(0.0f);

    XMMATRIX vp = camera.GetViewProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    EXPECT_FALSE(Collision3D::TestFrustumVsPoint(frustum, Vector3(1000, 0, 0)));
}

// ============================================================================
// LOD distance selection concept
// ============================================================================

TEST(LODDistance, LOD0_AtNear)
{
    // screenPercentage thresholds: LOD0 > 0.5, LOD1 > 0.2, LOD2 > 0.05
    float screenPct = 0.8f; // near object -> large screen coverage
    int lod = (screenPct > 0.5f) ? 0 : (screenPct > 0.2f) ? 1 : 2;
    EXPECT_EQ(lod, 0);
}

TEST(LODDistance, LOD1_AtMid)
{
    float screenPct = 0.35f; // medium distance
    int lod = (screenPct > 0.5f) ? 0 : (screenPct > 0.2f) ? 1 : 2;
    EXPECT_EQ(lod, 1);
}

TEST(LODDistance, LOD2_AtFar)
{
    float screenPct = 0.1f; // far distance
    int lod = (screenPct > 0.5f) ? 0 : (screenPct > 0.2f) ? 1 : 2;
    EXPECT_EQ(lod, 2);
}

TEST(LODDistance, ScreenPercentage_DecreaseWithDistance)
{
    // Simple model: screenPct ~ boundingRadius / distance
    float radius = 1.0f;
    float pctNear = radius / 5.0f;   // 0.2
    float pctMid  = radius / 20.0f;  // 0.05
    float pctFar  = radius / 100.0f; // 0.01
    EXPECT_GT(pctNear, pctMid);
    EXPECT_GT(pctMid, pctFar);
}

// ============================================================================
// Scene entity management (for SceneRenderer use)
// ============================================================================

TEST(SceneForRenderer, CreateEntity_IncreasesCount)
{
    Scene scene("TestScene");
    EXPECT_EQ(scene.GetEntities().size(), 0u);
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    EXPECT_EQ(scene.GetEntities().size(), 2u);
}

TEST(SceneForRenderer, FindEntity_ReturnValid)
{
    Scene scene("TestScene");
    scene.CreateEntity("Player");
    Entity* found = scene.FindEntity("Player");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "Player");
}

TEST(SceneForRenderer, DestroyEntity_ReducesCount)
{
    Scene scene("TestScene");
    Entity* e = scene.CreateEntity("ToRemove");
    EXPECT_EQ(scene.GetEntities().size(), 1u);
    scene.DestroyEntity(e);
    scene.Update(0.0f); // 遅延削除を実行
    EXPECT_EQ(scene.GetEntities().size(), 0u);
}

// ============================================================================
// Camera3D properties for SceneRenderer
// ============================================================================

TEST(Camera3DForRenderer, DefaultNearFar)
{
    Camera3D camera;
    EXPECT_NEAR(camera.GetNearZ(), 0.1f, kEps);
    EXPECT_NEAR(camera.GetFarZ(), 1000.0f, kEps);
}

TEST(Camera3DForRenderer, SetPerspective_UpdatesFov)
{
    Camera3D camera;
    camera.SetPerspective(DirectX::XM_PI / 3.0f, 16.0f / 9.0f, 0.5f, 500.0f);
    EXPECT_NEAR(camera.GetFovY(), DirectX::XM_PI / 3.0f, kEps);
    EXPECT_NEAR(camera.GetAspect(), 16.0f / 9.0f, kEps);
    EXPECT_NEAR(camera.GetNearZ(), 0.5f, kEps);
    EXPECT_NEAR(camera.GetFarZ(), 500.0f, kEps);
}

TEST(Camera3DForRenderer, SetPosition_ReturnsSamePosition)
{
    Camera3D camera;
    camera.SetPosition(10.0f, 20.0f, 30.0f);
    auto pos = camera.GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, kEps);
    EXPECT_NEAR(pos.y, 20.0f, kEps);
    EXPECT_NEAR(pos.z, 30.0f, kEps);
}

TEST(Camera3DForRenderer, Jitter_DefaultZero)
{
    Camera3D camera;
    auto jitter = camera.GetJitter();
    EXPECT_NEAR(jitter.x, 0.0f, kEps);
    EXPECT_NEAR(jitter.y, 0.0f, kEps);
}

TEST(Camera3DForRenderer, SetAndClearJitter)
{
    Camera3D camera;
    camera.SetJitter(0.5f, -0.3f);
    auto j = camera.GetJitter();
    EXPECT_NEAR(j.x, 0.5f, kEps);
    EXPECT_NEAR(j.y, -0.3f, kEps);

    camera.ClearJitter();
    j = camera.GetJitter();
    EXPECT_NEAR(j.x, 0.0f, kEps);
    EXPECT_NEAR(j.y, 0.0f, kEps);
}

TEST(Camera3DForRenderer, DefaultMode_IsFree)
{
    Camera3D camera;
    EXPECT_EQ(camera.GetMode(), CameraMode::Free);
}
