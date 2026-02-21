#pragma once
#include "Math/Vector2.h"

namespace GX {

// --- 2D形状定義 ---

/// @brief 2D軸平行境界ボックス（AABB）
///
/// 矩形の当たり判定に使う。DxLibでは画面座標の矩形判定に相当する。
/// min/maxの2頂点で矩形を定義する。
struct AABB2D {
    Vector2 min;    ///< 左下隅の座標
    Vector2 max;    ///< 右上隅の座標

    AABB2D() = default;

    /// @brief min/maxで初期化する
    /// @param min 左下隅
    /// @param max 右上隅
    AABB2D(const Vector2& min, const Vector2& max) : min(min), max(max) {}

    /// @brief 左上座標+サイズで初期化する
    /// @param x 左上X
    /// @param y 左上Y
    /// @param w 幅
    /// @param h 高さ
    AABB2D(float x, float y, float w, float h) : min(x, y), max(x + w, y + h) {}

    /// @brief 中心座標を取得する
    /// @return 矩形の中心
    Vector2 Center() const { return (min + max) * 0.5f; }

    /// @brief サイズ（幅・高さ）を取得する
    /// @return サイズベクトル
    Vector2 Size() const { return max - min; }

    /// @brief 半サイズを取得する
    /// @return 半サイズベクトル
    Vector2 HalfSize() const { return (max - min) * 0.5f; }

    /// @brief 点が矩形内にあるか判定する
    /// @param point 判定する点
    /// @return 矩形内ならtrue
    bool Contains(const Vector2& point) const
    {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }

    /// @brief 全方向にマージン分拡大した矩形を返す
    /// @param margin 拡大量
    /// @return 拡大後の矩形
    AABB2D Expand(float margin) const
    {
        return { { min.x - margin, min.y - margin },
                 { max.x + margin, max.y + margin } };
    }

    /// @brief 他の矩形と統合した最小の包含矩形を返す
    /// @param other 統合する矩形
    /// @return 両方を含む最小の矩形
    AABB2D Merged(const AABB2D& other) const
    {
        return { Vector2::Min(min, other.min), Vector2::Max(max, other.max) };
    }

    /// @brief 面積を取得する
    /// @return 矩形の面積
    float Area() const
    {
        Vector2 s = Size();
        return s.x * s.y;
    }
};

/// @brief 2D円形状
///
/// 円による当たり判定に使う。中心座標と半径で定義する。
struct Circle {
    Vector2 center;     ///< 中心座標
    float radius = 0.0f; ///< 半径

    Circle() = default;

    /// @brief 中心と半径で初期化する
    /// @param c 中心座標
    /// @param r 半径
    Circle(const Vector2& c, float r) : center(c), radius(r) {}

    /// @brief XY座標と半径で初期化する
    /// @param x 中心X
    /// @param y 中心Y
    /// @param r 半径
    Circle(float x, float y, float r) : center(x, y), radius(r) {}

    /// @brief 点が円内にあるか判定する
    /// @param point 判定する点
    /// @return 円内ならtrue
    bool Contains(const Vector2& point) const
    {
        return center.DistanceSquared(point) <= radius * radius;
    }
};

/// @brief 2D線分
///
/// 始点と終点で定義される有限長の線分。
struct Line2D {
    Vector2 start;  ///< 始点
    Vector2 end;    ///< 終点

    Line2D() = default;

    /// @brief 始点と終点で初期化する
    /// @param s 始点
    /// @param e 終点
    Line2D(const Vector2& s, const Vector2& e) : start(s), end(e) {}

    /// @brief 線分の長さを取得する
    /// @return 始点から終点までの距離
    float Length() const { return start.Distance(end); }

    /// @brief 線分の方向ベクトル（正規化済み）を取得する
    /// @return 始点から終点への単位ベクトル
    Vector2 Direction() const { return (end - start).Normalized(); }

    /// @brief 線分上で指定した点に最も近い点を返す
    /// @param point 対象の点
    /// @return 線分上の最近点
    Vector2 ClosestPoint(const Vector2& point) const
    {
        Vector2 ab = end - start;
        float t = (point - start).Dot(ab) / ab.Dot(ab);
        t = MathUtil::Clamp(t, 0.0f, 1.0f);
        return start + ab * t;
    }
};

/// @brief 2D多角形（頂点列で定義）
///
/// 任意の凸/凹多角形を表現する。頂点は順序付きで格納する。
struct Polygon2D {
    std::vector<Vector2> vertices; ///< 頂点列（時計回りまたは反時計回り）

    /// @brief 点が多角形内にあるか判定する（巻き数法）
    /// @param point 判定する点
    /// @return 多角形内ならtrue
    bool Contains(const Vector2& point) const;

    /// @brief 多角形の包含AABBを取得する
    /// @return 全頂点を含む最小のAABB
    AABB2D GetBounds() const;
};

// --- 衝突結果 ---

/// @brief 2D衝突判定の結果情報
///
/// 衝突の有無に加え、衝突点・法線・めり込み深さを保持する。
/// bool変換で衝突判定の結果をそのままif文で使える。
struct HitResult2D {
    bool hit = false;       ///< 衝突したかどうか
    Vector2 point;          ///< 衝突点（ワールド座標）
    Vector2 normal;         ///< 衝突法線（押し出し方向）
    float depth = 0.0f;     ///< めり込み深さ

