/// @file NavMesh3D.cpp
/// @brief NavMesh3D の実装
#include "pch_common.h"

#include "AI/NavMesh3D.h"
#include "Math/MathConvert.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Core/Logger.h"
#include <queue>

namespace gx
{

// ============================================================================
// Build
// ============================================================================
bool NavMesh3D::Build(const Vector3& minBounds, const Vector3& maxBounds, float voxelSize)
{
    if (voxelSize <= 0.0f)
    {
        GX_LOG_ERROR("NavMesh3D::Build - voxelSize must be > 0");
        return false;
    }

    if (maxBounds.x <= minBounds.x || maxBounds.y <= minBounds.y || maxBounds.z <= minBounds.z)
    {
        GX_LOG_ERROR("NavMesh3D::Build - invalid bounds");
        return false;
    }

    m_minBounds = minBounds;
    m_voxelSize = voxelSize;

    m_sizeX = static_cast<int>(std::ceil((maxBounds.x - minBounds.x) / voxelSize));
    m_sizeY = static_cast<int>(std::ceil((maxBounds.y - minBounds.y) / voxelSize));
    m_sizeZ = static_cast<int>(std::ceil((maxBounds.z - minBounds.z) / voxelSize));

    if (m_sizeX <= 0 || m_sizeY <= 0 || m_sizeZ <= 0)
    {
        GX_LOG_ERROR("NavMesh3D::Build - grid dimensions are zero");
        return false;
    }

    size_t totalVoxels = static_cast<size_t>(m_sizeX) * m_sizeY * m_sizeZ;
    m_voxels.assign(totalVoxels, VoxelState::Open);
    m_costs.assign(totalVoxels, 1.0f);
    m_offMeshLinks.clear();

    GX_LOG_INFO("NavMesh3D::Build - %dx%dx%d grid (voxelSize=%.2f, total=%zu)",
                m_sizeX, m_sizeY, m_sizeZ, voxelSize, totalVoxels);
    return true;
}

// ============================================================================
// BuildFromGeometry
// ============================================================================
bool NavMesh3D::BuildFromGeometry(const float* vertices, int vertexCount,
                                   const int* indices, int indexCount,
                                   float voxelSize)
{
    if (!vertices || vertexCount <= 0 || !indices || indexCount <= 0 || indexCount % 3 != 0)
    {
        GX_LOG_ERROR("NavMesh3D::BuildFromGeometry - invalid input");
        return false;
    }

    // 頂点データから範囲を決定
    float minX =  FLT_MAX, minY =  FLT_MAX, minZ =  FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < vertexCount; ++i)
    {
        float vx = vertices[i * 3 + 0];
        float vy = vertices[i * 3 + 1];
        float vz = vertices[i * 3 + 2];
        minX = (std::min)(minX, vx);
        minY = (std::min)(minY, vy);
        minZ = (std::min)(minZ, vz);
        maxX = (std::max)(maxX, vx);
        maxY = (std::max)(maxY, vy);
        maxZ = (std::max)(maxZ, vz);
    }

    // パディングを追加
    float pad = voxelSize * 0.5f;
    Vector3 bMin = { minX - pad, minY - pad, minZ - pad };
    Vector3 bMax = { maxX + pad, maxY + pad, maxZ + pad };

    if (!Build(bMin, bMax, voxelSize))
        return false;

