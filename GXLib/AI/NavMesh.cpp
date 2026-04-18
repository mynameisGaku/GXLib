#include "pch_common.h"
/// @file NavMesh.cpp
/// @brief Grid-based navigation mesh with A* pathfinding.
///
/// ADR-0018 §Constraints: AI module links only GXLib_Foundation. Do NOT add
/// any `Graphics/` include here. Graphics-dependent helpers
/// (BuildFromTerrain, DebugDraw, DebugDrawPath) live in
/// `GXLib/AI/Debug/NavMeshDebug.cpp` under the sibling `GXLib_AIDebug`
/// static lib.

#include "AI/NavMesh.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"
#include <queue>

namespace gx
{

// ============================================================================
// Build (flat grid)
// ============================================================================
bool NavMesh::Build(float worldMinX, float worldMinZ,
                    float worldMaxX, float worldMaxZ,
                    float cellSize, float maxClimb, float maxSlope)
{
    if (cellSize <= 0.0f)
    {
        Logger::Error("NavMesh::Build - cellSize must be > 0");
        return false;
    }
    if (worldMaxX <= worldMinX || worldMaxZ <= worldMinZ)
    {
        Logger::Error("NavMesh::Build - invalid world bounds");
        return false;
    }

    m_cellSize  = cellSize;
    m_worldMinX = worldMinX;
    m_worldMinZ = worldMinZ;

    m_gridWidth  = static_cast<int>(std::ceil((worldMaxX - worldMinX) / cellSize));
    m_gridHeight = static_cast<int>(std::ceil((worldMaxZ - worldMinZ) / cellSize));

    if (m_gridWidth <= 0 || m_gridHeight <= 0)
    {
        Logger::Error("NavMesh::Build - grid dimensions are zero");
        return false;
    }

    m_grid.resize(static_cast<size_t>(m_gridWidth) * m_gridHeight);

    // Initialize all cells as walkable with height 0
    for (auto& cell : m_grid)
    {
        cell.height         = 0.0f;
        cell.walkable       = true;
        cell.costMultiplier = 1.0f;
    }

    m_baseGrid = m_grid; // save base state for obstacle restore
    m_built = true;
    Logger::Info("NavMesh::Build - %dx%d grid (cellSize=%.2f)", m_gridWidth, m_gridHeight, cellSize);
    return true;
}

// ============================================================================
// BuildFromTerrain — moved to GXLib/AI/Debug/NavMeshDebug.cpp (2026-04-18, E1)
// ============================================================================

// ============================================================================
// BuildFromGeometry
// ============================================================================
bool NavMesh::BuildFromGeometry(const float* vertices, int vertexCount,
                                const int* indices, int indexCount,
                                float maxClimb, float maxSlope)
{
    if (!vertices || vertexCount <= 0 || !indices || indexCount <= 0 || indexCount % 3 != 0)
    {
        Logger::Error("NavMesh::BuildFromGeometry - invalid input");
        return false;
    }

    // Determine world bounds from vertex data
    float minX =  FLT_MAX, minZ =  FLT_MAX;
    float maxX = -FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < vertexCount; ++i)
    {
        float vx = vertices[i * 3 + 0];
        float vz = vertices[i * 3 + 2];
        minX = std::min(minX, vx);
        maxX = std::max(maxX, vx);
        minZ = std::min(minZ, vz);
        maxZ = std::max(maxZ, vz);
    }

    // Add small padding
    float pad = m_cellSize * 0.5f;
    if (!Build(minX - pad, minZ - pad, maxX + pad, maxZ + pad, m_cellSize, maxClimb, maxSlope))
        return false;

    // Mark all cells as unwalkable initially, then stamp triangles
    for (auto& cell : m_grid)
        cell.walkable = false;

    // Rasterize each triangle: for each cell overlapping the triangle's XZ AABB,
    // project and set height + walkable
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

        // Triangle XZ AABB
        float triMinX = std::min({ v0.x, v1.x, v2.x });
        float triMaxX = std::max({ v0.x, v1.x, v2.x });
        float triMinZ = std::min({ v0.z, v1.z, v2.z });
        float triMaxZ = std::max({ v0.z, v1.z, v2.z });

        // Cell range
        int cxMin, czMin, cxMax, czMax;
        WorldToCell(triMinX, triMinZ, cxMin, czMin);
        WorldToCell(triMaxX, triMaxZ, cxMax, czMax);

