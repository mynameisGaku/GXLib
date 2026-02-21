#pragma once
/// @file NavMesh.h
/// @brief Grid-based navigation mesh with A* pathfinding
///
/// Recast/Detour not used. Lightweight standalone implementation.
/// Divides world space into a cell grid, determines walkable cells from
/// height map data (terrain sampling or manual geometry), and runs
/// A* search for shortest paths.

#include "pch.h"

namespace GX
{

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
    /// @return true if a path was found
    bool FindPath(const XMFLOAT3& start, const XMFLOAT3& end,
                  std::vector<XMFLOAT3>& path) const;

    /// @brief Find the nearest walkable cell to a world position
    bool FindNearestWalkable(const XMFLOAT3& position, XMFLOAT3& nearest) const;

    /// @brief Check if a world position is on a walkable cell
    bool IsWalkable(const XMFLOAT3& position) const;

    /// @brief Debug draw using PrimitiveBatch3D (green = walkable, red = blocked)
    void DebugDraw(PrimitiveBatch3D& batch) const;

    /// @brief Debug draw a path as a series of lines
    void DebugDrawPath(PrimitiveBatch3D& batch,
                       const std::vector<XMFLOAT3>& path,
                       const XMFLOAT4& color) const;

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
    XMFLOAT3 CellToWorld(int cellX, int cellZ) const;

    /// Diagonal distance heuristic
    static float Heuristic(int x1, int z1, int x2, int z2);

    /// Mark cells unwalkable based on slope between neighbours
    void ApplySlopeFilter(float maxClimb, float maxSlope);

    std::vector<Cell> m_grid;
    int   m_gridWidth  = 0;
    int   m_gridHeight = 0;
    float m_cellSize   = 0.5f;
    float m_worldMinX  = 0.0f;
    float m_worldMinZ  = 0.0f;
    bool  m_built      = false;
};

} // namespace GX
