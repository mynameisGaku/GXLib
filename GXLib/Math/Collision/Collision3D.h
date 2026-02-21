#pragma once
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"

namespace GX {

// --- 3D形状定義 ---

/// @brief 3D軸平行境界ボックス（AABB）
///
/// 3Dオブジェクトの大まかな範囲を表す直方体。
/// 各辺がXYZ軸に平行なので高速に判定できる。
struct AABB3D {
    Vector3 min;    ///< 最小隅の座標
    Vector3 max;    ///< 最大隅の座標

    AABB3D() = default;

    /// @brief min/maxで初期化する
    /// @param min 最小隅
    /// @param max 最大隅
    AABB3D(const Vector3& min, const Vector3& max) : min(min), max(max) {}

    /// @brief 中心座標を取得する
    /// @return 直方体の中心
    Vector3 Center() const { return (min + max) * 0.5f; }

    /// @brief サイズ（各辺の長さ）を取得する
    /// @return サイズベクトル
    Vector3 Size() const { return max - min; }

    /// @brief 半サイズ（中心から各面までの距離）を取得する
    /// @return 半サイズベクトル
    Vector3 HalfExtents() const { return (max - min) * 0.5f; }

    /// @brief 点がAABB内にあるか判定する
    /// @param point 判定する点
    /// @return AABB内ならtrue
    bool Contains(const Vector3& point) const
    {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    /// @brief 全方向にマージン分拡大したAABBを返す
    /// @param margin 拡大量
    /// @return 拡大後のAABB
    AABB3D Expand(float margin) const
    {
        return { { min.x - margin, min.y - margin, min.z - margin },
                 { max.x + margin, max.y + margin, max.z + margin } };
    }

    /// @brief 他のAABBと統合した最小の包含AABBを返す
    /// @param other 統合するAABB
    /// @return 両方を含む最小のAABB
    AABB3D Merged(const AABB3D& other) const
    {
        return { Vector3::Min(min, other.min), Vector3::Max(max, other.max) };
    }

    /// @brief 体積を取得する
    /// @return AABBの体積
    float Volume() const
    {
        Vector3 s = Size();
        return s.x * s.y * s.z;
    }

    /// @brief 表面積を取得する（BVHのSAHコスト計算に使用）
    /// @return AABBの表面積
    float SurfaceArea() const
    {
        Vector3 s = Size();
        return 2.0f * (s.x * s.y + s.y * s.z + s.z * s.x);
    }
};

/// @brief 3D球形状
///
/// 球による当たり判定に使う。中心座標と半径で定義する。
struct Sphere {
    Vector3 center;      ///< 中心座標
    float radius = 0.0f; ///< 半径

    Sphere() = default;

    /// @brief 中心と半径で初期化する
    /// @param c 中心座標
    /// @param r 半径
    Sphere(const Vector3& c, float r) : center(c), radius(r) {}

    /// @brief 点が球内にあるか判定する
    /// @param point 判定する点
    /// @return 球内ならtrue
    bool Contains(const Vector3& point) const
    {
        return center.DistanceSquared(point) <= radius * radius;
    }
};

/// @brief 3Dレイ（始点+方向の半直線）
///
/// レイキャストに使う。始点と方向ベクトルで定義する。
struct Ray {
    Vector3 origin;     ///< レイの始点
    Vector3 direction;  ///< レイの方向（正規化推奨）

    Ray() = default;

    /// @brief 始点と方向で初期化する
    /// @param o 始点
    /// @param d 方向ベクトル
    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d) {}

    /// @brief レイ上のパラメータtの位置を取得する
    /// @param t パラメータ（始点からの距離比）
    /// @return レイ上の点
    Vector3 GetPoint(float t) const { return origin + direction * t; }
};

/// @brief 3D平面（法線+原点からの距離）
///
/// 視錐台カリングや衝突判定の基礎要素。
struct Plane {
    Vector3 normal;       ///< 平面の法線（正規化されていること）
    float distance = 0.0f; ///< 原点から平面までの符号付き距離

    Plane() = default;

    /// @brief 法線と距離で初期化する
    /// @param n 法線
    /// @param d 原点からの距離
    Plane(const Vector3& n, float d) : normal(n), distance(d) {}

    /// @brief 法線と平面上の点で初期化する
    /// @param n 法線
    /// @param point 平面上の1点
    Plane(const Vector3& n, const Vector3& point) : normal(n), distance(n.Dot(point)) {}

