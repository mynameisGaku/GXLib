#pragma once
#include "Math/Vector2.h"

namespace GX {

// --- 2D形状定義 ---

/// @brief 2D軸平行境界ボックス（AABB）
struct AABB2D {
    Vector2 min, max;
    AABB2D() = default;
    AABB2D(const Vector2& min, const Vector2& max) : min(min), max(max) {}
    AABB2D(float x, float y, float w, float h) : min(x, y), max(x + w, y + h) {}

    Vector2 Center() const { return (min + max) * 0.5f; }
    Vector2 Size() const { return max - min; }
    Vector2 HalfSize() const { return (max - min) * 0.5f; }

    bool Contains(const Vector2& point) const
    {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }

    AABB2D Expand(float margin) const
    {
        return { { min.x - margin, min.y - margin },
                 { max.x + margin, max.y + margin } };
    }

    AABB2D Merged(const AABB2D& other) const
    {
        return { Vector2::Min(min, other.min), Vector2::Max(max, other.max) };
    }

    float Area() const
    {
        Vector2 s = Size();
        return s.x * s.y;
    }
};

/// @brief 2D円
struct Circle {
    Vector2 center;
    float radius = 0.0f;
    Circle() = default;
    Circle(const Vector2& c, float r) : center(c), radius(r) {}
    Circle(float x, float y, float r) : center(x, y), radius(r) {}

    bool Contains(const Vector2& point) const
    {
        return center.DistanceSquared(point) <= radius * radius;
    }
};

/// @brief 2D線分
struct Line2D {
    Vector2 start, end;
    Line2D() = default;
    Line2D(const Vector2& s, const Vector2& e) : start(s), end(e) {}

    float Length() const { return start.Distance(end); }

    Vector2 Direction() const { return (end - start).Normalized(); }

    Vector2 ClosestPoint(const Vector2& point) const
    {
        Vector2 ab = end - start;
        float t = (point - start).Dot(ab) / ab.Dot(ab);
        t = MathUtil::Clamp(t, 0.0f, 1.0f);
        return start + ab * t;
    }
};

/// @brief 2D多角形（頂点列）
struct Polygon2D {
    std::vector<Vector2> vertices;

    bool Contains(const Vector2& point) const;
    AABB2D GetBounds() const;
};

// --- 衝突結果 ---

/// @brief 2D衝突結果
struct HitResult2D {
    bool hit = false;
    Vector2 point;
    Vector2 normal;
    float depth = 0.0f;
    operator bool() const { return hit; }
};

// --- 衝突判定関数 ---

/// @brief 2D衝突判定ユーティリティ
namespace Collision2D {

    // 判定のみ（ヒット/非ヒット）
    bool TestAABBvsAABB(const AABB2D& a, const AABB2D& b);
    bool TestCirclevsCircle(const Circle& a, const Circle& b);
    bool TestAABBvsCircle(const AABB2D& aabb, const Circle& circle);
    bool TestPointInAABB(const Vector2& point, const AABB2D& aabb);
    bool TestPointInCircle(const Vector2& point, const Circle& circle);
    bool TestPointInPolygon(const Vector2& point, const Polygon2D& polygon);
    bool TestLinevsAABB(const Line2D& line, const AABB2D& aabb);
    bool TestLinevsCircle(const Line2D& line, const Circle& circle);
    bool TestLinevsLine(const Line2D& a, const Line2D& b, Vector2* outPoint = nullptr);

    // 交差情報付き（法線・貫通深度など）
    HitResult2D IntersectAABBvsAABB(const AABB2D& a, const AABB2D& b);
    HitResult2D IntersectCirclevsCircle(const Circle& a, const Circle& b);
    HitResult2D IntersectAABBvsCircle(const AABB2D& aabb, const Circle& circle);

    // レイキャスト（線分の当たり判定）
    bool Raycast2D(const Vector2& origin, const Vector2& direction, const AABB2D& aabb,
                   float& outT, Vector2* outNormal = nullptr);
    bool Raycast2D(const Vector2& origin, const Vector2& direction, const Circle& circle,
                   float& outT);

    // スイープ（移動円の当たり判定）
    bool SweepCirclevsCircle(const Circle& a, const Vector2& velA,
                              const Circle& b, const Vector2& velB,
                              float& outT);

} // namespace Collision2D
} // namespace GX
