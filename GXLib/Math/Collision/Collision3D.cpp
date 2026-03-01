#include "pch_common.h"
#include "Math/Collision/Collision3D.h"

namespace gx {

// --- 視錐台 ---

Frustum Frustum::FromViewProjection(const XMMATRIX& viewProj)
{
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, viewProj);

    Frustum f;
    // 左面
    f.planes[2].normal = { m._14 + m._11, m._24 + m._21, m._34 + m._31 };
    f.planes[2].distance = -(m._44 + m._41);
    // 右面
    f.planes[3].normal = { m._14 - m._11, m._24 - m._21, m._34 - m._31 };
    f.planes[3].distance = -(m._44 - m._41);
    // 下面
    f.planes[5].normal = { m._14 + m._12, m._24 + m._22, m._34 + m._32 };
    f.planes[5].distance = -(m._44 + m._42);
    // 上面
    f.planes[4].normal = { m._14 - m._12, m._24 - m._22, m._34 - m._32 };
    f.planes[4].distance = -(m._44 - m._42);
    // 近面
    f.planes[0].normal = { m._13, m._23, m._33 };
    f.planes[0].distance = -m._43;
    // 遠面
    f.planes[1].normal = { m._14 - m._13, m._24 - m._23, m._34 - m._33 };
    f.planes[1].distance = -(m._44 - m._43);

    // すべての平面を正規化
    for (int i = 0; i < 6; ++i)
    {
        float len = f.planes[i].normal.Length();
        if (len > MathUtil::EPSILON)
        {
            f.planes[i].normal = f.planes[i].normal * (1.0f / len);
            f.planes[i].distance /= len;
        }
    }

    return f;
}

// --- OBB ---

OBB::OBB(const Vector3& center, const Vector3& halfExtents, const Matrix4x4& rotation)
    : center(center), halfExtents(halfExtents)
{
    axes[0] = Vector3::TransformNormal(Vector3(1, 0, 0), rotation);
    axes[1] = Vector3::TransformNormal(Vector3(0, 1, 0), rotation);
    axes[2] = Vector3::TransformNormal(Vector3(0, 0, 1), rotation);
}

// --- 3D衝突判定 ---