    /// @brief 点から平面までの符号付き距離を取得する
    /// @param point 対象の点
    /// @return 法線方向が正の符号付き距離
    float DistanceToPoint(const Vector3& point) const
    {
        return normal.Dot(point) - distance;
    }
};

/// @brief 視錐台（6平面で定義されるカメラの見える範囲）
///
/// カメラの描画範囲に入っているかどうかのカリング判定に使う。
struct Frustum {
    Plane planes[6]; ///< 6平面: [0]=近面 [1]=遠面 [2]=左 [3]=右 [4]=上 [5]=下

    /// @brief ビュー×プロジェクション行列から視錐台を構築する
    /// @param viewProj ビュープロジェクション行列
    /// @return 構築された視錐台
    static Frustum FromViewProjection(const XMMATRIX& viewProj);
};

/// @brief 有向境界ボックス（OBB: Oriented Bounding Box）
///
/// 回転可能な直方体。AABBより密にオブジェクトを囲めるが判定は重い。
struct OBB {
    Vector3 center;         ///< 中心座標
    Vector3 halfExtents;    ///< 各軸の半サイズ
    Vector3 axes[3];        ///< ローカル座標軸（正規化済み）

    OBB() = default;

    /// @brief 中心・半サイズ・回転行列で初期化する
    /// @param center 中心座標
    /// @param halfExtents 各軸の半サイズ
    /// @param rotation 回転行列
    OBB(const Vector3& center, const Vector3& halfExtents, const Matrix4x4& rotation);
};

/// @brief 3D三角形
///
/// レイキャストやメッシュ衝突判定の基本要素。
struct Triangle {
    Vector3 v0, v1, v2; ///< 三角形の3頂点

    Triangle() = default;

    /// @brief 3頂点で初期化する
    /// @param a 頂点0
    /// @param b 頂点1
    /// @param c 頂点2
    Triangle(const Vector3& a, const Vector3& b, const Vector3& c) : v0(a), v1(b), v2(c) {}

    /// @brief 三角形の法線を取得する（正規化済み）
    /// @return 法線ベクトル（v0→v1 と v0→v2 の外積）
    Vector3 Normal() const
    {
        return (v1 - v0).Cross(v2 - v0).Normalized();
    }
};

// --- 衝突結果 ---

/// @brief 3D衝突判定の結果情報
///
/// 衝突の有無に加え、衝突点・法線・めり込み深さ・レイパラメータを保持する。
struct HitResult3D {
    bool hit = false;       ///< 衝突したかどうか
    Vector3 point;          ///< 衝突点（ワールド座標）
    Vector3 normal;         ///< 衝突法線（押し出し方向）
    float depth = 0.0f;     ///< めり込み深さ
    float t = 0.0f;         ///< レイキャスト時のパラメータt

    /// @brief boolへの暗黙変換（hitを返す）
    operator bool() const { return hit; }
};

// --- 衝突判定関数 ---

/// @brief 3D衝突判定ユーティリティ
///
/// 球・AABB・OBB・三角形・平面・視錐台の3D当たり判定関数をまとめた名前空間。
/// レイキャストや最近点計算も含む。
namespace Collision3D {

    // --- 判定のみ（true/false） ---

    /// @brief 球同士の衝突判定
    /// @param a 1つ目の球
    /// @param b 2つ目の球
    /// @return 衝突していればtrue
    bool TestSphereVsSphere(const Sphere& a, const Sphere& b);

    /// @brief AABB同士の衝突判定
    /// @param a 1つ目のAABB
    /// @param b 2つ目のAABB
    /// @return 衝突していればtrue
    bool TestAABBVsAABB(const AABB3D& a, const AABB3D& b);

    /// @brief 球とAABBの衝突判定
    /// @param sphere 球
    /// @param aabb AABB
    /// @return 衝突していればtrue
    bool TestSphereVsAABB(const Sphere& sphere, const AABB3D& aabb);

    /// @brief 点が球内にあるか判定
    /// @param point 判定する点
    /// @param sphere 球
    /// @return 球内ならtrue
    bool TestPointInSphere(const Vector3& point, const Sphere& sphere);

    /// @brief 点がAABB内にあるか判定
    /// @param point 判定する点
    /// @param aabb AABB
    /// @return AABB内ならtrue
    bool TestPointInAABB(const Vector3& point, const AABB3D& aabb);