    // 全ボクセルを初期状態でOpenに設定（Build内で実行済み）
    // 三角形をボクセルにラスタライズ — ジオメトリを含むボクセルをBlockedにする
    int triCount = indexCount / 3;
    for (int t = 0; t < triCount; ++t)
    {
        int i0 = indices[t * 3 + 0];
        int i1 = indices[t * 3 + 1];
        int i2 = indices[t * 3 + 2];

        if (i0 < 0 || i0 >= vertexCount || i1 < 0 || i1 >= vertexCount || i2 < 0 || i2 >= vertexCount)
            continue;

        Vector3 v0 = { vertices[i0 * 3 + 0], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2] };
        Vector3 v1 = { vertices[i1 * 3 + 0], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2] };
        Vector3 v2 = { vertices[i2 * 3 + 0], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2] };

        // 三角形の3D AABB
        float triMinX = (std::min)({ v0.x, v1.x, v2.x });
        float triMaxX = (std::max)({ v0.x, v1.x, v2.x });
        float triMinY = (std::min)({ v0.y, v1.y, v2.y });
        float triMaxY = (std::max)({ v0.y, v1.y, v2.y });
        float triMinZ = (std::min)({ v0.z, v1.z, v2.z });
        float triMaxZ = (std::max)({ v0.z, v1.z, v2.z });

        // この三角形のAABBをカバーするボクセル範囲
        int vxMin, vyMin, vzMin, vxMax, vyMax, vzMax;
        {
            int dummy;
            Vector3 pMin = { triMinX, triMinY, triMinZ };
            Vector3 pMax = { triMaxX, triMaxY, triMaxZ };
            WorldToVoxel(pMin, vxMin, vyMin, vzMin);
            WorldToVoxel(pMax, vxMax, vyMax, vzMax);
        }

        vxMin = (std::max)(0, vxMin);
        vyMin = (std::max)(0, vyMin);
        vzMin = (std::max)(0, vzMin);
        vxMax = (std::min)(m_sizeX - 1, vxMax);
        vyMax = (std::min)(m_sizeY - 1, vyMax);
        vzMax = (std::min)(m_sizeZ - 1, vzMax);

        // AABB-三角形オーバーラップテスト用の三角形エッジ（簡易版）
        XMVECTOR tv0 = XMLoadFloat3(XM(&v0));
        XMVECTOR tv1 = XMLoadFloat3(XM(&v1));
        XMVECTOR tv2 = XMLoadFloat3(XM(&v2));
        XMVECTOR e0 = XMVectorSubtract(tv1, tv0);
        XMVECTOR e1 = XMVectorSubtract(tv2, tv0);
        XMVECTOR triNormal = XMVector3Normalize(XMVector3Cross(e0, e1));

        for (int vz = vzMin; vz <= vzMax; ++vz)
        {
            for (int vy = vyMin; vy <= vyMax; ++vy)
            {
                for (int vx = vxMin; vx <= vxMax; ++vx)
                {
                    // ボクセル中心が三角形平面に近いか確認
                    Vector3 center = VoxelToWorld(vx, vy, vz);
                    XMVECTOR c = XMLoadFloat3(XM(&center));
                    XMVECTOR toCenter = XMVectorSubtract(c, tv0);
                    float dist;
                    XMStoreFloat(&dist, XMVector3Dot(toCenter, triNormal));

                    // ボクセル中心が三角形平面から半対角線以内なら、
                    // Blockedにする（三角形がボクセルと交差）
                    float halfDiag = m_voxelSize * 0.866f; // sqrt(3)/2
                    if (std::abs(dist) <= halfDiag)
                    {
                        // ボクセル中心を三角形平面に投影して近接を確認
                        XMVECTOR projected = XMVectorSubtract(c, XMVectorScale(triNormal, dist));

                        // 投影点での重心座標テスト
                        XMVECTOR v0p = XMVectorSubtract(projected, tv0);
                        float d00, d01, d11, d20, d21;
                        XMStoreFloat(&d00, XMVector3Dot(e0, e0));
                        XMStoreFloat(&d01, XMVector3Dot(e0, e1));
                        XMStoreFloat(&d11, XMVector3Dot(e1, e1));
                        XMStoreFloat(&d20, XMVector3Dot(v0p, e0));
                        XMStoreFloat(&d21, XMVector3Dot(v0p, e1));

                        float denom = d00 * d11 - d01 * d01;
                        if (std::abs(denom) < 1e-8f) continue;

                        float invDenom = 1.0f / denom;
                        float u = (d11 * d20 - d01 * d21) * invDenom;
                        float v = (d00 * d21 - d01 * d20) * invDenom;

                        // ボクセルカバレッジ用の許容値を使用
                        float tol = m_voxelSize / (std::max)(1e-4f, std::sqrt(denom)) * 0.5f;
                        tol = (std::min)(tol, 0.3f);

                        if (u >= -tol && v >= -tol && (u + v) <= 1.0f + tol)
                        {
                            int idx = VoxelIndex(vx, vy, vz);
                            if (idx >= 0)
                            {
                                m_voxels[idx] = VoxelState::Blocked;
                            }
                        }
                    }
                }
            }
        }
    }

    GX_LOG_INFO("NavMesh3D::BuildFromGeometry - %d tris -> %dx%dx%d grid",
                triCount, m_sizeX, m_sizeY, m_sizeZ);
    return true;
}

