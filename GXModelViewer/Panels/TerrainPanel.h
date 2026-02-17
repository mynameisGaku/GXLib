#pragma once
/// @file TerrainPanel.h
/// @brief ImGui panel for terrain editing (placeholder)

#include <string>

/// @brief ImGui panel for terrain parameters (placeholder UI, not wired to actual terrain)
class TerrainPanel
{
public:
    /// Draw the terrain editing panel.
    void Draw();

private:
    // Heightmap
    std::string m_heightmapPath;

    // Parameters
    float m_heightScale = 10.0f;

    // Splat texture layers (4 layers)
    std::string m_splatTextures[4];

    // LOD
    int m_lodLevel = 3;
};