    /// @brief OBB同士の衝突判定（分離軸定理）
    /// @param a 1つ目のOBB
    /// @param b 2つ目のOBB
    /// @return 衝突していればtrue
    bool TestOBBVsOBB(const OBB& a, const OBB& b);

    /// @brief 視錐台と球の包含判定（カリング用）
    /// @param frustum 視錐台
    /// @param sphere 球
    /// @return 視錐台内にあればtrue
    bool TestFrustumVsSphere(const Frustum& frustum, const Sphere& sphere);

    /// @brief 視錐台とAABBの包含判定（カリング用）
    /// @param frustum 視錐台
    /// @param aabb AABB
    /// @return 視錐台内にあればtrue
    bool TestFrustumVsAABB(const Frustum& frustum, const AABB3D& aabb);

    /// @brief 視錐台と点の包含判定（カリング用）
    /// @param frustum 視錐台
    /// @param point 点
    /// @return 視錐台内にあればtrue
    bool TestFrustumVsPoint(const Frustum& frustum, const Vector3& point);

    // --- レイキャスト ---

    /// @brief レイと球の交差判定
    /// @param ray レイ
    /// @param sphere 球
    /// @param outT ヒット位置のパラメータt
    /// @return ヒットした場合true
    bool RaycastSphere(const Ray& ray, const Sphere& sphere, float& outT);

    /// @brief レイとAABBの交差判定（スラブ法）
    /// @param ray レイ
    /// @param aabb AABB
    /// @param outT ヒット位置のパラメータt
    /// @return ヒットした場合true
    bool RaycastAABB(const Ray& ray, const AABB3D& aabb, float& outT);

    /// @brief レイと三角形の交差判定（Moller-Trumbore法）
    /// @param ray レイ
    /// @param tri 三角形
    /// @param outT ヒット位置のパラメータt
    /// @param outU 重心座標u
    /// @param outV 重心座標v
    /// @return ヒットした場合true
    bool RaycastTriangle(const Ray& ray, const Triangle& tri, float& outT, float& outU, float& outV);

    /// @brief レイと平面の交差判定
    /// @param ray レイ
    /// @param plane 平面
    /// @param outT ヒット位置のパラメータt
    /// @return ヒットした場合true
    bool RaycastPlane(const Ray& ray, const Plane& plane, float& outT);

    /// @brief レイとOBBの交差判定
    /// @param ray レイ
    /// @param obb OBB
    /// @param outT ヒット位置のパラメータt
    /// @return ヒットした場合true
    bool RaycastOBB(const Ray& ray, const OBB& obb, float& outT);

    // --- 交差情報付き ---

    /// @brief 球同士の交差情報を取得する
    /// @param a 1つ目の球
    /// @param b 2つ目の球
    /// @return 衝突結果（法線・深さ付き）
    HitResult3D IntersectSphereVsSphere(const Sphere& a, const Sphere& b);

    /// @brief 球とAABBの交差情報を取得する
    /// @param sphere 球
    /// @param aabb AABB
    /// @return 衝突結果（法線・深さ付き）
    HitResult3D IntersectSphereVsAABB(const Sphere& sphere, const AABB3D& aabb);

    // --- スイープ ---

    /// @brief 移動する球同士の衝突時刻を求める
    /// @param a 1つ目の球
    /// @param velA aの移動速度
    /// @param b 2つ目の球
    /// @param velB bの移動速度
    /// @param outT 衝突時刻
    /// @return 衝突する場合true
    bool SweepSphereVsSphere(const Sphere& a, const Vector3& velA,
                              const Sphere& b, const Vector3& velB, float& outT);

    // --- 最近点計算 ---

    /// @brief AABB上で指定した点に最も近い点を返す
    /// @param point 対象の点
    /// @param aabb AABB
    /// @return AABB上の最近点
    Vector3 ClosestPointOnAABB(const Vector3& point, const AABB3D& aabb);

    /// @brief 三角形上で指定した点に最も近い点を返す
    /// @param point 対象の点
    /// @param tri 三角形
    /// @return 三角形上の最近点
    Vector3 ClosestPointOnTriangle(const Vector3& point, const Triangle& tri);

    /// @brief 線分上で指定した点に最も近い点を返す
    /// @param point 対象の点
    /// @param lineA 線分の始点
    /// @param lineB 線分の終点
    /// @return 線分上の最近点
    Vector3 ClosestPointOnLine(const Vector3& point, const Vector3& lineA, const Vector3& lineB);

} // namespace Collision3D
} // namespace GX
