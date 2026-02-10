#pragma once
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"

namespace GX {

// --- 3D形状定義 ---

/// @brief 3D軸平行境界ボックス（AABB）
struct AABB3D {
    Vector3 min, max;
    AABB3D() = default;
    AABB3D(const Vector3& min, const Vector3& max) : min(min), max(max) {}

    Vector3 Center() const { return (min + max) * 0.5f; }
    Vector3 Size() const { return max - min; }
    Vector3 HalfExtents() const { return (max - min) * 0.5f; }

    bool Contains(const Vector3& point) const
    {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    AABB3D Expand(float margin) const
    {
        return { { min.x - margin, min.y - margin, min.z - margin },
                 { max.x + margin, max.y + margin, max.z + margin } };
    }

    AABB3D Merged(const AABB3D& other) const
    {
        return { Vector3::Min(min, other.min), Vector3::Max(max, other.max) };
    }

    float Volume() const
    {
        Vector3 s = Size();
        return s.x * s.y * s.z;
    }

    float SurfaceArea() const
    {
        Vector3 s = Size();
        return 2.0f * (s.x * s.y + s.y * s.z + s.z * s.x);
    }
};

/// @brief 3D球
struct Sphere {
    Vector3 center;
    float radius = 0.0f;
    Sphere() = default;
    Sphere(const Vector3& c, float r) : center(c), radius(r) {}

    bool Contains(const Vector3& point) const
    {
        return center.DistanceSquared(point) <= radius * radius;
    }
};

/// @brief 3Dレイ
struct Ray {
    Vector3 origin;
    Vector3 direction;
    Ray() = default;
    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d) {}

    Vector3 GetPoint(float t) const { return origin + direction * t; }
};

/// @brief 平面（法線 + 距離）
struct Plane {
    Vector3 normal;
    float distance = 0.0f;
    Plane() = default;
    Plane(const Vector3& n, float d) : normal(n), distance(d) {}
    Plane(const Vector3& n, const Vector3& point) : normal(n), distance(n.Dot(point)) {}

    float DistanceToPoint(const Vector3& point) const
    {
        return normal.Dot(point) - distance;
    }
};

/// @brief 視錐台（6平面）
struct Frustum {
    Plane planes[6]; // 近面/遠面/左/右/上/下
    static Frustum FromViewProjection(const XMMATRIX& viewProj);
};

/// @brief 有向境界ボックス（OBB）
struct OBB {
    Vector3 center;
    Vector3 halfExtents;
    Vector3 axes[3];
    OBB() = default;
    OBB(const Vector3& center, const Vector3& halfExtents, const Matrix4x4& rotation);
};

/// @brief 3D三角形
struct Triangle {
    Vector3 v0, v1, v2;
    Triangle() = default;
    Triangle(const Vector3& a, const Vector3& b, const Vector3& c) : v0(a), v1(b), v2(c) {}

    Vector3 Normal() const
    {
        return (v1 - v0).Cross(v2 - v0).Normalized();
    }
};

// --- 衝突結果 ---

/// @brief 3D衝突結果
struct HitResult3D {
    bool hit = false;
    Vector3 point;
    Vector3 normal;
    float depth = 0.0f;
    float t = 0.0f;
    operator bool() const { return hit; }
};

// --- 衝突判定関数 ---

/// @brief 3D衝突判定ユーティリティ
namespace Collision3D {

    // 判定のみ（ヒット/非ヒット）
    bool TestSphereVsSphere(const Sphere& a, const Sphere& b);
    bool TestAABBVsAABB(const AABB3D& a, const AABB3D& b);
    bool TestSphereVsAABB(const Sphere& sphere, const AABB3D& aabb);
    bool TestPointInSphere(const Vector3& point, const Sphere& sphere);
    bool TestPointInAABB(const Vector3& point, const AABB3D& aabb);
    bool TestOBBVsOBB(const OBB& a, const OBB& b);

    bool TestFrustumVsSphere(const Frustum& frustum, const Sphere& sphere);
    bool TestFrustumVsAABB(const Frustum& frustum, const AABB3D& aabb);
    bool TestFrustumVsPoint(const Frustum& frustum, const Vector3& point);

    // レイキャスト
    bool RaycastSphere(const Ray& ray, const Sphere& sphere, float& outT);
    bool RaycastAABB(const Ray& ray, const AABB3D& aabb, float& outT);
    bool RaycastTriangle(const Ray& ray, const Triangle& tri, float& outT, float& outU, float& outV);
    bool RaycastPlane(const Ray& ray, const Plane& plane, float& outT);
    bool RaycastOBB(const Ray& ray, const OBB& obb, float& outT);

    // 交差情報付き
    HitResult3D IntersectSphereVsSphere(const Sphere& a, const Sphere& b);
    HitResult3D IntersectSphereVsAABB(const Sphere& sphere, const AABB3D& aabb);

    // スイープ（移動球の当たり判定）
    bool SweepSphereVsSphere(const Sphere& a, const Vector3& velA,
                              const Sphere& b, const Vector3& velB, float& outT);

    // 最近点計算
    Vector3 ClosestPointOnAABB(const Vector3& point, const AABB3D& aabb);
    Vector3 ClosestPointOnTriangle(const Vector3& point, const Triangle& tri);
    Vector3 ClosestPointOnLine(const Vector3& point, const Vector3& lineA, const Vector3& lineB);

} // namespace Collision3D
} // namespace GX
