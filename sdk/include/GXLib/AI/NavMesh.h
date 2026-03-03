#pragma once
/// @file NavMesh.h
/// @brief Grid-based navigation mesh with A* pathfinding
///
/// Recast/Detour not used. Lightweight standalone implementation.
/// Divides world space into a cell grid, determines walkable cells from
/// height map data (terrain sampling or manual geometry), and runs
/// A* search for shortest paths.

#include "pch_common.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace gx
{
/// @addtogroup grp_ai
/// @{

class Terrain;
class PrimitiveBatch3D;

/// @brief Grid-based navigation mesh
class NavMesh
{
public:
    NavMesh() = default;
    ~NavMesh() = default;

    /// @brief Build a flat navmesh grid with given world bounds
    /// @param worldMinX World-space minimum X
    /// @param worldMinZ World-space minimum Z
    /// @param worldMaxX World-space maximum X
    /// @param worldMaxZ World-space maximum Z
    /// @param cellSize  Cell edge length (world units)
    /// @param maxClimb  Maximum walkable height difference between adjacent cells
    /// @param maxSlope  Maximum walkable slope (degrees)
    /// @return true on success
    bool Build(float worldMinX, float worldMinZ,
               float worldMaxX, float worldMaxZ,
               float cellSize = 0.5f,
               float maxClimb = 0.9f,
               float maxSlope = 45.0f);

    /// @brief Build from a Terrain instance (samples heights automatically)
    bool BuildFromTerrain(const Terrain& terrain,
                          float cellSize = 0.5f,
                          float maxClimb = 0.9f,
                          float maxSlope = 45.0f);

    /// @brief Build from raw geometry (rasterize triangles onto grid)
    /// @param vertices  Vertex positions (x,y,z repeated, stride = 3 floats)
    /// @param vertexCount Number of vertices
    /// @param indices   Triangle index array
    /// @param indexCount Number of indices (must be multiple of 3)
    /// @param maxClimb  Maximum walkable height difference
    /// @param maxSlope  Maximum walkable slope (degrees)
    bool BuildFromGeometry(const float* vertices, int vertexCount,
                           const int* indices, int indexCount,
                           float maxClimb = 0.9f, float maxSlope = 45.0f);

    /// @brief Manually set a cell's walkable state
    void SetCellWalkable(int cellX, int cellZ, bool walkable);

    /// @brief Set a cell's cost multiplier (e.g. mud = 2.0, water = 3.0)
    void SetCellCost(int cellX, int cellZ, float costMultiplier);

    /// @brief Find a path between two world positions using A*
    /// @param start Start world position
    /// @param end   Goal world position
    /// @param path  Output: waypoints in world coordinates
    /// @param smooth If true, apply path smoothing (default: false)
    /// @return true if a path was found
    bool FindPath(const Vector3& start, const Vector3& end,
                  gx::Vector<Vector3>& path, bool smooth = false) const;

    /// @brief Find the nearest walkable cell to a world position
    bool FindNearestWalkable(const Vector3& position, Vector3& nearest) const;

    /// @brief Check if a world position is on a walkable cell
    bool IsWalkable(const Vector3& position) const;

    /// @brief Debug draw using PrimitiveBatch3D (green = walkable, red = blocked)
    void DebugDraw(PrimitiveBatch3D& batch) const;

    /// @brief Debug draw a path as a series of lines
    void DebugDrawPath(PrimitiveBatch3D& batch,
                       const gx::Vector<Vector3>& path,
                       const Vector4& color) const;

    // --- Dynamic obstacle API ---

    /// @brief AABBタイプの障害物を追加する
    /// @param minX 最小X座標
    /// @param minZ 最小Z座標
    /// @param maxX 最大X座標
    /// @param maxZ 最大Z座標
    /// @return 障害物ハンドル（0以上）。失敗時は0
    uint32_t AddObstacleAABB(float minX, float minZ, float maxX, float maxZ);

    /// @brief 円柱タイプの障害物を追加する
    /// @param centerX 中心X座標
    /// @param centerZ 中心Z座標
    /// @param radius 半径
    /// @return 障害物ハンドル
    uint32_t AddObstacleCylinder(float centerX, float centerZ, float radius);

    /// @brief 障害物を削除する
    /// @param handle AddObstacleXXXで取得したハンドル
    void RemoveObstacle(uint32_t handle);

    /// @brief 登録されている障害物の数を取得する
    uint32_t GetObstacleCount() const { return static_cast<uint32_t>(m_obstacles.size()); }

    /// @brief 全障害物を削除する
    void ClearObstacles();

    /// @brief 障害物を移動する（AABBの場合オフセットとして適用）
    /// @param handle 障害物ハンドル
    /// @param newMinX 新しい最小X座標
    /// @param newMinZ 新しい最小Z座標
    void MoveObstacle(uint32_t handle, float newMinX, float newMinZ);

    int   GetGridWidth()  const { return m_gridWidth; }
    int   GetGridHeight() const { return m_gridHeight; }
    float GetCellSize()   const { return m_cellSize; }
    bool  IsBuilt()       const { return m_built; }

private:
    /// @brief Internal cell data
    struct Cell
    {
        float height         = 0.0f;   ///< Height at cell center
        bool  walkable       = true;   ///< Is cell walkable
        float costMultiplier = 1.0f;   ///< Cost modifier (1.0 = normal)
    };

    /// @brief A* search node (used during pathfinding)
    struct AStarNode
    {
        int   x, z;
        float g, h, f;
        int   parentX, parentZ;

        /// For priority queue (min-heap by f)
        bool operator>(const AStarNode& other) const { return f > other.f; }
    };

    /// World coords -> cell indices
    void WorldToCell(float worldX, float worldZ, int& cellX, int& cellZ) const;

    /// Cell indices -> world center position
    Vector3 CellToWorld(int cellX, int cellZ) const;

    /// Diagonal distance heuristic
    static float Heuristic(int x1, int z1, int x2, int z2);

    /// Mark cells unwalkable based on slope between neighbours
    void ApplySlopeFilter(float maxClimb, float maxSlope);

    /// @brief 障害物種類
    enum class ObstacleType { AABB, Cylinder };

    /// @brief 動的障害物データ
    struct Obstacle
    {
        ObstacleType type;
        float minX, minZ, maxX, maxZ; // AABB
        float centerX, centerZ, radius; // Cylinder
        uint32_t handle;
    };

    /// @brief 障害物の影響をセルに反映する
    void ApplyObstacles();

    /// @brief 障害物の影響を除去して元のwalkable状態に戻す
    void RemoveObstacleEffects();

    gx::Vector<Cell> m_grid;
    gx::Vector<Cell> m_baseGrid;  ///< 障害物適用前の基礎グリッド
    gx::Vector<Obstacle> m_obstacles;
    uint32_t m_nextObstacleHandle = 1;
    int   m_gridWidth  = 0;
    int   m_gridHeight = 0;
    float m_cellSize   = 0.5f;
    float m_worldMinX  = 0.0f;
    float m_worldMinZ  = 0.0f;
    bool  m_built      = false;
};

/// @}
} // namespace gx