    /// @brief boolへの暗黙変換（hitを返す）
    operator bool() const { return hit; }
};

// --- 衝突判定関数 ---

/// @brief 2D衝突判定ユーティリティ
///
/// 矩形・円・線分・多角形の2D当たり判定関数をまとめた名前空間。
/// DxLibにはない高機能な衝突判定を提供する。
namespace Collision2D {

    // --- 判定のみ（true/false） ---

    /// @brief AABB同士の衝突判定
    /// @param a 1つ目のAABB
    /// @param b 2つ目のAABB
    /// @return 衝突していればtrue
    bool TestAABBvsAABB(const AABB2D& a, const AABB2D& b);

    /// @brief 円同士の衝突判定
    /// @param a 1つ目の円
    /// @param b 2つ目の円
    /// @return 衝突していればtrue
    bool TestCirclevsCircle(const Circle& a, const Circle& b);

    /// @brief AABBと円の衝突判定
    /// @param aabb 矩形
    /// @param circle 円
    /// @return 衝突していればtrue
    bool TestAABBvsCircle(const AABB2D& aabb, const Circle& circle);

    /// @brief 点がAABB内にあるか判定
    /// @param point 判定する点
    /// @param aabb 矩形
    /// @return 矩形内ならtrue
    bool TestPointInAABB(const Vector2& point, const AABB2D& aabb);

    /// @brief 点が円内にあるか判定
    /// @param point 判定する点
    /// @param circle 円
    /// @return 円内ならtrue
    bool TestPointInCircle(const Vector2& point, const Circle& circle);

    /// @brief 点が多角形内にあるか判定
    /// @param point 判定する点
    /// @param polygon 多角形
    /// @return 多角形内ならtrue
    bool TestPointInPolygon(const Vector2& point, const Polygon2D& polygon);

    /// @brief 線分とAABBの衝突判定
    /// @param line 線分
    /// @param aabb 矩形
    /// @return 交差していればtrue
    bool TestLinevsAABB(const Line2D& line, const AABB2D& aabb);

    /// @brief 線分と円の衝突判定
    /// @param line 線分
    /// @param circle 円
    /// @return 交差していればtrue
    bool TestLinevsCircle(const Line2D& line, const Circle& circle);

    /// @brief 線分同士の衝突判定
    /// @param a 1つ目の線分
    /// @param b 2つ目の線分
    /// @param outPoint 交差点の出力先（nullptrで省略可）
    /// @return 交差していればtrue
    bool TestLinevsLine(const Line2D& a, const Line2D& b, Vector2* outPoint = nullptr);

    // --- 交差情報付き（法線・貫通深度を含む） ---

    /// @brief AABB同士の交差情報を取得する
    /// @param a 1つ目のAABB
    /// @param b 2つ目のAABB
    /// @return 衝突結果（法線・深さ付き）
    HitResult2D IntersectAABBvsAABB(const AABB2D& a, const AABB2D& b);

    /// @brief 円同士の交差情報を取得する
    /// @param a 1つ目の円
    /// @param b 2つ目の円
    /// @return 衝突結果（法線・深さ付き）
    HitResult2D IntersectCirclevsCircle(const Circle& a, const Circle& b);

    /// @brief AABBと円の交差情報を取得する
    /// @param aabb 矩形
    /// @param circle 円
    /// @return 衝突結果（法線・深さ付き）
    HitResult2D IntersectAABBvsCircle(const AABB2D& aabb, const Circle& circle);

    // --- レイキャスト ---

    /// @brief レイとAABBの交差判定
    /// @param origin レイの始点
    /// @param direction レイの方向
    /// @param aabb 矩形
    /// @param outT ヒット位置のパラメータt（始点からの距離比）
    /// @param outNormal ヒット法線の出力先（nullptrで省略可）
    /// @return ヒットした場合true
    bool Raycast2D(const Vector2& origin, const Vector2& direction, const AABB2D& aabb,
                   float& outT, Vector2* outNormal = nullptr);

    /// @brief レイと円の交差判定
    /// @param origin レイの始点
    /// @param direction レイの方向
    /// @param circle 円
    /// @param outT ヒット位置のパラメータt
    /// @return ヒットした場合true
    bool Raycast2D(const Vector2& origin, const Vector2& direction, const Circle& circle,
                   float& outT);

    // --- スイープ ---

    /// @brief 移動する円同士の衝突時刻を求める
    /// @param a 1つ目の円
    /// @param velA aの移動速度
    /// @param b 2つ目の円
    /// @param velB bの移動速度
    /// @param outT 衝突時刻（0〜1、0=フレーム開始、1=フレーム終了）
    /// @return 衝突する場合true
    bool SweepCirclevsCircle(const Circle& a, const Vector2& velA,
                              const Circle& b, const Vector2& velB,
                              float& outT);

} // namespace Collision2D
} // namespace GX
