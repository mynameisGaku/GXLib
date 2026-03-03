#include "pch_common.h"
/// @file PolyNavMesh.cpp
/// @brief ポリゴンベースNavMesh実装

#include "AI/PolyNavMesh.h"
#include <unordered_set>
#include <unordered_map>
#include <limits>

namespace gx
{

// ============================================================================
// Build
// ============================================================================

void PolyNavMesh::Build(const float* vertices, uint32_t vertexCount,
                        const uint32_t* indices, uint32_t indexCount)
{
    Clear();

    if (!vertices || !indices || vertexCount == 0 || indexCount == 0) return;
    if (indexCount % 3 != 0) return;

    uint32_t triCount = indexCount / 3;
    m_triangles.resize(triCount);

    float maxSlopeRad = m_maxSlope * 3.14159265f / 180.0f;
    float cosSlopeThreshold = std::cos(maxSlopeRad);

    for (uint32_t i = 0; i < triCount; ++i)
    {
        uint32_t i0 = indices[i * 3 + 0];
        uint32_t i1 = indices[i * 3 + 1];
        uint32_t i2 = indices[i * 3 + 2];

        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) continue;

        m_triangles[i].v0 = { vertices[i0 * 3 + 0], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2] };
        m_triangles[i].v1 = { vertices[i1 * 3 + 0], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2] };
        m_triangles[i].v2 = { vertices[i2 * 3 + 0], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2] };
        m_triangles[i].neighborIndices[0] = -1;
        m_triangles[i].neighborIndices[1] = -1;
        m_triangles[i].neighborIndices[2] = -1;
    }

    // 傾斜フィルタ: 急すぎる三角形を除去
    if (m_maxSlope < 90.0f)
    {
        auto it = m_triangles.begin();
        while (it != m_triangles.end())
        {
            NavPoint normal = TriangleNormal(*it);
            float len = normal.Length();
            if (len > 1e-6f)
            {
                float ny = std::abs(normal.y / len); // 正規化Y成分（絶対値: 巻き方向に依存しない）
                if (ny < cosSlopeThreshold)
                {
                    it = m_triangles.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    ComputeAdjacency();
}

// ============================================================================
// ComputeAdjacency
// ============================================================================

void PolyNavMesh::ComputeAdjacency()
{
    uint32_t triCount = static_cast<uint32_t>(m_triangles.size());

    // 各三角形のエッジ（頂点ペア）を比較して共有エッジを検出
    // エッジ: (v0,v1), (v1,v2), (v2,v0)
    auto getEdgeVertex = [](const NavTriangle& tri, int edgeIdx, NavPoint& a, NavPoint& b)
    {
        switch (edgeIdx)
        {
        case 0: a = tri.v0; b = tri.v1; break;
        case 1: a = tri.v1; b = tri.v2; break;
        case 2: a = tri.v2; b = tri.v0; break;
        }
    };

    auto vertexMatch = [](const NavPoint& a, const NavPoint& b) -> bool
    {
        float eps = 1e-4f;
        return std::abs(a.x - b.x) < eps &&
               std::abs(a.y - b.y) < eps &&
               std::abs(a.z - b.z) < eps;
    };

    for (uint32_t i = 0; i < triCount; ++i)
    {
        for (uint32_t j = i + 1; j < triCount; ++j)
        {
            // 各エッジの組み合わせを調べる
            for (int ei = 0; ei < 3; ++ei)
            {
                NavPoint a1, b1;
                getEdgeVertex(m_triangles[i], ei, a1, b1);

                for (int ej = 0; ej < 3; ++ej)
                {
                    NavPoint a2, b2;
                    getEdgeVertex(m_triangles[j], ej, a2, b2);

                    // 共有エッジ: (a1==a2 && b1==b2) || (a1==b2 && b1==a2)
                    if ((vertexMatch(a1, a2) && vertexMatch(b1, b2)) ||
                        (vertexMatch(a1, b2) && vertexMatch(b1, a2)))
                    {
                        m_triangles[i].neighborIndices[ei] = static_cast<int>(j);
                        m_triangles[j].neighborIndices[ej] = static_cast<int>(i);
                    }
                }
            }
        }
    }
}

// ============================================================================
// FindPath
// ============================================================================

gx::Vector<NavPoint> PolyNavMesh::FindPath(const NavPoint& start, const NavPoint& end) const
{
    if (m_triangles.empty()) return {};

    int startTri = FindNearestPolygon(start);
    int endTri = FindNearestPolygon(end);

    if (startTri < 0 || endTri < 0) return {};

    // 同一三角形なら直線パス
    if (startTri == endTri)
    {
        return { start, end };
    }

    // A*でポリゴン列を取得
    gx::Vector<int> triPath = AStarOnGraph(startTri, endTri);
    if (triPath.empty()) return {};

    // ファンネルアルゴリズムでパスを平滑化
    return FunnelSmooth(triPath, start, end);
}

// ============================================================================
// AStarOnGraph
// ============================================================================

gx::Vector<int> PolyNavMesh::AStarOnGraph(int startTri, int endTri) const
{
    struct ANode
    {
        int triIndex;
        float g, f;
        bool operator>(const ANode& o) const { return f > o.f; }
    };

    uint32_t triCount = static_cast<uint32_t>(m_triangles.size());
    NavPoint goalCenter = TriangleCenter(m_triangles[endTri]);

    gx::Vector<float> gScore(triCount, std::numeric_limits<float>::max());
    gx::Vector<int> cameFrom(triCount, -1);
    gx::Vector<bool> closed(triCount, false);

    gx::PriorityQueue<ANode, gx::Deque<ANode>, std::greater<ANode>> openSet;

    gScore[startTri] = 0.0f;
    NavPoint startCenter = TriangleCenter(m_triangles[startTri]);
    float h = NavPoint::Distance(startCenter, goalCenter);
    openSet.push({ startTri, 0.0f, h });

    while (!openSet.empty())
    {
        ANode current = openSet.top();
        openSet.pop();

        if (current.triIndex == endTri)
        {
            // パスを再構築
            gx::Vector<int> path;
            int idx = endTri;
            while (idx != -1)
            {
                path.push_back(idx);
                idx = cameFrom[idx];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (closed[current.triIndex]) continue;
        closed[current.triIndex] = true;

        const NavTriangle& tri = m_triangles[current.triIndex];
        NavPoint curCenter = TriangleCenter(tri);

        for (int i = 0; i < 3; ++i)
        {
            int neighbor = tri.neighborIndices[i];
            if (neighbor < 0 || neighbor >= static_cast<int>(triCount)) continue;
            if (closed[neighbor]) continue;

            NavPoint neighborCenter = TriangleCenter(m_triangles[neighbor]);
            float tentativeG = gScore[current.triIndex] + NavPoint::Distance(curCenter, neighborCenter);

            if (tentativeG < gScore[neighbor])
            {
                gScore[neighbor] = tentativeG;
                cameFrom[neighbor] = current.triIndex;
                float hN = NavPoint::Distance(neighborCenter, goalCenter);
                openSet.push({ neighbor, tentativeG, tentativeG + hN });
            }
        }
    }

    return {}; // パスなし
}

// ============================================================================
// FunnelSmooth (Simple Stupid Funnel Algorithm)
// ============================================================================

gx::Vector<NavPoint> PolyNavMesh::FunnelSmooth(const gx::Vector<int>& triPath,
                                                 const NavPoint& start,
                                                 const NavPoint& end) const
{
    if (triPath.size() <= 1)
    {
        return { start, end };
    }

    // ポータル列を構築
    gx::Vector<NavPortal> portals;

    // 開始ポータル（開始点自身）
    portals.push_back({ start, start });

    for (size_t i = 0; i + 1 < triPath.size(); ++i)
    {
        NavPortal portal = SharedEdge(m_triangles[triPath[i]], m_triangles[triPath[i + 1]]);
        portals.push_back(portal);
    }

    // 終了ポータル（終了点自身）
    portals.push_back({ end, end });

    // ファンネル走査
    gx::Vector<NavPoint> path;
    path.push_back(start);

    NavPoint apex = start;
    NavPoint portalLeft = start;
    NavPoint portalRight = start;
    int apexIndex = 0;
    int leftIndex = 0;
    int rightIndex = 0;

    auto triArea2D = [](const NavPoint& a, const NavPoint& b, const NavPoint& c) -> float
    {
        return (b.x - a.x) * (c.z - a.z) - (c.x - a.x) * (b.z - a.z);
    };

    for (size_t i = 1; i < portals.size(); ++i)
    {
        const NavPoint& left = portals[i].left;
        const NavPoint& right = portals[i].right;

        // 右側を更新
        if (triArea2D(apex, portalRight, right) <= 0.0f)
        {
            if (NavPoint::DistanceSq(apex, portalRight) < 1e-6f ||
                triArea2D(apex, portalLeft, right) > 0.0f)
            {
                portalRight = right;
                rightIndex = static_cast<int>(i);
            }
            else
            {
                // 右が左を越えた → 左をウェイポイントに追加
                path.push_back(portalLeft);
                apex = portalLeft;
                apexIndex = leftIndex;
                portalLeft = apex;
                portalRight = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                i = static_cast<size_t>(apexIndex);
                continue;
            }
        }

        // 左側を更新
        if (triArea2D(apex, portalLeft, left) >= 0.0f)
        {
            if (NavPoint::DistanceSq(apex, portalLeft) < 1e-6f ||
                triArea2D(apex, portalRight, left) < 0.0f)
            {
                portalLeft = left;
                leftIndex = static_cast<int>(i);
            }
            else
            {
                // 左が右を越えた → 右をウェイポイントに追加
                path.push_back(portalRight);
                apex = portalRight;
                apexIndex = rightIndex;
                portalLeft = apex;
                portalRight = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                i = static_cast<size_t>(apexIndex);
                continue;
            }
        }
    }

    // 終点を追加
    if (path.empty() || NavPoint::DistanceSq(path.back(), end) > 1e-6f)
    {
        path.push_back(end);
    }

    return path;
}

// ============================================================================
// FindNearestPolygon
// ============================================================================

int PolyNavMesh::FindNearestPolygon(const NavPoint& point) const
{
    if (m_triangles.empty()) return -1;

    // まず三角形内部の点を探す
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_triangles.size()); ++i)
    {
        if (PointInTriangle(point, m_triangles[i]))
        {
            return static_cast<int>(i);
        }
    }

    // 三角形内部にない場合は最近傍の重心を探す
    int nearest = -1;
    float minDist = std::numeric_limits<float>::max();

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_triangles.size()); ++i)
    {
        NavPoint center = TriangleCenter(m_triangles[i]);
        float dist = NavPoint::DistanceSq(point, center);
        if (dist < minDist)
        {
            minDist = dist;
            nearest = static_cast<int>(i);
        }
    }

    return nearest;
}

// ============================================================================
// IsPointInMesh
// ============================================================================

bool PolyNavMesh::IsPointInMesh(const NavPoint& point) const
{
    for (const auto& tri : m_triangles)
    {
        if (PointInTriangle(point, tri)) return true;
    }
    return false;
}

// ============================================================================
// GetTriangle
// ============================================================================

const NavTriangle* PolyNavMesh::GetTriangle(uint32_t index) const
{
    if (index >= static_cast<uint32_t>(m_triangles.size())) return nullptr;
    return &m_triangles[index];
}

// ============================================================================
// Clear
// ============================================================================

void PolyNavMesh::Clear()
{
    m_triangles.clear();
}

// ============================================================================
// PointInTriangle (重心座標テスト)
// ============================================================================

bool PolyNavMesh::PointInTriangle(const NavPoint& p, const NavTriangle& tri)
{
    // 2D投影（XZ平面）で重心座標テスト
    NavPoint v0 = { tri.v2.x - tri.v0.x, 0.0f, tri.v2.z - tri.v0.z };
    NavPoint v1 = { tri.v1.x - tri.v0.x, 0.0f, tri.v1.z - tri.v0.z };
    NavPoint v2 = { p.x - tri.v0.x, 0.0f, p.z - tri.v0.z };

    float dot00 = v0.x * v0.x + v0.z * v0.z;
    float dot01 = v0.x * v1.x + v0.z * v1.z;
    float dot02 = v0.x * v2.x + v0.z * v2.z;
    float dot11 = v1.x * v1.x + v1.z * v1.z;
    float dot12 = v1.x * v2.x + v1.z * v2.z;

    float invDenom = dot00 * dot11 - dot01 * dot01;
    if (std::abs(invDenom) < 1e-10f) return false;
    invDenom = 1.0f / invDenom;

    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= -1e-5f) && (v >= -1e-5f) && (u + v <= 1.0f + 1e-5f);
}

