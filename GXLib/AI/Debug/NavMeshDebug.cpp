/// @file NavMeshDebug.cpp
/// @brief Graphics-dependent NavMesh / NavMesh3D helpers.
///
/// Split from `AI/NavMesh.cpp` + `AI/NavMesh3D.cpp` on 2026-04-18 to restore
/// ADR-0018 §Constraints "AI module links only GXLib_Foundation" — the core
/// AI pathfinding + query TUs are now free of any `Graphics/` include. This
/// TU pulls Graphics on purpose and is packaged into the sibling
/// `GXLib_AIDebug` static library, which callers link explicitly when they
/// need `DebugDraw`/`BuildFromTerrain` (typically tools + reference apps).
///
/// The methods defined here are members of `NavMesh` / `NavMesh3D`; moving
/// the definitions to a separate TU does not change the public contract.
/// Consumers that don't call `DebugDraw`/`DebugDrawPath`/`BuildFromTerrain`
/// don't need to link `GXLib_AIDebug` at all.

#include "pch_graphics.h"

#include "AI/NavMesh.h"
#include "AI/NavMesh3D.h"
#include "Math/MathConvert.h"
#include "Graphics/3D/Terrain.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// NavMesh::BuildFromTerrain — moved from GXLib/AI/NavMesh.cpp (2026-04-18)
// ============================================================================
bool NavMesh::BuildFromTerrain(const Terrain& terrain,
                               float cellSize, float maxClimb, float maxSlope)
{
    // Terrain is centered at origin: originX = -width/2, originZ = -depth/2
    float worldMinX = terrain.GetOriginX();
    float worldMinZ = terrain.GetOriginZ();
    float worldMaxX = worldMinX + terrain.GetWidth();
    float worldMaxZ = worldMinZ + terrain.GetDepth();

    if (!Build(worldMinX, worldMinZ, worldMaxX, worldMaxZ, cellSize, maxClimb, maxSlope))
        return false;

    // Sample terrain heights at each cell center
    for (int z = 0; z < m_gridHeight; ++z)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            Vector3 worldPos = CellToWorld(x, z);
            float h = terrain.GetHeight(worldPos.x, worldPos.z);
            m_grid[static_cast<size_t>(z) * m_gridWidth + x].height = h;
        }
    }

    // Apply slope/climb filtering
    ApplySlopeFilter(maxClimb, maxSlope);

    Logger::Info("NavMesh::BuildFromTerrain - terrain (%.1fx%.1f) -> %dx%d grid",
                 terrain.GetWidth(), terrain.GetDepth(), m_gridWidth, m_gridHeight);
    return true;
}

// ============================================================================
// NavMesh::DebugDraw — moved from GXLib/AI/NavMesh.cpp (2026-04-18)
// ============================================================================
void NavMesh::DebugDraw(PrimitiveBatch3D& batch) const
{
    if (!m_built) return;

    const Vector4 walkableColor   = { 0.1f, 0.8f, 0.2f, 0.4f };
    const Vector4 unwalkableColor = { 0.9f, 0.15f, 0.1f, 0.5f };
    const float drawOffset = 0.05f; // Slight Y offset to avoid z-fighting

    for (int z = 0; z < m_gridHeight; ++z)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            const Cell& cell = m_grid[static_cast<size_t>(z) * m_gridWidth + x];
            Vector3 center = CellToWorld(x, z);
            center.y = cell.height + drawOffset;

            const Vector4& color = cell.walkable ? walkableColor : unwalkableColor;

            float half = m_cellSize * 0.45f; // Slightly smaller than cell to see grid lines

            // Draw cell as 4 lines (rectangle outline on XZ plane)
            Vector3 p0 = { center.x - half, center.y, center.z - half };
            Vector3 p1 = { center.x + half, center.y, center.z - half };
            Vector3 p2 = { center.x + half, center.y, center.z + half };
            Vector3 p3 = { center.x - half, center.y, center.z + half };

            batch.DrawLine(p0, p1, color);
            batch.DrawLine(p1, p2, color);
            batch.DrawLine(p2, p3, color);
            batch.DrawLine(p3, p0, color);
        }
    }
}

// ============================================================================
// NavMesh::DebugDrawPath — moved from GXLib/AI/NavMesh.cpp (2026-04-18)
// ============================================================================
void NavMesh::DebugDrawPath(PrimitiveBatch3D& batch,
                            const gx::Vector<Vector3>& path,
                            const Vector4& color) const
{
    if (path.size() < 2) return;

    const float yOffset = 0.15f;
    for (size_t i = 0; i + 1 < path.size(); ++i)
    {
        Vector3 a = path[i];
        Vector3 b = path[i + 1];
        a.y += yOffset;
        b.y += yOffset;
        batch.DrawLine(a, b, color);
    }
}

// ============================================================================
// NavMesh3D::DebugDraw — moved from GXLib/AI/NavMesh3D.cpp (2026-04-18)
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

} // namespace gx
