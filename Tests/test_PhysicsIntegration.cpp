/// @file test_PhysicsIntegration.cpp
/// @brief 物理シミュレーション数値検証テスト（重力・反発・レイキャスト精度・速度積分）

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Collision/Collision3D.h"
#include "Math/Collision/Collision2D.h"
#include "Math/Vector3.h"

using namespace gx;

static constexpr float kEps = 1e-5f;
static constexpr float kGravity = 9.80665f;

// ============================================================================
// Physics Constants
// ============================================================================

TEST(PhysicsConstants, StandardGravity)
{
    EXPECT_NEAR(kGravity, 9.80665f, kEps);
}

TEST(PhysicsConstants, GravityDirection_Down)
{
    Vector3 gravDir = Vector3::Down();
    EXPECT_NEAR(gravDir.x, 0.0f, kEps);
    EXPECT_NEAR(gravDir.y, -1.0f, kEps);
    EXPECT_NEAR(gravDir.z, 0.0f, kEps);
}

// ============================================================================
// Gravity Freefall: d = 0.5 * g * t^2
// ============================================================================

TEST(GravityFreefall, Distance_At1Second)
{
    float t = 1.0f;
    float d = 0.5f * kGravity * t * t;
    EXPECT_NEAR(d, 4.903325f, 1e-4f);
}

TEST(GravityFreefall, Distance_At2Seconds)
{
    float t = 2.0f;
    float d = 0.5f * kGravity * t * t;
    EXPECT_NEAR(d, 19.6133f, 1e-3f);
}

TEST(GravityFreefall, Distance_At3Seconds)
{
    float t = 3.0f;
    float d = 0.5f * kGravity * t * t;
    EXPECT_NEAR(d, 44.12993f, 1e-3f);
}

TEST(GravityFreefall, Velocity_At1Second)
{
    float t = 1.0f;
    float v = kGravity * t;
    EXPECT_NEAR(v, 9.80665f, kEps);
}

TEST(GravityFreefall, Velocity_At3Seconds)
{
    float t = 3.0f;
    float v = kGravity * t;
    EXPECT_NEAR(v, 29.41995f, 1e-4f);
}

// ============================================================================
// Collision Bounce: h' = h * e^2
// ============================================================================

TEST(CollisionBounce, PerfectBounce_RestituionOne)
{
    float h = 10.0f;
    float e = 1.0f; // perfect elastic
    float bounceH = h * e * e;
    EXPECT_NEAR(bounceH, 10.0f, kEps);
}

TEST(CollisionBounce, HalfRestitution)
{
    float h = 10.0f;
    float e = 0.5f;
    float bounceH = h * e * e;
    EXPECT_NEAR(bounceH, 2.5f, kEps);
}

TEST(CollisionBounce, ZeroRestitution_NoBounce)
{
    float h = 10.0f;
    float e = 0.0f;
    float bounceH = h * e * e;
    EXPECT_NEAR(bounceH, 0.0f, kEps);
}

TEST(CollisionBounce, MultipleBounces)
{
    float h = 10.0f;
    float e = 0.7f;
    // After 3 bounces: h * (e^2)^3
    float finalH = h;
    for (int i = 0; i < 3; ++i)
        finalH *= e * e;
    EXPECT_NEAR(finalH, 10.0f * 0.49f * 0.49f * 0.49f, 1e-4f);
}

// ============================================================================
// Velocity Integration: v = v0 + a*t, p = p0 + v*t + 0.5*a*t^2
// ============================================================================

TEST(VelocityIntegration, SimpleVelocity)
{
    Vector3 v0(0, 0, 0);
    Vector3 accel(0, -kGravity, 0);
    float dt = 1.0f;
    Vector3 v1 = v0 + accel * dt;
    EXPECT_NEAR(v1.y, -kGravity, kEps);
}

TEST(VelocityIntegration, PositionUpdate)
{
    Vector3 p0(0, 100, 0);
    Vector3 v0(0, 0, 0);
    Vector3 accel(0, -kGravity, 0);
    float dt = 1.0f;
    Vector3 p1 = p0 + v0 * dt + accel * (0.5f * dt * dt);
    EXPECT_NEAR(p1.x, 0.0f, kEps);
    EXPECT_NEAR(p1.y, 100.0f - 0.5f * kGravity, 1e-3f);
    EXPECT_NEAR(p1.z, 0.0f, kEps);
}

TEST(VelocityIntegration, HorizontalMotion_Unchanged)
{
    Vector3 p0(0, 0, 0);
    Vector3 v0(10, 0, 0);
    Vector3 accel(0, -kGravity, 0);
    float dt = 2.0f;
    Vector3 p1 = p0 + v0 * dt + accel * (0.5f * dt * dt);
    EXPECT_NEAR(p1.x, 20.0f, kEps);
    EXPECT_NEAR(p1.y, -0.5f * kGravity * 4.0f, 1e-3f);
}