namespace Collision3D {

bool TestSphereVsSphere(const Sphere& a, const Sphere& b)
{
    float r = a.radius + b.radius;
    return a.center.DistanceSquared(b.center) <= r * r;
}

bool TestAABBVsAABB(const AABB3D& a, const AABB3D& b)
{
    return a.max.x >= b.min.x && a.min.x <= b.max.x &&
           a.max.y >= b.min.y && a.min.y <= b.max.y &&
           a.max.z >= b.min.z && a.min.z <= b.max.z;
}

bool TestSphereVsAABB(const Sphere& sphere, const AABB3D& aabb)
{
    Vector3 closest = ClosestPointOnAABB(sphere.center, aabb);
    return sphere.center.DistanceSquared(closest) <= sphere.radius * sphere.radius;
}

bool TestPointInSphere(const Vector3& point, const Sphere& sphere)
{
    return sphere.Contains(point);
}

bool TestPointInAABB(const Vector3& point, const AABB3D& aabb)
{
    return aabb.Contains(point);
}

bool TestOBBVsOBB(const OBB& a, const OBB& b)
{
    // 分離軸定理（SAT）: 2つの箱を分離できる軸が1つでも見つかれば非衝突
    Vector3 d = b.center - a.center;
    float ha[3] = { a.halfExtents.x, a.halfExtents.y, a.halfExtents.z };
    float hb[3] = { b.halfExtents.x, b.halfExtents.y, b.halfExtents.z };

    // 回転行列 R: R[i][j] = dot(a.axes[i], b.axes[j])
    float R[3][3], absR[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        {
            R[i][j] = a.axes[i].Dot(b.axes[j]);
            absR[i][j] = std::abs(R[i][j]) + MathUtil::EPSILON;
        }

    float t[3] = { d.Dot(a.axes[0]), d.Dot(a.axes[1]), d.Dot(a.axes[2]) };

    // Aの3軸で判定
    for (int i = 0; i < 3; ++i)
    {
        float ra = ha[i];
        float rb = hb[0] * absR[i][0] + hb[1] * absR[i][1] + hb[2] * absR[i][2];
        if (std::abs(t[i]) > ra + rb) return false;
    }

    // Bの3軸で判定
    for (int j = 0; j < 3; ++j)
    {
        float ra = ha[0] * absR[0][j] + ha[1] * absR[1][j] + ha[2] * absR[2][j];
        float rb = hb[j];
        float proj = std::abs(t[0] * R[0][j] + t[1] * R[1][j] + t[2] * R[2][j]);
        if (proj > ra + rb) return false;
    }

    // 9本の外積軸で判定
    // a0 x b0
    { float ra = ha[1]*absR[2][0] + ha[2]*absR[1][0]; float rb = hb[1]*absR[0][2] + hb[2]*absR[0][1]; if (std::abs(t[2]*R[1][0] - t[1]*R[2][0]) > ra + rb) return false; }
    // a0 x b1
    { float ra = ha[1]*absR[2][1] + ha[2]*absR[1][1]; float rb = hb[0]*absR[0][2] + hb[2]*absR[0][0]; if (std::abs(t[2]*R[1][1] - t[1]*R[2][1]) > ra + rb) return false; }
    // a0 x b2
    { float ra = ha[1]*absR[2][2] + ha[2]*absR[1][2]; float rb = hb[0]*absR[0][1] + hb[1]*absR[0][0]; if (std::abs(t[2]*R[1][2] - t[1]*R[2][2]) > ra + rb) return false; }
    // a1 x b0
    { float ra = ha[0]*absR[2][0] + ha[2]*absR[0][0]; float rb = hb[1]*absR[1][2] + hb[2]*absR[1][1]; if (std::abs(t[0]*R[2][0] - t[2]*R[0][0]) > ra + rb) return false; }
    // a1 x b1
    { float ra = ha[0]*absR[2][1] + ha[2]*absR[0][1]; float rb = hb[0]*absR[1][2] + hb[2]*absR[1][0]; if (std::abs(t[0]*R[2][1] - t[2]*R[0][1]) > ra + rb) return false; }
    // a1 x b2
    { float ra = ha[0]*absR[2][2] + ha[2]*absR[0][2]; float rb = hb[0]*absR[1][1] + hb[1]*absR[1][0]; if (std::abs(t[0]*R[2][2] - t[2]*R[0][2]) > ra + rb) return false; }
    // a2 x b0
    { float ra = ha[0]*absR[1][0] + ha[1]*absR[0][0]; float rb = hb[1]*absR[2][2] + hb[2]*absR[2][1]; if (std::abs(t[1]*R[0][0] - t[0]*R[1][0]) > ra + rb) return false; }
    // a2 x b1
    { float ra = ha[0]*absR[1][1] + ha[1]*absR[0][1]; float rb = hb[0]*absR[2][2] + hb[2]*absR[2][0]; if (std::abs(t[1]*R[0][1] - t[0]*R[1][1]) > ra + rb) return false; }
    // a2 x b2
    { float ra = ha[0]*absR[1][2] + ha[1]*absR[0][2]; float rb = hb[0]*absR[2][1] + hb[1]*absR[2][0]; if (std::abs(t[1]*R[0][2] - t[0]*R[1][2]) > ra + rb) return false; }

    return true;
}

bool TestFrustumVsSphere(const Frustum& frustum, const Sphere& sphere)
{
    for (int i = 0; i < 6; ++i)
    {
        if (frustum.planes[i].DistanceToPoint(sphere.center) < -sphere.radius)
            return false;
    }
    return true;
}

bool TestFrustumVsAABB(const Frustum& frustum, const AABB3D& aabb)
{
    for (int i = 0; i < 6; ++i)
    {
        const Plane& p = frustum.planes[i];
        // 平面法線方向に最も遠い頂点を選ぶ
        Vector3 pv;
        pv.x = (p.normal.x >= 0.0f) ? aabb.max.x : aabb.min.x;
        pv.y = (p.normal.y >= 0.0f) ? aabb.max.y : aabb.min.y;
        pv.z = (p.normal.z >= 0.0f) ? aabb.max.z : aabb.min.z;

        if (p.DistanceToPoint(pv) < 0.0f) return false;
    }
    return true;
}

bool TestFrustumVsPoint(const Frustum& frustum, const Vector3& point)
{
    for (int i = 0; i < 6; ++i)
    {
        if (frustum.planes[i].DistanceToPoint(point) < 0.0f)
            return false;
    }
    return true;
}

bool RaycastSphere(const Ray& ray, const Sphere& sphere, float& outT)
{
    Vector3 oc = ray.origin - sphere.center;
    float a = ray.direction.Dot(ray.direction);
    float b = 2.0f * oc.Dot(ray.direction);
    float c = oc.Dot(oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) return false;

    float sqrtD = std::sqrt(discriminant);
    float t = (-b - sqrtD) / (2.0f * a);
    if (t < 0.0f)
    {
        t = (-b + sqrtD) / (2.0f * a);
        if (t < 0.0f) return false;
    }

    outT = t;
    return true;
}

bool RaycastAABB(const Ray& ray, const AABB3D& aabb, float& outT)
{
    // スラブ法（各軸で交差区間を絞り込む）
    float tmin = 0.0f, tmax = 1e30f;
    float orig[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
    float dir[3]  = { ray.direction.x, ray.direction.y, ray.direction.z };
    float lo[3]   = { aabb.min.x, aabb.min.y, aabb.min.z };
    float hi[3]   = { aabb.max.x, aabb.max.y, aabb.max.z };

    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(dir[i]) < MathUtil::EPSILON)
        {
            if (orig[i] < lo[i] || orig[i] > hi[i]) return false;
        }
        else
        {
            float inv = 1.0f / dir[i];
            float t1 = (lo[i] - orig[i]) * inv;
            float t2 = (hi[i] - orig[i]) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) return false;
        }
    }