// ============================================================================
// SetVoxel / GetVoxel
// ============================================================================
void NavMesh3D::SetVoxel(int x, int y, int z, VoxelState state)
{
    int idx = VoxelIndex(x, y, z);
    if (idx < 0) return;
    m_voxels[idx] = state;
}

VoxelState NavMesh3D::GetVoxel(int x, int y, int z) const
{
    int idx = VoxelIndex(x, y, z);
    if (idx < 0) return VoxelState::Blocked;
    return m_voxels[idx];
}

// ============================================================================
// WorldToVoxel / VoxelToWorld
// ============================================================================
bool NavMesh3D::WorldToVoxel(const Vector3& worldPos, int& outX, int& outY, int& outZ) const
{
    outX = static_cast<int>(std::floor((worldPos.x - m_minBounds.x) / m_voxelSize));
    outY = static_cast<int>(std::floor((worldPos.y - m_minBounds.y) / m_voxelSize));
    outZ = static_cast<int>(std::floor((worldPos.z - m_minBounds.z) / m_voxelSize));
    return IsInBounds(outX, outY, outZ);
}

Vector3 NavMesh3D::VoxelToWorld(int x, int y, int z) const
{
    return {
        m_minBounds.x + (x + 0.5f) * m_voxelSize,
        m_minBounds.y + (y + 0.5f) * m_voxelSize,
        m_minBounds.z + (z + 0.5f) * m_voxelSize
    };
}

// ============================================================================
// SetVoxelCost
// ============================================================================
void NavMesh3D::SetVoxelCost(int x, int y, int z, float cost)
{
    int idx = VoxelIndex(x, y, z);
    if (idx < 0) return;
    m_costs[idx] = cost;
}