        cxMin = std::max(0, cxMin);
        czMin = std::max(0, czMin);
        cxMax = std::min(m_gridWidth - 1, cxMax);
        czMax = std::min(m_gridHeight - 1, czMax);

        // Compute triangle normal for slope check
        XMVECTOR e1 = XMVectorSubtract(XMLoadFloat3(XM(&v1)), XMLoadFloat3(XM(&v0)));
        XMVECTOR e2 = XMVectorSubtract(XMLoadFloat3(XM(&v2)), XMLoadFloat3(XM(&v0)));
        XMVECTOR normal = XMVector3Normalize(XMVector3Cross(e1, e2));
        Vector3 n;
        XMStoreFloat3(XM(&n), normal);

        // Slope angle from normal (angle between normal and up vector)
        float slopeAngle = std::acos(std::max(0.0f, std::min(1.0f, std::abs(n.y))))
                           * (180.0f / XM_PI);

        for (int cz = czMin; cz <= czMax; ++cz)
        {
            for (int cx = cxMin; cx <= cxMax; ++cx)
            {
                Vector3 cellWorld = CellToWorld(cx, cz);

                // Barycentric test to check if cell center is inside triangle (XZ projection)
                float dx0 = v1.x - v0.x, dz0 = v1.z - v0.z;
                float dx1 = v2.x - v0.x, dz1 = v2.z - v0.z;
                float dx2 = cellWorld.x - v0.x, dz2 = cellWorld.z - v0.z;

                float d00 = dx0 * dx0 + dz0 * dz0;
                float d01 = dx0 * dx1 + dz0 * dz1;
                float d11 = dx1 * dx1 + dz1 * dz1;
                float d20 = dx2 * dx0 + dz2 * dz0;
                float d21 = dx2 * dx1 + dz2 * dz1;

                float denom = d00 * d11 - d01 * d01;
                if (std::abs(denom) < 1e-8f) continue;

                float invDenom = 1.0f / denom;
                float u = (d11 * d20 - d01 * d21) * invDenom;
                float v = (d00 * d21 - d01 * d20) * invDenom;

                // Cell center inside triangle (with small tolerance for cell coverage)
                if (u >= -0.1f && v >= -0.1f && (u + v) <= 1.1f)
                {
                    // Interpolate height
                    float baryU = std::max(0.0f, std::min(1.0f, u));
                    float baryV = std::max(0.0f, std::min(1.0f, v));
                    float baryW = std::max(0.0f, 1.0f - baryU - baryV);
                    float h = baryW * v0.y + baryU * v1.y + baryV * v2.y;

                    auto& cell = m_grid[static_cast<size_t>(cz) * m_gridWidth + cx];
                    // Keep the highest surface
                    if (!cell.walkable || h > cell.height)
                    {
                        cell.height   = h;
                        cell.walkable = (slopeAngle <= maxSlope);
                    }
                }
            }
        }
    }

    // Apply slope/climb filter on the final height grid
    ApplySlopeFilter(maxClimb, maxSlope);

    Logger::Info("NavMesh::BuildFromGeometry - %d tris -> %dx%d grid",
                 triCount, m_gridWidth, m_gridHeight);
    return true;
}

// ============================================================================
// ApplySlopeFilter
// ============================================================================
void NavMesh::ApplySlopeFilter(float maxClimb, float maxSlope)
{
    // Convert maxSlope to radians for height difference calculation
    float maxSlopeRad = maxSlope * (XM_PI / 180.0f);

    // 8-directional neighbours
    static const int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static const int dz[] = { -1, -1, -1, 0, 0, 1, 1, 1 };

    for (int z = 0; z < m_gridHeight; ++z)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            auto& cell = m_grid[static_cast<size_t>(z) * m_gridWidth + x];
            if (!cell.walkable) continue;

            // Check all 8 neighbours: if ANY adjacent walkable cell
            // has too much height difference, check if this cell is reachable
            bool tooSteep = false;
            int walkableNeighbours = 0;

            for (int d = 0; d < 8; ++d)
            {
                int nx = x + dx[d];
                int nz = z + dz[d];
                if (nx < 0 || nx >= m_gridWidth || nz < 0 || nz >= m_gridHeight)
                    continue;

                auto& neighbour = m_grid[static_cast<size_t>(nz) * m_gridWidth + nx];
                float heightDiff = std::abs(cell.height - neighbour.height);

                // Check step height (maxClimb) and slope
                float dist = (dx[d] != 0 && dz[d] != 0) ? (m_cellSize * 1.414f) : m_cellSize;
                float slopeH = std::tan(maxSlopeRad) * dist;

                if (heightDiff > maxClimb || heightDiff > slopeH)
                {
                    tooSteep = true;
                }
                else
                {
                    ++walkableNeighbours;
                }
            }

            // If no walkable neighbours due to steep slopes, mark as unwalkable
            if (tooSteep && walkableNeighbours == 0)
                cell.walkable = false;
        }
    }
}