    if (tmin < 0.0f) return false;
    outT = tmin;
    return true;
}

bool RaycastTriangle(const Ray& ray, const Triangle& tri, float& outT, float& outU, float& outV)
{
    // Moller-Trumbore法: 三角形の2辺と面内座標(u,v)でレイの交差を判定する
    Vector3 edge1 = tri.v1 - tri.v0;
    Vector3 edge2 = tri.v2 - tri.v0;
    Vector3 h = ray.direction.Cross(edge2);
    float a = edge1.Dot(h);

    if (std::abs(a) < MathUtil::EPSILON) return false;

    float f = 1.0f / a;
    Vector3 s = ray.origin - tri.v0;
    float u = f * s.Dot(h);

    if (u < 0.0f || u > 1.0f) return false;

    Vector3 q = s.Cross(edge1);
    float v = f * ray.direction.Dot(q);

    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * edge2.Dot(q);
    if (t < 0.0f) return false;

    outT = t;
    outU = u;
    outV = v;
    return true;
}

bool RaycastPlane(const Ray& ray, const Plane& plane, float& outT)
{
    float denom = plane.normal.Dot(ray.direction);
    if (std::abs(denom) < MathUtil::EPSILON) return false;

    float t = (plane.distance - plane.normal.Dot(ray.origin)) / denom;
    if (t < 0.0f) return false;

    outT = t;
    return true;
}

bool RaycastOBB(const Ray& ray, const OBB& obb, float& outT)
{
    // レイをOBBのローカル空間に変換し、AABBとしてレイキャストする
    Vector3 d = ray.origin - obb.center;
    Vector3 localOrigin(d.Dot(obb.axes[0]), d.Dot(obb.axes[1]), d.Dot(obb.axes[2]));
    Vector3 localDir(ray.direction.Dot(obb.axes[0]), ray.direction.Dot(obb.axes[1]), ray.direction.Dot(obb.axes[2]));

    AABB3D localAABB(-obb.halfExtents, obb.halfExtents);
    Ray localRay(localOrigin, localDir);
    return RaycastAABB(localRay, localAABB, outT);
}