// ============================================================================
// FindPath（26接続の3D A*）
// ============================================================================
bool NavMesh3D::FindPath(const Vector3& start, const Vector3& end,
                          gx::Vector<Vector3>& path,
                          uint8_t allowedStates) const
{
    path.clear();

    if (m_voxels.empty()) return false;

    int sx, sy, sz, ex, ey, ez;
    WorldToVoxel(start, sx, sy, sz);
    WorldToVoxel(end, ex, ey, ez);

    // グリッドにクランプ
    sx = (std::max)(0, (std::min)(m_sizeX - 1, sx));
    sy = (std::max)(0, (std::min)(m_sizeY - 1, sy));
    sz = (std::max)(0, (std::min)(m_sizeZ - 1, sz));
    ex = (std::max)(0, (std::min)(m_sizeX - 1, ex));
    ey = (std::max)(0, (std::min)(m_sizeY - 1, ey));
    ez = (std::max)(0, (std::min)(m_sizeZ - 1, ez));

    // 開始/終了地点が通行可能か確認
    auto isAllowed = [&](int idx) -> bool
    {
        uint8_t stateBit = static_cast<uint8_t>(1 << static_cast<int>(m_voxels[idx]));
        return (allowedStates & stateBit) != 0;
    };

    int startIdx = VoxelIndex(sx, sy, sz);
    int endIdx = VoxelIndex(ex, ey, ez);
    if (startIdx < 0 || endIdx < 0) return false;
    if (!isAllowed(startIdx) || !isAllowed(endIdx)) return false;

    // 自明なケース
    if (sx == ex && sy == ey && sz == ez)
    {
        path.push_back(end);
        return true;
    }

    size_t totalSize = static_cast<size_t>(m_sizeX) * m_sizeY * m_sizeZ;

    // A*データ構造
    gx::Vector<bool> closed(totalSize, false);
    gx::Vector<float> gScore(totalSize, FLT_MAX);
    gx::Vector<int> parentIndex(totalSize, -1);

    struct AStarNode
    {
        int x, y, z;
        float f;
        bool operator>(const AStarNode& other) const { return f > other.f; }
    };

    auto heuristic = [&](int x, int y, int z) -> float
    {
        // ボクセルサイズでスケールされた3Dマンハッタン距離
        return (std::abs(x - ex) + std::abs(y - ey) + std::abs(z - ez)) * m_voxelSize;
    };

    gx::PriorityQueue<AStarNode, gx::Deque<AStarNode>, std::greater<AStarNode>> openList;

    gScore[startIdx] = 0.0f;
    float startH = heuristic(sx, sy, sz);
    openList.push({ sx, sy, sz, startH });

    // 26接続: 対角線を含む全隣接オフセット
    // y=-1層（9エントリ）、y=0層（中心を除く8エントリ）、y=+1層（9エントリ）= 26
    static const int dx[26] = {
        // y=-1: z varies -1,0,1 for each x=-1,0,1
        -1,  0,  1, -1,  0,  1, -1,  0,  1,
        // y=0: skip (0,0,0)
        -1,  0,  1, -1,  1, -1,  0,  1,
        // y=+1
        -1,  0,  1, -1,  0,  1, -1,  0,  1
    };
    static const int dy[26] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1,
         0,  0,  0,  0,  0,  0,  0,  0,
         1,  1,  1,  1,  1,  1,  1,  1,  1
    };
    static const int dz[26] = {
        -1, -1, -1,  0,  0,  0,  1,  1,  1,
        -1, -1, -1,  0,  0,  1,  1,  1,
        -1, -1, -1,  0,  0,  0,  1,  1,  1
    };

    // 移動コスト（ユークリッド距離）を事前計算
    float moveCost[26];
    for (int d = 0; d < 26; ++d)
    {
        int ax = std::abs(dx[d]) + std::abs(dy[d]) + std::abs(dz[d]);
        if (ax == 1)      moveCost[d] = m_voxelSize;           // 面隣接
        else if (ax == 2) moveCost[d] = m_voxelSize * 1.414f;  // 辺隣接
        else              moveCost[d] = m_voxelSize * 1.732f;  // 角隣接
    }

    bool found = false;

    while (!openList.empty())
    {
        AStarNode current = openList.top();
        openList.pop();

        int curIdx = VoxelIndex(current.x, current.y, current.z);

        if (closed[curIdx]) continue;
        closed[curIdx] = true;

        if (current.x == ex && current.y == ey && current.z == ez)
        {
            found = true;
            break;
        }

        // 26隣接を探索
        for (int d = 0; d < 26; ++d)
        {
            int nx = current.x + dx[d];
            int ny = current.y + dy[d];
            int nz = current.z + dz[d];

            if (!IsInBounds(nx, ny, nz)) continue;

            int nIdx = VoxelIndex(nx, ny, nz);
            if (closed[nIdx]) continue;
            if (!isAllowed(nIdx)) continue;

            float stepCost = moveCost[d] * m_costs[nIdx];
            float tentativeG = gScore[curIdx] + stepCost;

            if (tentativeG < gScore[nIdx])
            {
                gScore[nIdx] = tentativeG;
                parentIndex[nIdx] = curIdx;

                float h = heuristic(nx, ny, nz);
                openList.push({ nx, ny, nz, tentativeG + h });
            }
        }

        // 現在のボクセルからのOff-meshリンクも確認
        Vector3 curWorld = VoxelToWorld(current.x, current.y, current.z);
        float linkSnapDist = m_voxelSize * 0.75f;
        float linkSnapDist2 = linkSnapDist * linkSnapDist;

        for (const auto& link : m_offMeshLinks)
        {
            // 現在のボクセルがリンク始点の近くか確認
            float dsx = curWorld.x - link.start.x;
            float dsy = curWorld.y - link.start.y;
            float dsz = curWorld.z - link.start.z;
            bool nearStart = (dsx * dsx + dsy * dsy + dsz * dsz) <= linkSnapDist2;

            float dex = curWorld.x - link.end.x;
            float dey = curWorld.y - link.end.y;
            float dez = curWorld.z - link.end.z;
            bool nearEnd = link.bidirectional && (dex * dex + dey * dey + dez * dez) <= linkSnapDist2;

            Vector3 targetWorld;
            if (nearStart)
                targetWorld = link.end;
            else if (nearEnd)
                targetWorld = link.start;
            else
                continue;

            int tx, ty, tz;
            if (!WorldToVoxel(targetWorld, tx, ty, tz)) continue;
            if (!IsInBounds(tx, ty, tz)) continue;

            int tIdx = VoxelIndex(tx, ty, tz);
            if (closed[tIdx]) continue;

            float tentativeG = gScore[curIdx] + link.cost;
            if (tentativeG < gScore[tIdx])
            {
                gScore[tIdx] = tentativeG;
                parentIndex[tIdx] = curIdx;

                float h = heuristic(tx, ty, tz);
                openList.push({ tx, ty, tz, tentativeG + h });
            }
        }
    }

    if (!found) return false;

    // パスを再構築
    gx::Vector<Vector3> reversePath;
    int ci = endIdx;
    while (ci >= 0)
    {
        // フラットインデックスをx,y,zに戻す
        int iz = ci / (m_sizeX * m_sizeY);
        int remainder = ci % (m_sizeX * m_sizeY);
        int iy = remainder / m_sizeX;
        int ix = remainder % m_sizeX;
        reversePath.push_back(VoxelToWorld(ix, iy, iz));
        ci = parentIndex[ci];
    }

    path.reserve(reversePath.size());
    for (int i = static_cast<int>(reversePath.size()) - 1; i >= 0; --i)
        path.push_back(reversePath[i]);

    // 最初と最後を正確な開始/終了位置に置き換え
    if (!path.empty())
    {
        path.front() = start;
        path.back() = end;
    }

    return true;
}

