#include "pch.h"
#include "Math/Collision/Collision2D.h"

namespace GX {

// --- Polygon2D ---

bool Polygon2D::Contains(const Vector2& point) const
{
    // 巻き数アルゴリズム: 点の周りを辺が何回回り込むかで内外を判定する
    int winding = 0;
    size_t n = vertices.size();
    for (size_t i = 0; i < n; ++i)
    {
        const Vector2& v1 = vertices[i];
        const Vector2& v2 = vertices[(i + 1) % n];
        if (v1.y <= point.y)
        {
            if (v2.y > point.y)
            {
                float cross = (v2.x - v1.x) * (point.y - v1.y) - (point.x - v1.x) * (v2.y - v1.y);
                if (cross > 0) ++winding;
            }
        }
        else
        {
            if (v2.y <= point.y)
            {
                float cross = (v2.x - v1.x) * (point.y - v1.y) - (point.x - v1.x) * (v2.y - v1.y);
                if (cross < 0) --winding;
            }
        }
    }
    return winding != 0;
}

AABB2D Polygon2D::GetBounds() const
{
    if (vertices.empty()) return {};
    Vector2 lo = vertices[0], hi = vertices[0];
    for (size_t i = 1; i < vertices.size(); ++i)
    {
        lo = Vector2::Min(lo, vertices[i]);
        hi = Vector2::Max(hi, vertices[i]);
    }
    return { lo, hi };
}

// --- 2D衝突判定 ---

namespace Collision2D {

bool TestAABBvsAABB(const AABB2D& a, const AABB2D& b)
{
    return a.max.x >= b.min.x && a.min.x <= b.max.x &&
           a.max.y >= b.min.y && a.min.y <= b.max.y;
}

bool TestCirclevsCircle(const Circle& a, const Circle& b)
{
    float r = a.radius + b.radius;
    return a.center.DistanceSquared(b.center) <= r * r;
}

bool TestAABBvsCircle(const AABB2D& aabb, const Circle& circle)
{
    // 円中心に最も近いAABB上の点
    float cx = MathUtil::Clamp(circle.center.x, aabb.min.x, aabb.max.x);
    float cy = MathUtil::Clamp(circle.center.y, aabb.min.y, aabb.max.y);
    float dx = circle.center.x - cx;
    float dy = circle.center.y - cy;
    return (dx * dx + dy * dy) <= circle.radius * circle.radius;
}

bool TestPointInAABB(const Vector2& point, const AABB2D& aabb)
{
    return aabb.Contains(point);
}

bool TestPointInCircle(const Vector2& point, const Circle& circle)
{
    return circle.Contains(point);
}

bool TestPointInPolygon(const Vector2& point, const Polygon2D& polygon)
{
    return polygon.Contains(point);
}

bool TestLinevsAABB(const Line2D& line, const AABB2D& aabb)
{
    // スラブ法: 各軸ごとに交差区間[tmin,tmax]を絞り込んでいく
    Vector2 d = line.end - line.start;
    float tmin = 0.0f, tmax = 1.0f;

    for (int axis = 0; axis < 2; ++axis)
    {
        float orig = (axis == 0) ? line.start.x : line.start.y;
        float dir  = (axis == 0) ? d.x : d.y;
        float lo   = (axis == 0) ? aabb.min.x : aabb.min.y;
        float hi   = (axis == 0) ? aabb.max.x : aabb.max.y;

        if (std::abs(dir) < MathUtil::EPSILON)
        {
            if (orig < lo || orig > hi) return false;
        }
        else
        {
            float inv = 1.0f / dir;
            float t1 = (lo - orig) * inv;
            float t2 = (hi - orig) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    return true;
}

bool TestLinevsCircle(const Line2D& line, const Circle& circle)
{
    Vector2 closest = line.ClosestPoint(circle.center);
    return closest.DistanceSquared(circle.center) <= circle.radius * circle.radius;
}

bool TestLinevsLine(const Line2D& a, const Line2D& b, Vector2* outPoint)
{
    Vector2 d1 = a.end - a.start;
    Vector2 d2 = b.end - b.start;
    float cross = d1.Cross(d2);

    if (std::abs(cross) < MathUtil::EPSILON) return false; // 平行

    Vector2 d = b.start - a.start;
    float t = d.Cross(d2) / cross;
    float u = d.Cross(d1) / cross;

    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
    {
        if (outPoint) *outPoint = a.start + d1 * t;
        return true;
    }
    return false;
}

HitResult2D IntersectAABBvsAABB(const AABB2D& a, const AABB2D& b)
{
    HitResult2D result;

    float overlapX1 = a.max.x - b.min.x;
    float overlapX2 = b.max.x - a.min.x;
    float overlapY1 = a.max.y - b.min.y;
    float overlapY2 = b.max.y - a.min.y;

    if (overlapX1 <= 0.0f || overlapX2 <= 0.0f ||
        overlapY1 <= 0.0f || overlapY2 <= 0.0f)
        return result;

    float minOverlapX = (std::min)(overlapX1, overlapX2);
    float minOverlapY = (std::min)(overlapY1, overlapY2);

    result.hit = true;
    if (minOverlapX < minOverlapY)
    {
        result.depth = minOverlapX;
        result.normal = (overlapX1 < overlapX2) ? Vector2(-1.0f, 0.0f) : Vector2(1.0f, 0.0f);
    }
    else
    {
        result.depth = minOverlapY;
        result.normal = (overlapY1 < overlapY2) ? Vector2(0.0f, -1.0f) : Vector2(0.0f, 1.0f);
    }
    result.point = (a.Center() + b.Center()) * 0.5f;
    return result;
}

HitResult2D IntersectCirclevsCircle(const Circle& a, const Circle& b)
{
    HitResult2D result;
    Vector2 diff = b.center - a.center;
    float distSq = diff.LengthSquared();
    float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum) return result;

    float dist = std::sqrt(distSq);
    result.hit = true;
    result.depth = radiusSum - dist;

    if (dist > MathUtil::EPSILON)
    {
        result.normal = diff * (1.0f / dist);
    }
    else
    {
        result.normal = Vector2(1.0f, 0.0f);
    }
    result.point = a.center + result.normal * a.radius;
    return result;
}

HitResult2D IntersectAABBvsCircle(const AABB2D& aabb, const Circle& circle)
{
    HitResult2D result;

    float cx = MathUtil::Clamp(circle.center.x, aabb.min.x, aabb.max.x);
    float cy = MathUtil::Clamp(circle.center.y, aabb.min.y, aabb.max.y);
    Vector2 closest(cx, cy);

    Vector2 diff = circle.center - closest;
    float distSq = diff.LengthSquared();

    if (distSq > circle.radius * circle.radius) return result;

    result.hit = true;
    float dist = std::sqrt(distSq);

    if (dist > MathUtil::EPSILON)
    {
        result.normal = diff * (1.0f / dist);
        result.depth = circle.radius - dist;
    }
    else
    {
        // 円の中心がAABB内部にめり込んでいる場合、最も浅い方向へ押し出す
        float dLeft   = circle.center.x - aabb.min.x;
        float dRight  = aabb.max.x - circle.center.x;
        float dBottom = circle.center.y - aabb.min.y;
        float dTop    = aabb.max.y - circle.center.y;
        float minD = (std::min)({ dLeft, dRight, dBottom, dTop });

        if (minD == dLeft)       result.normal = Vector2(-1.0f, 0.0f);
        else if (minD == dRight) result.normal = Vector2(1.0f, 0.0f);
        else if (minD == dBottom)result.normal = Vector2(0.0f, -1.0f);
        else                     result.normal = Vector2(0.0f, 1.0f);
        result.depth = circle.radius + minD;
    }
    result.point = closest;
    return result;
}

bool Raycast2D(const Vector2& origin, const Vector2& direction, const AABB2D& aabb,
               float& outT, Vector2* outNormal)
{
    float tmin = 0.0f, tmax = 1e30f;
    Vector2 normal;

    for (int axis = 0; axis < 2; ++axis)
    {
        float orig = (axis == 0) ? origin.x : origin.y;
        float dir  = (axis == 0) ? direction.x : direction.y;
        float lo   = (axis == 0) ? aabb.min.x : aabb.min.y;
        float hi   = (axis == 0) ? aabb.max.x : aabb.max.y;

        if (std::abs(dir) < MathUtil::EPSILON)
        {
            if (orig < lo || orig > hi) return false;
        }
        else
        {
            float inv = 1.0f / dir;
            float t1 = (lo - orig) * inv;
            float t2 = (hi - orig) * inv;
            Vector2 n1 = (axis == 0) ? Vector2(-1.0f, 0.0f) : Vector2(0.0f, -1.0f);
            Vector2 n2 = (axis == 0) ? Vector2(1.0f, 0.0f) : Vector2(0.0f, 1.0f);

            if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
            if (t1 > tmin) { tmin = t1; normal = n1; }
            if (t2 < tmax) { tmax = t2; }
            if (tmin > tmax) return false;
        }
    }

    if (tmin < 0.0f) return false;
    outT = tmin;
    if (outNormal) *outNormal = normal;
    return true;
}

bool Raycast2D(const Vector2& origin, const Vector2& direction, const Circle& circle,
               float& outT)
{
    Vector2 oc = origin - circle.center;
    float a = direction.Dot(direction);
    float b = 2.0f * oc.Dot(direction);
    float c = oc.Dot(oc) - circle.radius * circle.radius;
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

bool SweepCirclevsCircle(const Circle& a, const Vector2& velA,
                          const Circle& b, const Vector2& velB,
                          float& outT)
{
    // bを固定してaの相対速度でレイキャストする（ミンコフスキー和）
    Vector2 relVel = velA - velB;
    Circle expanded(b.center, a.radius + b.radius);

    return Raycast2D(a.center, relVel, expanded, outT);
}

} // namespace Collision2D
} // namespace GX
