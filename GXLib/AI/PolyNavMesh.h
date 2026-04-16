#pragma once
/// @file PolyNavMesh.h
/// @brief ポリゴンベースNavMesh（三角形メッシュ＋A*＋ファンネルアルゴリズム）
///
/// 三角形メッシュ上でA*経路探索を行い、ファンネルアルゴリズムで
/// 滑らかなパスを生成する。

#include "pch_common.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <queue>
#include <functional>

namespace gx
{
/// @addtogroup grp_ai
/// @{

/// @brief 3Dポイント（PolyNavMesh専用、軽量構造体）
struct NavPoint
{
    float x = 0.0f, y = 0.0f, z = 0.0f;

    NavPoint() = default;
    NavPoint(float ax, float ay, float az) : x(ax), y(ay), z(az) {}

    NavPoint operator+(const NavPoint& o) const { return { x + o.x, y + o.y, z + o.z }; }
    NavPoint operator-(const NavPoint& o) const { return { x - o.x, y - o.y, z - o.z }; }
    NavPoint operator*(float s) const { return { x * s, y * s, z * s }; }

    float LengthSq() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrt(LengthSq()); }

    static float Dot(const NavPoint& a, const NavPoint& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static NavPoint Cross(const NavPoint& a, const NavPoint& b)
    {
        return { a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x };
    }

    static float DistanceSq(const NavPoint& a, const NavPoint& b)
    {
        return (a - b).LengthSq();
    }

    static float Distance(const NavPoint& a, const NavPoint& b)
    {
        return (a - b).Length();
    }
};

/// @brief ナビゲーション三角形
struct NavTriangle
{
    NavPoint v0, v1, v2;           ///< 頂点座標
    int neighborIndices[3] = { -1, -1, -1 }; ///< 隣接三角形インデックス（-1=なし）
};

/// @brief ファンネルアルゴリズム用ポータル
struct NavPortal
{
    NavPoint left;   ///< ポータル左端
    NavPoint right;  ///< ポータル右端
};

/// @brief ポリゴンベースNavMesh
///
/// 三角形メッシュを入力としてNavMeshを構築し、A*経路探索＋
/// ファンネルアルゴリズムによるパス平滑化を行う。
class PolyNavMesh
{
public:
    PolyNavMesh() = default;
    ~PolyNavMesh() = default;

    /// @brief 三角形メッシュからNavMeshを構築
    /// @param vertices 頂点配列（x,y,z の繰り返し）
    /// @param vertexCount 頂点数
    /// @param indices インデックス配列（3つずつで三角形）
    /// @param indexCount インデックス数（3の倍数）
    void Build(const float* vertices, uint32_t vertexCount,
               const uint32_t* indices, uint32_t indexCount);

    /// @brief A*＋ファンネルで経路探索
    /// @param start 開始位置
    /// @param end 終了位置
    /// @return ウェイポイント列（空なら経路なし）
    gx::Vector<NavPoint> FindPath(const NavPoint& start, const NavPoint& end) const;

    /// @brief 最寄りのポリゴンインデックスを検索
    int FindNearestPolygon(const NavPoint& point) const;

    /// @brief ポイントがメッシュ内にあるか判定
    bool IsPointInMesh(const NavPoint& point) const;

    /// @brief ポリゴン（三角形）数を取得
    uint32_t GetPolygonCount() const { return static_cast<uint32_t>(m_triangles.size()); }

    /// @brief 三角形データを取得
    const NavTriangle* GetTriangle(uint32_t index) const;

    /// @brief 最大傾斜角度を設定（度）
    void SetMaxSlope(float degrees) { m_maxSlope = degrees; }

    /// @brief 最大傾斜角度を取得（度）
    float GetMaxSlope() const { return m_maxSlope; }

    /// @brief NavMeshをクリア
    void Clear();

private:
    /// @brief 隣接関係を計算（共有エッジ検出）
    void ComputeAdjacency();

    /// @brief A*でポリゴングラフ上の経路を探索
    /// @return ポリゴンインデックス列
    gx::Vector<int> AStarOnGraph(int startTri, int endTri) const;

    /// @brief ファンネルアルゴリズムでパスを平滑化
    gx::Vector<NavPoint> FunnelSmooth(const gx::Vector<int>& triPath,
                                       const NavPoint& start,
                                       const NavPoint& end) const;

    /// @brief 点が三角形内にあるか判定（重心座標テスト）
    static bool PointInTriangle(const NavPoint& p, const NavTriangle& tri);

    /// @brief 三角形の重心を計算
    static NavPoint TriangleCenter(const NavTriangle& tri);

    /// @brief 2つの三角形の共有エッジからポータルを生成
    static NavPortal SharedEdge(const NavTriangle& triA, const NavTriangle& triB);

    /// @brief 三角形の法線を計算
    static NavPoint TriangleNormal(const NavTriangle& tri);

    gx::Vector<NavTriangle> m_triangles;   ///< 三角形配列
    float m_maxSlope = 45.0f;               ///< 最大傾斜角度（度）
};

/// @}
} // namespace gx