// ============================================================================
// TriangleCenter
// ============================================================================

NavPoint PolyNavMesh::TriangleCenter(const NavTriangle& tri)
{
    return {
        (tri.v0.x + tri.v1.x + tri.v2.x) / 3.0f,
        (tri.v0.y + tri.v1.y + tri.v2.y) / 3.0f,
        (tri.v0.z + tri.v1.z + tri.v2.z) / 3.0f
    };
}

// ============================================================================
// SharedEdge
// ============================================================================

NavPortal PolyNavMesh::SharedEdge(const NavTriangle& triA, const NavTriangle& triB)
{
    // triAとtriBの共有頂点を見つける
    const NavPoint* vertsA[] = { &triA.v0, &triA.v1, &triA.v2 };
    const NavPoint* vertsB[] = { &triB.v0, &triB.v1, &triB.v2 };

    gx::Vector<NavPoint> shared;
    float eps = 1e-4f;

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            if (std::abs(vertsA[i]->x - vertsB[j]->x) < eps &&
                std::abs(vertsA[i]->y - vertsB[j]->y) < eps &&
                std::abs(vertsA[i]->z - vertsB[j]->z) < eps)
            {
                shared.push_back(*vertsA[i]);
                break;
            }
        }
    }

    if (shared.size() >= 2)
    {
        return { shared[0], shared[1] };
    }

    // フォールバック: 重心同士
    NavPoint cA = TriangleCenter(triA);
    NavPoint cB = TriangleCenter(triB);
    return { cA, cB };
}

// ============================================================================
// TriangleNormal
// ============================================================================

NavPoint PolyNavMesh::TriangleNormal(const NavTriangle& tri)
{
    NavPoint e1 = tri.v1 - tri.v0;
    NavPoint e2 = tri.v2 - tri.v0;
    return NavPoint::Cross(e1, e2);
}

} // namespace gx
