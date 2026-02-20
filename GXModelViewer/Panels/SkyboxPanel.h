#pragma once
/// @file SkyboxPanel.h
/// @brief ImGui panel for Skybox settings

#include <string>

namespace GX { class Skybox; }

/// @brief ImGui panel for editing Skybox parameters
class SkyboxPanel
{
public:
    /// Draw the skybox settings panel (standalone window).
    void Draw(GX::Skybox& skybox);

    /// Draw only the content (no Begin/End), for embedding in a tabbed container.
    void DrawContent(GX::Skybox& skybox);

private:
    // Cubemap face file paths (placeholder for future cubemap support)
    std::string m_cubemapFaces[6];

    // HDR environment map
    std::string m_hdrEnvMapPath;

    // Local copies of skybox parameters (Skybox has no getters)
    float m_topColor[3]    = { 0.3f, 0.5f, 0.9f };
    float m_bottomColor[3] = { 0.7f, 0.8f, 0.95f };
    float m_sunDirection[3] = { 0.3f, -1.0f, 0.5f };
    float m_sunIntensity    = 5.0f;
    float m_rotation        = 0.0f;
};
