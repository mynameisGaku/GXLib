#pragma once
/// @file LightingPanel.h
/// @brief Lighting editor panel for the scene

#include <vector>
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Renderer3D.h"

/// @brief ImGui panel for editing scene lights
class LightingPanel
{
public:
    /// Initialize the panel with default lights.
    void Initialize();

    /// Draw the lighting panel.
    /// @param renderer The 3D renderer to apply light changes to.
    void Draw(GX::Renderer3D& renderer);

    /// Get the current light data array.
    const std::vector<GX::LightData>& GetLights() const { return m_lights; }

    /// Get the ambient color.
    const float* GetAmbientColor() const { return m_ambientColor; }

private:
    static constexpr int k_MaxLights = 16;

    std::vector<GX::LightData> m_lights;
    float m_ambientColor[3] = { 0.15f, 0.15f, 0.18f };
    bool  m_dirty = true; // track if lights need to be applied
};