HitResult3D IntersectSphereVsSphere(const Sphere& a, const Sphere& b)
{
    HitResult3D result;
    Vector3 diff = b.center - a.center;
    float distSq = diff.LengthSquared();
    float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum) return result;

    float dist = std::sqrt(distSq);
    result.hit = true;
    result.depth = radiusSum - dist;

    if (dist > MathUtil::EPSILON)
        result.normal = diff * (1.0f / dist);
    else
        result.normal = Vector3(1, 0, 0);

    result.point = a.center + result.normal * a.radius;
    return result;
}

HitResult3D IntersectSphereVsAABB(const Sphere& sphere, const AABB3D& aabb)
{
    HitResult3D result;
    Vector3 closest = ClosestPointOnAABB(sphere.center, aabb);
    Vector3 diff = sphere.center - closest;
    float distSq = diff.LengthSquared();

    if (distSq > sphere.radius * sphere.radius) return result;

    result.hit = true;
    float dist = std::sqrt(distSq);

    if (dist > MathUtil::EPSILON)
    {
        result.normal = diff * (1.0f / dist);
        result.depth = sphere.radius - dist;
    }
    else
    {
        // 球の中心がAABB内部にある場合
        Vector3 center = sphere.center;
        float dists[6] = {
            center.x - aabb.min.x, aabb.max.x - center.x,
            center.y - aabb.min.y, aabb.max.y - center.y,
            center.z - aabb.min.z, aabb.max.z - center.z
        };
        int minIdx = 0;
        for (int i = 1; i < 6; ++i)
            if (dists[i] < dists[minIdx]) minIdx = i;

        Vector3 normals[6] = {
            {-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}
        };
        result.normal = normals[minIdx];
        result.depth = sphere.radius + dists[minIdx];
    }
    result.point = closest;
    return result;
}

bool SweepSphereVsSphere(const Sphere& a, const Vector3& velA,
                          const Sphere& b, const Vector3& velB, float& outT)
{
    Vector3 relVel = velA - velB;
    Sphere expanded(b.center, a.radius + b.radius);
    Ray ray(a.center, relVel);
    return RaycastSphere(ray, expanded, outT);
}

Vector3 ClosestPointOnAABB(const Vector3& point, const AABB3D& aabb)
{
    return {
        MathUtil::Clamp(point.x, aabb.min.x, aabb.max.x),
        MathUtil::Clamp(point.y, aabb.min.y, aabb.max.y),
        MathUtil::Clamp(point.z, aabb.min.z, aabb.max.z)
    };
}

Vector3 ClosestPointOnTriangle(const Vector3& point, const Triangle& tri)
{
    // 重心座標法で最近点を計算
    Vector3 ab = tri.v1 - tri.v0;
    Vector3 ac = tri.v2 - tri.v0;
    Vector3 ap = point - tri.v0;

    float d1 = ab.Dot(ap);
    float d2 = ac.Dot(ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return tri.v0;

    Vector3 bp = point - tri.v1;
    float d3 = ab.Dot(bp);
    float d4 = ac.Dot(bp);
    if (d3 >= 0.0f && d4 <= d3) return tri.v1;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return tri.v0 + ab * v;
    }

    Vector3 cp = point - tri.v2;
    float d5 = ab.Dot(cp);
    float d6 = ac.Dot(cp);
    if (d6 >= 0.0f && d5 <= d6) return tri.v2;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return tri.v0 + ac * w;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return tri.v1 + (tri.v2 - tri.v1) * w;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return tri.v0 + ab * v + ac * w;
}

Vector3 ClosestPointOnLine(const Vector3& point, const Vector3& lineA, const Vector3& lineB)
{
    Vector3 ab = lineB - lineA;
    float denom = ab.Dot(ab);
    if (denom < 1e-6f) return lineA;
    float t = (point - lineA).Dot(ab) / denom;
    t = MathUtil::Clamp(t, 0.0f, 1.0f);
    return lineA + ab * t;
}