// ============================================================================
// AddOffMeshLink / RemoveOffMeshLink
// ============================================================================
uint32_t NavMesh3D::AddOffMeshLink(const OffMeshLink& link)
{
    m_offMeshLinks.push_back(link);
    return static_cast<uint32_t>(m_offMeshLinks.size() - 1);
}

void NavMesh3D::RemoveOffMeshLink(uint32_t index)
{
    if (index < m_offMeshLinks.size())
    {
        m_offMeshLinks.erase(m_offMeshLinks.begin() + index);
    }
}

// ============================================================================
// FindNearestOpen（外向きBFS）
// ============================================================================
Vector3 NavMesh3D::FindNearestOpen(const Vector3& pos) const
{
    if (m_voxels.empty()) return pos;

    int cx, cy, cz;
    WorldToVoxel(pos, cx, cy, cz);

    // グリッドにクランプ
    cx = (std::max)(0, (std::min)(m_sizeX - 1, cx));
    cy = (std::max)(0, (std::min)(m_sizeY - 1, cy));
    cz = (std::max)(0, (std::min)(m_sizeZ - 1, cz));

    // 現在のボクセルが既にOpenか確認
    int idx = VoxelIndex(cx, cy, cz);
    if (idx >= 0 && m_voxels[idx] != VoxelState::Blocked)
    {
        return VoxelToWorld(cx, cy, cz);
    }

    // 開始ボクセルから外向きにBFS
    struct BFSNode { int x, y, z; };
    gx::Queue<BFSNode> frontier;
    size_t totalSize = static_cast<size_t>(m_sizeX) * m_sizeY * m_sizeZ;
    gx::Vector<bool> visited(totalSize, false);

    frontier.push({ cx, cy, cz });
    if (idx >= 0) visited[idx] = true;

    // BFS用の6接続（効率のため面隣接のみ）
    static const int dx[] = { -1, 1, 0, 0, 0, 0 };
    static const int dy[] = { 0, 0, -1, 1, 0, 0 };
    static const int dz[] = { 0, 0, 0, 0, -1, 1 };

    while (!frontier.empty())
    {
        BFSNode node = frontier.front();
        frontier.pop();

        for (int d = 0; d < 6; ++d)
        {
            int nx = node.x + dx[d];
            int ny = node.y + dy[d];
            int nz = node.z + dz[d];

            if (!IsInBounds(nx, ny, nz)) continue;

            int nIdx = VoxelIndex(nx, ny, nz);
            if (nIdx < 0 || visited[nIdx]) continue;
            visited[nIdx] = true;

            if (m_voxels[nIdx] != VoxelState::Blocked)
            {
                return VoxelToWorld(nx, ny, nz);
            }

            frontier.push({ nx, ny, nz });
        }
    }

    // Openボクセルが見つからない場合、元の位置を返す
    return pos;
}