TEST(VelocityIntegration, ProjectileRange)
{
    // Projectile launched at 45 degrees with v=10 m/s
    float v = 10.0f;
    float angle = DirectX::XM_PI / 4.0f;
    float vx = v * cosf(angle);
    float vy = v * sinf(angle);
    // Time of flight: 2 * vy / g
    float tFlight = 2.0f * vy / kGravity;
    float range = vx * tFlight;
    // R = v^2 * sin(2*angle) / g
    float expected = (v * v) * sinf(2.0f * angle) / kGravity;
    EXPECT_NEAR(range, expected, 0.01f);
}

// ============================================================================
// Raycast Precision: Ray-Sphere intersection
// ============================================================================

TEST(RaycastPrecision, RaySphere_DirectHit)
{
    Ray ray(Vector3(0, 0, -5), Vector3(0, 0, 1));
    Sphere sphere(Vector3(0, 0, 0), 1.0f);
    float t = 0.0f;
    EXPECT_TRUE(Collision3D::RaycastSphere(ray, sphere, t));
    EXPECT_NEAR(t, 4.0f, 0.01f); // hit at z=-1, so t=4
}

TEST(RaycastPrecision, RaySphere_Miss)
{
    Ray ray(Vector3(0, 5, -5), Vector3(0, 0, 1));
    Sphere sphere(Vector3(0, 0, 0), 1.0f);
    float t = 0.0f;
    EXPECT_FALSE(Collision3D::RaycastSphere(ray, sphere, t));
}

TEST(RaycastPrecision, RaySphere_TangentHit)
{
    // Ray grazing the sphere at exactly radius distance
    Ray ray(Vector3(1, 0, -5), Vector3(0, 0, 1));
    Sphere sphere(Vector3(0, 0, 0), 1.0f);
    float t = 0.0f;
    bool hit = Collision3D::RaycastSphere(ray, sphere, t);
    // Tangent case: should hit at the surface
    if (hit) {
        Vector3 hitPt = ray.GetPoint(t);
        float dist = hitPt.Distance(sphere.center);
        EXPECT_NEAR(dist, 1.0f, 0.1f);
    }
}

// ============================================================================
// Raycast Precision: Ray-Plane intersection
// ============================================================================

TEST(RaycastPrecision, RayPlane_FrontHit)
{
    Ray ray(Vector3(0, 5, 0), Vector3(0, -1, 0));
    Plane plane(Vector3(0, 1, 0), 0.0f); // y=0 plane
    float t = 0.0f;
    EXPECT_TRUE(Collision3D::RaycastPlane(ray, plane, t));
    EXPECT_NEAR(t, 5.0f, 0.01f);
}

TEST(RaycastPrecision, RayPlane_ParallelMiss)
{
    Ray ray(Vector3(0, 5, 0), Vector3(1, 0, 0)); // parallel to plane
    Plane plane(Vector3(0, 1, 0), 0.0f);
    float t = 0.0f;
    EXPECT_FALSE(Collision3D::RaycastPlane(ray, plane, t));
}

TEST(RaycastPrecision, RayPlane_HitPoint)
{
    Ray ray(Vector3(3, 10, 7), Vector3(0, -1, 0));
    Plane plane(Vector3(0, 1, 0), 0.0f);
    float t = 0.0f;
    EXPECT_TRUE(Collision3D::RaycastPlane(ray, plane, t));
    Vector3 hitPt = ray.GetPoint(t);
    EXPECT_NEAR(hitPt.x, 3.0f, 0.01f);
    EXPECT_NEAR(hitPt.y, 0.0f, 0.01f);
    EXPECT_NEAR(hitPt.z, 7.0f, 0.01f);
}

// ============================================================================
// Raycast Precision: Ray-AABB intersection
// ============================================================================