// --- カプセル判定 ---

/// 2線分間の最短距離の二乗を返すヘルパー
static float ClosestSegmentSegmentDistSq(const Vector3& p1, const Vector3& q1,
                                          const Vector3& p2, const Vector3& q2,
                                          float& s, float& t)
{
    Vector3 d1 = q1 - p1;
    Vector3 d2 = q2 - p2;
    Vector3 r = p1 - p2;

    float a = d1.Dot(d1);
    float e = d2.Dot(d2);
    float f = d2.Dot(r);

    if (a <= 1e-6f && e <= 1e-6f) { s = t = 0.0f; return r.LengthSquared(); }
    if (a <= 1e-6f) { s = 0.0f; t = MathUtil::Clamp(f / e, 0.0f, 1.0f); }
    else
    {
        float c = d1.Dot(r);
        if (e <= 1e-6f) { t = 0.0f; s = MathUtil::Clamp(-c / a, 0.0f, 1.0f); }
        else
        {
            float b = d1.Dot(d2);
            float denom = a * e - b * b;
            s = (denom != 0.0f) ? MathUtil::Clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
            t = (b * s + f) / e;
            if (t < 0.0f) { t = 0.0f; s = MathUtil::Clamp(-c / a, 0.0f, 1.0f); }
            else if (t > 1.0f) { t = 1.0f; s = MathUtil::Clamp((b - c) / a, 0.0f, 1.0f); }
        }
    }
    Vector3 c1 = p1 + d1 * s;
    Vector3 c2 = p2 + d2 * t;
    return (c1 - c2).LengthSquared();
}

bool TestCapsuleVsCapsule(const Capsule& a, const Capsule& b)
{
    float s, t;
    float distSq = ClosestSegmentSegmentDistSq(a.start, a.end, b.start, b.end, s, t);
    float r = a.radius + b.radius;
    return distSq <= r * r;
}

bool TestCapsuleVsSphere(const Capsule& capsule, const Sphere& sphere)
{
    Vector3 closest = ClosestPointOnLine(sphere.center, capsule.start, capsule.end);
    float r = capsule.radius + sphere.radius;
    return closest.DistanceSquared(sphere.center) <= r * r;
}

bool TestCapsuleVsAABB(const Capsule& capsule, const AABB3D& aabb)
{
    // Approximate: expand AABB by capsule radius and test segment vs AABB
    AABB3D expanded = aabb.Expand(capsule.radius);
    // Check if any point on the segment is in the expanded AABB
    const int steps = 16;
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        Vector3 p = capsule.start + (capsule.end - capsule.start) * t;
        if (expanded.Contains(p)) return true;
    }
    // Also check closest point on AABB to segment endpoints
    Vector3 closestToStart = ClosestPointOnAABB(capsule.start, aabb);
    if (capsule.start.DistanceSquared(closestToStart) <= capsule.radius * capsule.radius) return true;
    Vector3 closestToEnd = ClosestPointOnAABB(capsule.end, aabb);
    if (capsule.end.DistanceSquared(closestToEnd) <= capsule.radius * capsule.radius) return true;
    return false;
}