// ============================================================================
// SetCellWalkable / SetCellCost
// ============================================================================
void NavMesh::SetCellWalkable(int cellX, int cellZ, bool walkable)
{
    if (!m_built) return;
    if (cellX < 0 || cellX >= m_gridWidth || cellZ < 0 || cellZ >= m_gridHeight) return;
    m_grid[static_cast<size_t>(cellZ) * m_gridWidth + cellX].walkable = walkable;
}

void NavMesh::SetCellCost(int cellX, int cellZ, float costMultiplier)
{
    if (!m_built) return;
    if (cellX < 0 || cellX >= m_gridWidth || cellZ < 0 || cellZ >= m_gridHeight) return;
    m_grid[static_cast<size_t>(cellZ) * m_gridWidth + cellX].costMultiplier = costMultiplier;
}

// ============================================================================
// FindPath (A*)
// ============================================================================
bool NavMesh::FindPath(const Vector3& start, const Vector3& end,
                       gx::Vector<Vector3>& path, bool smooth) const
{
    path.clear();
    if (!m_built) return false;

    int sx, sz, ex, ez;
    WorldToCell(start.x, start.z, sx, sz);
    WorldToCell(end.x,   end.z,   ex, ez);

    // Clamp to grid
    sx = std::max(0, std::min(m_gridWidth - 1, sx));
    sz = std::max(0, std::min(m_gridHeight - 1, sz));
    ex = std::max(0, std::min(m_gridWidth - 1, ex));
    ez = std::max(0, std::min(m_gridHeight - 1, ez));

    // Check start/end walkable
    if (!m_grid[static_cast<size_t>(sz) * m_gridWidth + sx].walkable)
        return false;
    if (!m_grid[static_cast<size_t>(ez) * m_gridWidth + ex].walkable)
        return false;

    // Trivial case
    if (sx == ex && sz == ez)
    {
        path.push_back(end);
        return true;
    }

    size_t gridSize = static_cast<size_t>(m_gridWidth) * m_gridHeight;

    // Closed set + g-scores + parent tracking
    gx::Vector<bool>  closed(gridSize, false);
    gx::Vector<float> gScore(gridSize, FLT_MAX);
    gx::Vector<int>   parentX(gridSize, -1);
    gx::Vector<int>   parentZ(gridSize, -1);

    // Min-heap priority queue
    auto cmp = [](const AStarNode& a, const AStarNode& b) { return a.f > b.f; };
    gx::PriorityQueue<AStarNode, gx::Deque<AStarNode>, decltype(cmp)> openList(cmp);

    // Start node
    size_t startIdx = static_cast<size_t>(sz) * m_gridWidth + sx;
    gScore[startIdx] = 0.0f;
    float startH = Heuristic(sx, sz, ex, ez);
    openList.push({ sx, sz, 0.0f, startH, startH, -1, -1 });

    // 8-directional movement
    static const int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static const int dz[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    static const float moveCost[] = { 1.414f, 1.0f, 1.414f, 1.0f, 1.0f, 1.414f, 1.0f, 1.414f };

    bool found = false;

    while (!openList.empty())
    {
        AStarNode current = openList.top();
        openList.pop();

        size_t curIdx = static_cast<size_t>(current.z) * m_gridWidth + current.x;

        // Skip if already processed
        if (closed[curIdx]) continue;
        closed[curIdx] = true;

        // Goal reached
        if (current.x == ex && current.z == ez)
        {
            found = true;
            break;
        }

        // Explore neighbours
        for (int d = 0; d < 8; ++d)
        {
            int nx = current.x + dx[d];
            int nz = current.z + dz[d];

            if (nx < 0 || nx >= m_gridWidth || nz < 0 || nz >= m_gridHeight)
                continue;

            size_t nIdx = static_cast<size_t>(nz) * m_gridWidth + nx;
            if (closed[nIdx]) continue;

            const Cell& nCell = m_grid[nIdx];
            if (!nCell.walkable) continue;

            // For diagonal moves, check that both adjacent cardinal cells are walkable
            // to prevent cutting corners around obstacles
            if (dx[d] != 0 && dz[d] != 0)
            {
                size_t adjX = static_cast<size_t>(current.z) * m_gridWidth + nx;
                size_t adjZ = static_cast<size_t>(nz) * m_gridWidth + current.x;
                if (!m_grid[adjX].walkable || !m_grid[adjZ].walkable)
                    continue;
            }

            float stepCost = moveCost[d] * nCell.costMultiplier;
            float tentativeG = gScore[curIdx] + stepCost;

            if (tentativeG < gScore[nIdx])
            {
                gScore[nIdx]  = tentativeG;
                parentX[nIdx] = current.x;
                parentZ[nIdx] = current.z;

                float h = Heuristic(nx, nz, ex, ez);
                openList.push({ nx, nz, tentativeG, h, tentativeG + h, current.x, current.z });
            }
        }
    }

    if (!found) return false;

    // Reconstruct path (from end to start)
    gx::Vector<Vector3> reversePath;
    int cx = ex, cz = ez;
    while (cx != -1 && cz != -1)
    {
        reversePath.push_back(CellToWorld(cx, cz));
        size_t idx = static_cast<size_t>(cz) * m_gridWidth + cx;
        int px = parentX[idx];
        int pz = parentZ[idx];
        cx = px;
        cz = pz;
    }

    // Reverse to get start->end order
    path.reserve(reversePath.size());
    for (int i = static_cast<int>(reversePath.size()) - 1; i >= 0; --i)
        path.push_back(reversePath[i]);

    // Set the first waypoint's height to start height and last to end height
    if (!path.empty())
    {
        path.front().y = start.y;
        path.back().y  = end.y;
    }

    return true;
}

// ============================================================================
// FindNearestWalkable (spiral search)
// ============================================================================
bool NavMesh::FindNearestWalkable(const Vector3& position, Vector3& nearest) const
{
    if (!m_built) return false;

    int cx, cz;
    WorldToCell(position.x, position.z, cx, cz);

    // If the cell itself is walkable, return it
    if (cx >= 0 && cx < m_gridWidth && cz >= 0 && cz < m_gridHeight)
    {
        if (m_grid[static_cast<size_t>(cz) * m_gridWidth + cx].walkable)
        {
            nearest = CellToWorld(cx, cz);
            return true;
        }
    }

    // Spiral outward search
    int maxRadius = std::max(m_gridWidth, m_gridHeight);
    for (int r = 1; r <= maxRadius; ++r)
    {
        for (int dz = -r; dz <= r; ++dz)
        {
            for (int dx = -r; dx <= r; ++dx)
            {
                // Only check border of current ring
                if (std::abs(dx) != r && std::abs(dz) != r) continue;

                int nx = cx + dx;
                int nz = cz + dz;
                if (nx < 0 || nx >= m_gridWidth || nz < 0 || nz >= m_gridHeight)
                    continue;

                if (m_grid[static_cast<size_t>(nz) * m_gridWidth + nx].walkable)
                {
                    nearest = CellToWorld(nx, nz);
                    return true;
                }
            }
        }
    }

    return false;
}

// ============================================================================
// IsWalkable
// ============================================================================
bool NavMesh::IsWalkable(const Vector3& position) const
{
    if (!m_built) return false;

    int cx, cz;
    WorldToCell(position.x, position.z, cx, cz);

    if (cx < 0 || cx >= m_gridWidth || cz < 0 || cz >= m_gridHeight)
        return false;

    return m_grid[static_cast<size_t>(cz) * m_gridWidth + cx].walkable;
}

// ============================================================================
// DebugDraw / DebugDrawPath — moved to GXLib/AI/Debug/NavMeshDebug.cpp
// (2026-04-18, E1)
// ============================================================================

// ============================================================================
// WorldToCell / CellToWorld / Heuristic
// ============================================================================
void NavMesh::WorldToCell(float worldX, float worldZ, int& cellX, int& cellZ) const
{
    cellX = static_cast<int>(std::floor((worldX - m_worldMinX) / m_cellSize));
    cellZ = static_cast<int>(std::floor((worldZ - m_worldMinZ) / m_cellSize));
}

Vector3 NavMesh::CellToWorld(int cellX, int cellZ) const
{
    float wx = m_worldMinX + (cellX + 0.5f) * m_cellSize;
    float wz = m_worldMinZ + (cellZ + 0.5f) * m_cellSize;
    float h  = 0.0f;
    if (cellX >= 0 && cellX < m_gridWidth && cellZ >= 0 && cellZ < m_gridHeight)
        h = m_grid[static_cast<size_t>(cellZ) * m_gridWidth + cellX].height;
    return { wx, h, wz };
}

float NavMesh::Heuristic(int x1, int z1, int x2, int z2)
{
    // Octile distance (diagonal heuristic for 8-directional movement)
    int dx = std::abs(x2 - x1);
    int dz = std::abs(z2 - z1);
    int mn = std::min(dx, dz);
    int mx = std::max(dx, dz);
    return static_cast<float>(mx) + 0.414f * static_cast<float>(mn);
}

// ============================================================================
// Dynamic Obstacles
// ============================================================================

uint32_t NavMesh::AddObstacleAABB(float minX, float minZ, float maxX, float maxZ)
{
    Obstacle obs;
    obs.type = ObstacleType::AABB;
    obs.minX = minX; obs.minZ = minZ; obs.maxX = maxX; obs.maxZ = maxZ;
    obs.centerX = obs.centerZ = obs.radius = 0.0f;
    obs.handle = m_nextObstacleHandle++;
    m_obstacles.push_back(obs);
    ApplyObstacles();
    return obs.handle;
}

uint32_t NavMesh::AddObstacleCylinder(float centerX, float centerZ, float radius)
{
    Obstacle obs;
    obs.type = ObstacleType::Cylinder;
    obs.centerX = centerX; obs.centerZ = centerZ; obs.radius = radius;
    obs.minX = centerX - radius; obs.minZ = centerZ - radius;
    obs.maxX = centerX + radius; obs.maxZ = centerZ + radius;
    obs.handle = m_nextObstacleHandle++;
    m_obstacles.push_back(obs);
    ApplyObstacles();
    return obs.handle;
}

void NavMesh::RemoveObstacle(uint32_t handle)
{
    m_obstacles.erase(
        std::remove_if(m_obstacles.begin(), m_obstacles.end(),
            [handle](const Obstacle& o) { return o.handle == handle; }),
        m_obstacles.end());
    ApplyObstacles();
}

void NavMesh::ClearObstacles()
{
    m_obstacles.clear();
    ApplyObstacles();
}

void NavMesh::MoveObstacle(uint32_t handle, float newMinX, float newMinZ)
{
    for (auto& obs : m_obstacles)
    {
        if (obs.handle == handle)
        {
            float width = obs.maxX - obs.minX;
            float depth = obs.maxZ - obs.minZ;
            obs.minX = newMinX; obs.minZ = newMinZ;
            obs.maxX = newMinX + width; obs.maxZ = newMinZ + depth;
            if (obs.type == ObstacleType::Cylinder)
            {
                obs.centerX = newMinX + obs.radius;
                obs.centerZ = newMinZ + obs.radius;
            }
            break;
        }
    }
    ApplyObstacles();
}

void NavMesh::ApplyObstacles()
{
    if (m_baseGrid.empty()) return;
    m_grid = m_baseGrid; // restore base

    for (const auto& obs : m_obstacles)
    {
        // Determine which cells this obstacle covers
        int minCX, minCZ, maxCX, maxCZ;
        WorldToCell(obs.minX, obs.minZ, minCX, minCZ);
        WorldToCell(obs.maxX, obs.maxZ, maxCX, maxCZ);

        minCX = (std::max)(0, minCX);
        minCZ = (std::max)(0, minCZ);
        maxCX = (std::min)(m_gridWidth - 1, maxCX);
        maxCZ = (std::min)(m_gridHeight - 1, maxCZ);

        for (int z = minCZ; z <= maxCZ; ++z)
        {
            for (int x = minCX; x <= maxCX; ++x)
            {
                bool block = false;
                if (obs.type == ObstacleType::AABB)
                {
                    block = true;
                }
                else // Cylinder
                {
                    float cx = m_worldMinX + (x + 0.5f) * m_cellSize;
                    float cz = m_worldMinZ + (z + 0.5f) * m_cellSize;
                    float dx = cx - obs.centerX;
                    float dz = cz - obs.centerZ;
                    block = (dx * dx + dz * dz) <= obs.radius * obs.radius;
                }
                if (block)
                {
                    m_grid[static_cast<size_t>(z) * m_gridWidth + x].walkable = false;
                }
            }
        }
    }
}

void NavMesh::RemoveObstacleEffects()
{
    if (!m_baseGrid.empty())
        m_grid = m_baseGrid;
}

} // namespace gx
