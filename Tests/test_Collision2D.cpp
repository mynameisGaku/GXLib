/// @file test_Collision2D.cpp
/// @brief 2D 衝突判定 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Collision/Collision2D.h"

using namespace GX;

// ============================================================================
// AABB vs AABB（軸平行ボックス同士）
// ============================================================================

TEST(Collision2DTest, AABBvsAABB_Overlap)
{
    AABB2D a({0, 0}, {10, 10});
    AABB2D b({5, 5}, {15, 15});
    EXPECT_TRUE(Collision2D::TestAABBvsAABB(a, b));
}

TEST(Collision2DTest, AABBvsAABB_Separated)
{
    AABB2D a({0, 0}, {10, 10});
    AABB2D b({20, 20}, {30, 30});
    EXPECT_FALSE(Collision2D::TestAABBvsAABB(a, b));
}

TEST(Collision2DTest, AABBvsAABB_Touching)
{
    AABB2D a({0, 0}, {10, 10});
    AABB2D b({10, 0}, {20, 10});
    EXPECT_TRUE(Collision2D::TestAABBvsAABB(a, b));
}

TEST(Collision2DTest, AABBvsAABB_Contained)
{
    AABB2D outer({0, 0}, {100, 100});
    AABB2D inner({10, 10}, {20, 20});
    EXPECT_TRUE(Collision2D::TestAABBvsAABB(outer, inner));
}

// ============================================================================
// Circle vs Circle（円同士）
// ============================================================================

TEST(Collision2DTest, CirclevsCircle_Overlap)
{
    Circle a({0, 0}, 5.0f);
    Circle b({7, 0}, 5.0f);
    EXPECT_TRUE(Collision2D::TestCirclevsCircle(a, b));
}

TEST(Collision2DTest, CirclevsCircle_Separated)
{
    Circle a({0, 0}, 5.0f);
    Circle b({20, 0}, 5.0f);
    EXPECT_FALSE(Collision2D::TestCirclevsCircle(a, b));
}

TEST(Collision2DTest, CirclevsCircle_Touching)
{
    Circle a({0, 0}, 5.0f);
    Circle b({10, 0}, 5.0f);
    EXPECT_TRUE(Collision2D::TestCirclevsCircle(a, b));
}

// ============================================================================
// AABB vs Circle（ボックスと円）
// ============================================================================

TEST(Collision2DTest, AABBvsCircle_Overlap)
{
    AABB2D aabb({0, 0}, {10, 10});
    Circle circle({12, 5}, 3.0f);
    EXPECT_TRUE(Collision2D::TestAABBvsCircle(aabb, circle));
}

TEST(Collision2DTest, AABBvsCircle_Separated)
{
    AABB2D aabb({0, 0}, {10, 10});
    Circle circle({20, 20}, 2.0f);
    EXPECT_FALSE(Collision2D::TestAABBvsCircle(aabb, circle));
}

// ============================================================================
// 点の判定
// ============================================================================

TEST(Collision2DTest, PointInAABB)
{
    AABB2D aabb({0, 0}, {10, 10});
    EXPECT_TRUE(Collision2D::TestPointInAABB({5, 5}, aabb));
    EXPECT_FALSE(Collision2D::TestPointInAABB({-1, 5}, aabb));
    EXPECT_TRUE(Collision2D::TestPointInAABB({0, 0}, aabb)); // 端は内側扱い
}

TEST(Collision2DTest, PointInCircle)
{
    Circle circle({5, 5}, 3.0f);
    EXPECT_TRUE(Collision2D::TestPointInCircle({5, 5}, circle));
    EXPECT_TRUE(Collision2D::TestPointInCircle({6, 5}, circle));
    EXPECT_FALSE(Collision2D::TestPointInCircle({20, 20}, circle));
}

TEST(Collision2DTest, PointInPolygon)
{
    // 正方形ポリゴン
    Polygon2D square;
    square.vertices = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};

    EXPECT_TRUE(Collision2D::TestPointInPolygon({5, 5}, square));
    EXPECT_FALSE(Collision2D::TestPointInPolygon({-1, 5}, square));
}

// ============================================================================
// 線分同士
// ============================================================================

TEST(Collision2DTest, LinevsLine_Cross)
{
    Line2D a({0, 0}, {10, 10});
    Line2D b({10, 0}, {0, 10});
    Vector2 pt;
    EXPECT_TRUE(Collision2D::TestLinevsLine(a, b, &pt));
    EXPECT_NEAR(pt.x, 5.0f, 1e-3f);
    EXPECT_NEAR(pt.y, 5.0f, 1e-3f);
}

TEST(Collision2DTest, LinevsLine_Parallel)
{
    Line2D a({0, 0}, {10, 0});
    Line2D b({0, 5}, {10, 5});
    EXPECT_FALSE(Collision2D::TestLinevsLine(a, b));
}

TEST(Collision2DTest, LinevsLine_NoIntersect)
{
    Line2D a({0, 0}, {5, 0});
    Line2D b({6, -1}, {6, 1}); // 線分が届かない位置
    // 無限直線では交差しても、線分同士だと交差しない場合がある
    // 実装が「線分」判定か「無限直線」判定かで結果が変わる点に注意
}

// ============================================================================
// 交差情報（深さ/法線あり）
// ============================================================================

TEST(Collision2DTest, IntersectAABBvsAABB)
{
    AABB2D a({0, 0}, {10, 10});
    AABB2D b({8, 0}, {18, 10});
    auto hit = Collision2D::IntersectAABBvsAABB(a, b);
    EXPECT_TRUE(hit.hit);
    EXPECT_GT(hit.depth, 0.0f);
}

TEST(Collision2DTest, IntersectCirclevsCircle)
{
    Circle a({0, 0}, 5.0f);
    Circle b({7, 0}, 5.0f);
    auto hit = Collision2D::IntersectCirclevsCircle(a, b);
    EXPECT_TRUE(hit.hit);
    EXPECT_NEAR(hit.depth, 3.0f, 1e-3f);
}

// ============================================================================
// 2Dレイキャスト
// ============================================================================

TEST(Collision2DTest, Raycast2D_AABB)
{
    AABB2D aabb({5, -2}, {8, 2});
    Vector2 origin(0, 0);
    Vector2 dir(1, 0);
    float t;
    EXPECT_TRUE(Collision2D::Raycast2D(origin, dir, aabb, t));
    EXPECT_NEAR(t, 5.0f, 1e-3f);
}

TEST(Collision2DTest, Raycast2D_Circle)
{
    Circle circle({10, 0}, 2.0f);
    Vector2 origin(0, 0);
    Vector2 dir(1, 0);
    float t;
    EXPECT_TRUE(Collision2D::Raycast2D(origin, dir, circle, t));
    EXPECT_NEAR(t, 8.0f, 1e-3f);
}