bool RaycastCapsule(const Ray& ray, const Capsule& capsule, float& outT)
{
    // Raycast against the infinite cylinder, then check against the two end caps
    Vector3 d = capsule.end - capsule.start;
    Vector3 m = ray.origin - capsule.start;
    float dd = d.Dot(d);
    float md = m.Dot(d);
    float nd = ray.direction.Dot(d);
    float mm = m.Dot(m);
    float mn = m.Dot(ray.direction);
    float nn = ray.direction.Dot(ray.direction);
    float r = capsule.radius;

    float a = dd * nn - nd * nd;
    float b = dd * mn - nd * md;
    float c = dd * (mm - r * r) - md * md;

    // Try the cylinder
    float bestT = 1e30f;
    bool hit = false;
    if (std::abs(a) > 1e-6f)
    {
        float disc = b * b - a * c;
        if (disc >= 0.0f)
        {
            float sqrtDisc = std::sqrtf(disc);
            float t1 = (-b - sqrtDisc) / a;
            float t2 = (-b + sqrtDisc) / a;
            for (float t : {t1, t2})
            {
                if (t >= 0.0f)
                {
                    float along = md + t * nd;
                    if (along >= 0.0f && along <= dd && t < bestT)
                    {
                        bestT = t;
                        hit = true;
                    }
                }
            }
        }
    }
    // Try the two end caps (spheres)
    Sphere capStart(capsule.start, capsule.radius);
    Sphere capEnd(capsule.end, capsule.radius);
    float ts, te;
    if (RaycastSphere(ray, capStart, ts) && ts < bestT) { bestT = ts; hit = true; }
    if (RaycastSphere(ray, capEnd, te) && te < bestT) { bestT = te; hit = true; }

    if (hit) outT = bestT;
    return hit;
}

HitResult3D IntersectCapsuleVsSphere(const Capsule& capsule, const Sphere& sphere)
{
    HitResult3D result;
    Vector3 closestOnSeg = ClosestPointOnLine(sphere.center, capsule.start, capsule.end);
    Vector3 diff = sphere.center - closestOnSeg;
    float dist = diff.Length();
    float combinedR = capsule.radius + sphere.radius;

    if (dist > combinedR) return result;

    result.hit = true;
    result.depth = combinedR - dist;
    if (dist > 1e-6f)
        result.normal = diff * (1.0f / dist);
    else
        result.normal = Vector3(0, 1, 0);
    result.point = closestOnSeg + result.normal * capsule.radius;
    return result;
}

Vector3 ClosestPointOnCapsule(const Vector3& point, const Capsule& capsule)
{
    Vector3 closestOnSeg = ClosestPointOnLine(point, capsule.start, capsule.end);
    Vector3 diff = point - closestOnSeg;
    float dist = diff.Length();
    if (dist < 1e-6f) return closestOnSeg + Vector3(0, capsule.radius, 0);
    return closestOnSeg + diff * (capsule.radius / dist);
}

// --- OBB vs OBB MTV ---