// ============================================================================
// Raycast (3D DDA)
// ============================================================================
bool NavMesh3D::Raycast(const Vector3& origin, const Vector3& direction, float maxDist,
                         Vector3& hitPos) const
{
    if (m_voxels.empty()) return false;

    // 方向を正規化
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (len < 1e-8f) return false;
    float invLen = 1.0f / len;
    float dx = direction.x * invLen;
    float dy = direction.y * invLen;
    float dz = direction.z * invLen;

    // 開始ボクセル
    int vx = static_cast<int>(std::floor((origin.x - m_minBounds.x) / m_voxelSize));
    int vy = static_cast<int>(std::floor((origin.y - m_minBounds.y) / m_voxelSize));
    int vz = static_cast<int>(std::floor((origin.z - m_minBounds.z) / m_voxelSize));

    // ステップ方向
    int stepX = (dx >= 0) ? 1 : -1;
    int stepY = (dy >= 0) ? 1 : -1;
    int stepZ = (dz >= 0) ? 1 : -1;

    // tMax: 各軸で次のボクセル境界までのレイ沿いの距離
    // tDelta: 各軸で1ボクセル分を横断するのに必要なレイ沿いの距離
    float tMaxX, tMaxY, tMaxZ;
    float tDeltaX, tDeltaY, tDeltaZ;

    if (std::abs(dx) > 1e-8f)
    {
        float boundary = m_minBounds.x + ((dx >= 0) ? (vx + 1) : vx) * m_voxelSize;
        tMaxX = (boundary - origin.x) / dx;
        tDeltaX = m_voxelSize / std::abs(dx);
    }
    else
    {
        tMaxX = FLT_MAX;
        tDeltaX = FLT_MAX;
    }

    if (std::abs(dy) > 1e-8f)
    {
        float boundary = m_minBounds.y + ((dy >= 0) ? (vy + 1) : vy) * m_voxelSize;
        tMaxY = (boundary - origin.y) / dy;
        tDeltaY = m_voxelSize / std::abs(dy);
    }
    else
    {
        tMaxY = FLT_MAX;
        tDeltaY = FLT_MAX;
    }

    if (std::abs(dz) > 1e-8f)
    {
        float boundary = m_minBounds.z + ((dz >= 0) ? (vz + 1) : vz) * m_voxelSize;
        tMaxZ = (boundary - origin.z) / dz;
        tDeltaZ = m_voxelSize / std::abs(dz);
    }
    else
    {
        tMaxZ = FLT_MAX;
        tDeltaZ = FLT_MAX;
    }

    float t = 0.0f;

    while (t <= maxDist)
    {
        // 現在のボクセルを確認
        if (IsInBounds(vx, vy, vz))
        {
            int idx = VoxelIndex(vx, vy, vz);
            if (idx >= 0 && m_voxels[idx] == VoxelState::Blocked)
            {
                hitPos = {
                    origin.x + dx * t,
                    origin.y + dy * t,
                    origin.z + dz * t
                };
                return true;
            }
        }
        else
        {
            // レイがグリッドを離れた
            break;
        }

        // 次のボクセル境界に進む
        if (tMaxX < tMaxY)
        {
            if (tMaxX < tMaxZ)
            {
                t = tMaxX;
                vx += stepX;
                tMaxX += tDeltaX;
            }
            else
            {
                t = tMaxZ;
                vz += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        else
        {
            if (tMaxY < tMaxZ)
            {
                t = tMaxY;
                vy += stepY;
                tMaxY += tDeltaY;
            }
            else
            {
                t = tMaxZ;
                vz += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
    }

    return false;
}

// ============================================================================
// DebugDraw
// ============================================================================
void NavMesh3D::DebugDraw(PrimitiveBatch3D& batch, bool showBlocked) const
{
    if (m_voxels.empty()) return;

    const Vector4 openColor    = { 0.1f, 0.8f, 0.2f, 0.3f };
    const Vector4 blockedColor = { 0.9f, 0.15f, 0.1f, 0.3f };
    const Vector4 waterColor   = { 0.1f, 0.3f, 0.9f, 0.3f };
    const Vector4 airColor     = { 0.7f, 0.7f, 0.9f, 0.2f };

    float half = m_voxelSize * 0.45f;

    // サーフェスボクセルのみ描画（少なくとも1つのBlocked隣接またはグリッド境界を持つボクセル）
    for (int z = 0; z < m_sizeZ; ++z)
    {
        for (int y = 0; y < m_sizeY; ++y)
        {
            for (int x = 0; x < m_sizeX; ++x)
            {
                VoxelState state = m_voxels[VoxelIndex(x, y, z)];
                if (state == VoxelState::Blocked && !showBlocked) continue;

                // サーフェスボクセル判定: 面隣接にBlockedまたは範囲外があるか？
                bool isSurface = false;
                static const int fd[][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
                for (int d = 0; d < 6; ++d)
                {
                    int nx = x + fd[d][0];
                    int ny = y + fd[d][1];
                    int nz = z + fd[d][2];
                    if (!IsInBounds(nx, ny, nz))
                    {
                        isSurface = true;
                        break;
                    }
                    if (m_voxels[VoxelIndex(nx, ny, nz)] != state)
                    {
                        isSurface = true;
                        break;
                    }
                }

                if (!isSurface) continue;

                Vector3 center = VoxelToWorld(x, y, z);
                const Vector4* color = &openColor;
                if (state == VoxelState::Blocked) color = &blockedColor;
                else if (state == VoxelState::Water) color = &waterColor;
                else if (state == VoxelState::Air) color = &airColor;

                // ワイヤーフレームボックスを描画（12辺）
                Vector3 corners[8] = {
                    { center.x - half, center.y - half, center.z - half },
                    { center.x + half, center.y - half, center.z - half },
                    { center.x + half, center.y - half, center.z + half },
                    { center.x - half, center.y - half, center.z + half },
                    { center.x - half, center.y + half, center.z - half },
                    { center.x + half, center.y + half, center.z - half },
                    { center.x + half, center.y + half, center.z + half },
                    { center.x - half, center.y + half, center.z + half },
                };

                // 底面
                batch.DrawLine(corners[0], corners[1], *color);
                batch.DrawLine(corners[1], corners[2], *color);
                batch.DrawLine(corners[2], corners[3], *color);
                batch.DrawLine(corners[3], corners[0], *color);
                // 上面
                batch.DrawLine(corners[4], corners[5], *color);
                batch.DrawLine(corners[5], corners[6], *color);
                batch.DrawLine(corners[6], corners[7], *color);
                batch.DrawLine(corners[7], corners[4], *color);
                // 垂直辺
                batch.DrawLine(corners[0], corners[4], *color);
                batch.DrawLine(corners[1], corners[5], *color);
                batch.DrawLine(corners[2], corners[6], *color);
                batch.DrawLine(corners[3], corners[7], *color);
            }
        }
    }
}

// ============================================================================
// GetOpenVoxelCount
// ============================================================================
uint32_t NavMesh3D::GetOpenVoxelCount() const
{
    uint32_t count = 0;
    for (auto state : m_voxels)
    {
        if (state != VoxelState::Blocked)
            ++count;
    }
    return count;
}

// ============================================================================
// Clear
// ============================================================================
void NavMesh3D::Clear()
{
    m_voxels.clear();
    m_costs.clear();
    m_offMeshLinks.clear();
    m_sizeX = m_sizeY = m_sizeZ = 0;
    m_voxelSize = 1.0f;
    m_minBounds = { 0, 0, 0 };
}

// ============================================================================
// プライベートヘルパー
// ============================================================================
int NavMesh3D::VoxelIndex(int x, int y, int z) const
{
    if (!IsInBounds(x, y, z)) return -1;
    return z * (m_sizeX * m_sizeY) + y * m_sizeX + x;
}

float NavMesh3D::GetCost(int x, int y, int z) const
{
    int idx = VoxelIndex(x, y, z);
    if (idx < 0) return FLT_MAX;
    return m_costs[idx];
}

bool NavMesh3D::IsInBounds(int x, int y, int z) const
{
    return x >= 0 && x < m_sizeX &&
           y >= 0 && y < m_sizeY &&
           z >= 0 && z < m_sizeZ;
}

} // namespace gx