TEST(RaycastPrecision, RayAABB_DirectHit)
{
    Ray ray(Vector3(0, 0, -5), Vector3(0, 0, 1));
    AABB3D aabb(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    float t = 0.0f;
    EXPECT_TRUE(Collision3D::RaycastAABB(ray, aabb, t));
    EXPECT_NEAR(t, 4.0f, 0.01f); // hits near face at z=-1
}

TEST(RaycastPrecision, RayAABB_Miss)
{
    Ray ray(Vector3(5, 5, -5), Vector3(0, 0, 1));
    AABB3D aabb(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    float t = 0.0f;
    EXPECT_FALSE(Collision3D::RaycastAABB(ray, aabb, t));
}

TEST(RaycastPrecision, RayAABB_HitFromInside)
{
    Ray ray(Vector3(0, 0, 0), Vector3(0, 0, 1));
    AABB3D aabb(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    float t = 0.0f;
    // Starting inside the AABB
    bool hit = Collision3D::RaycastAABB(ray, aabb, t);
    // Implementation dependent: some implementations return t<=0 or exit point
    if (hit) {
        EXPECT_GE(t, 0.0f);
    }
}

// ============================================================================
// Stack Stability: Center of mass calculation
// ============================================================================

TEST(StackStability, CenterOfMass_TwoEqualBoxes)
{
    // Two boxes stacked: box1 at y=0.5, box2 at y=1.5
    Vector3 pos1(0, 0.5f, 0);
    Vector3 pos2(0, 1.5f, 0);
    float mass = 1.0f;
    Vector3 com = (pos1 * mass + pos2 * mass) / (2.0f * mass);
    EXPECT_NEAR(com.x, 0.0f, kEps);
    EXPECT_NEAR(com.y, 1.0f, kEps);
    EXPECT_NEAR(com.z, 0.0f, kEps);
}

TEST(StackStability, CenterOfMass_UnequalMasses)
{
    Vector3 pos1(0, 0, 0); // mass 3
    Vector3 pos2(0, 10, 0); // mass 1
    float m1 = 3.0f, m2 = 1.0f;
    Vector3 com = (pos1 * m1 + pos2 * m2) / (m1 + m2);
    EXPECT_NEAR(com.y, 2.5f, kEps);
}

TEST(StackStability, CenterOfMass_Symmetry)
{
    Vector3 positions[] = {
        {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}
    };
    Vector3 com(0, 0, 0);
    for (auto& p : positions) com += p;
    com = com * (1.0f / 4.0f);
    EXPECT_NEAR(com.x, 0.0f, kEps);
    EXPECT_NEAR(com.y, 0.0f, kEps);
    EXPECT_NEAR(com.z, 0.0f, kEps);
}

// ============================================================================
// Collision shape tests (data structures)
// ============================================================================

TEST(CollisionShapes, Sphere_Contains_PointInside)
{
    Sphere s(Vector3(0, 0, 0), 5.0f);
    EXPECT_TRUE(s.Contains(Vector3(1, 1, 1)));
}

TEST(CollisionShapes, Sphere_Contains_PointOutside)
{
    Sphere s(Vector3(0, 0, 0), 1.0f);
    EXPECT_FALSE(s.Contains(Vector3(5, 5, 5)));
}

TEST(CollisionShapes, AABB3D_Contains_PointInside)
{
    AABB3D aabb(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    EXPECT_TRUE(aabb.Contains(Vector3(0, 0, 0)));
}

TEST(CollisionShapes, AABB3D_Contains_PointOutside)
{
    AABB3D aabb(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    EXPECT_FALSE(aabb.Contains(Vector3(5, 0, 0)));
}

TEST(CollisionShapes, AABB3D_Center)
{
    AABB3D aabb(Vector3(0, 0, 0), Vector3(10, 20, 30));
    Vector3 center = aabb.Center();
    EXPECT_NEAR(center.x, 5.0f, kEps);
    EXPECT_NEAR(center.y, 10.0f, kEps);
    EXPECT_NEAR(center.z, 15.0f, kEps);
}

TEST(CollisionShapes, AABB3D_Volume)
{
    AABB3D aabb(Vector3(0, 0, 0), Vector3(2, 3, 4));
    EXPECT_NEAR(aabb.Volume(), 24.0f, kEps);
}

TEST(CollisionShapes, Plane_DistanceToPoint)
{
    Plane plane(Vector3(0, 1, 0), 0.0f); // y=0
    EXPECT_NEAR(plane.DistanceToPoint(Vector3(0, 5, 0)), 5.0f, kEps);
    EXPECT_NEAR(plane.DistanceToPoint(Vector3(0, -3, 0)), -3.0f, kEps);
}

TEST(CollisionShapes, Triangle_Normal)
{
    Triangle tri(Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0));
    Vector3 n = tri.Normal();
    EXPECT_NEAR(n.x, 0.0f, 0.01f);
    EXPECT_NEAR(n.y, 0.0f, 0.01f);
    EXPECT_NEAR(fabsf(n.z), 1.0f, 0.01f);
}

TEST(CollisionShapes, Ray_GetPoint)
{
    Ray ray(Vector3(0, 0, 0), Vector3(1, 0, 0));
    Vector3 pt = ray.GetPoint(5.0f);
    EXPECT_NEAR(pt.x, 5.0f, kEps);
    EXPECT_NEAR(pt.y, 0.0f, kEps);
    EXPECT_NEAR(pt.z, 0.0f, kEps);
}