HitResult3D IntersectOBBVsOBB(const OBB& a, const OBB& b)
{
    HitResult3D result;
    Vector3 d = b.center - a.center;
    float ha[3] = { a.halfExtents.x, a.halfExtents.y, a.halfExtents.z };
    float hb[3] = { b.halfExtents.x, b.halfExtents.y, b.halfExtents.z };

    float R[3][3], absR[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        {
            R[i][j] = a.axes[i].Dot(b.axes[j]);
            absR[i][j] = std::abs(R[i][j]) + 1e-6f;
        }

    float t[3] = { d.Dot(a.axes[0]), d.Dot(a.axes[1]), d.Dot(a.axes[2]) };

    float minOverlap = 1e30f;
    Vector3 minAxis;

    // Test A's axes
    for (int i = 0; i < 3; ++i)
    {
        float ra = ha[i];
        float rb = hb[0] * absR[i][0] + hb[1] * absR[i][1] + hb[2] * absR[i][2];
        float overlap = (ra + rb) - std::abs(t[i]);
        if (overlap < 0.0f) return result;
        if (overlap < minOverlap) { minOverlap = overlap; minAxis = a.axes[i]; if (t[i] < 0) minAxis = minAxis * -1.0f; }
    }

    // Test B's axes
    for (int j = 0; j < 3; ++j)
    {
        float ra = ha[0] * absR[0][j] + ha[1] * absR[1][j] + ha[2] * absR[2][j];
        float rb = hb[j];
        float proj = t[0] * R[0][j] + t[1] * R[1][j] + t[2] * R[2][j];
        float overlap = (ra + rb) - std::abs(proj);
        if (overlap < 0.0f) return result;
        if (overlap < minOverlap) { minOverlap = overlap; minAxis = b.axes[j]; if (proj < 0) minAxis = minAxis * -1.0f; }
    }

    result.hit = true;
    result.depth = minOverlap;
    result.normal = minAxis;
    result.point = (a.center + b.center) * 0.5f;
    return result;
}

// --- GJK/EPA ---

bool TestGJK(const SupportFunc& supportA, const SupportFunc& supportB)
{
    auto support = [&](const Vector3& dir) -> Vector3 {
        return supportA(dir) - supportB(dir * -1.0f);
    };

    Vector3 dir(1, 0.5f, 0.25f); // non-axis-aligned initial direction
    Vector3 simplex[4];
    int simplexSize = 0;

    simplex[0] = support(dir);
    simplexSize = 1;
    dir = simplex[0] * -1.0f;

    for (int iter = 0; iter < 64; ++iter)
    {
        Vector3 a = support(dir);
        if (a.Dot(dir) < 0.0f) return false;

        simplex[simplexSize++] = a;

        if (simplexSize == 2)
        {
            // Line case
            Vector3 ab = simplex[0] - simplex[1];
            Vector3 ao = simplex[1] * -1.0f;
            dir = ab.Cross(ao).Cross(ab);
            if (dir.LengthSquared() < 1e-10f) dir = Vector3(1, 0.5f, 0.25f);
        }
        else if (simplexSize == 3)
        {
            // Triangle case
            Vector3 a0 = simplex[2] * -1.0f;
            Vector3 ab = simplex[1] - simplex[2];
            Vector3 ac = simplex[0] - simplex[2];
            Vector3 normal = ab.Cross(ac);

            if (normal.Dot(a0) > 0)
                dir = normal;
            else
                dir = normal * -1.0f;
        }
        else if (simplexSize == 4)
        {
            // Tetrahedron case
            Vector3 a0 = simplex[3] * -1.0f;
            Vector3 ab = simplex[2] - simplex[3];
            Vector3 ac = simplex[1] - simplex[3];
            Vector3 ad = simplex[0] - simplex[3];

            Vector3 abc = ab.Cross(ac);
            Vector3 acd = ac.Cross(ad);
            Vector3 adb = ad.Cross(ab);

            if (abc.Dot(a0) > 0) { simplex[0] = simplex[1]; simplex[1] = simplex[2]; simplex[2] = simplex[3]; simplexSize = 3; dir = abc; }
            else if (acd.Dot(a0) > 0) { simplex[0] = simplex[0]; simplex[1] = simplex[1]; simplex[2] = simplex[3]; simplexSize = 3; dir = acd; }
            else if (adb.Dot(a0) > 0) { simplex[1] = simplex[0]; simplex[0] = simplex[2]; simplex[2] = simplex[3]; simplexSize = 3; dir = adb; }
            else return true; // origin inside tetrahedron
        }
    }
    return false;
}

HitResult3D IntersectGJKEPA(const SupportFunc& supportA, const SupportFunc& supportB)
{
    HitResult3D result;
    if (!TestGJK(supportA, supportB)) return result;
    // Simplified: use GJK to detect overlap, approximate depth using support function
    result.hit = true;

    // Sample multiple directions to find minimum penetration
    Vector3 dirs[] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1},
        Vector3(1,1,0).Normalized(), Vector3(-1,1,0).Normalized(),
        Vector3(0,1,1).Normalized(), Vector3(0,-1,1).Normalized(),
        Vector3(1,0,1).Normalized(), Vector3(-1,0,-1).Normalized(),
    };

    float minDepth = 1e30f;
    Vector3 minNormal(1, 0, 0);
    for (const auto& dir : dirs)
    {
        Vector3 sa = supportA(dir);
        Vector3 sb = supportB(dir * -1.0f);
        float depth = dir.Dot(sa - sb);
        if (depth < minDepth) { minDepth = depth; minNormal = dir; }
    }

    result.depth = (std::max)(0.0f, minDepth);
    result.normal = minNormal;
    return result;
}

} // namespace Collision3D
} // namespace gx
