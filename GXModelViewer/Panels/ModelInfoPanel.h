#pragma once
/// @file ModelInfoPanel.h
/// @brief Model information display panel

#include "Scene/SceneGraph.h"

/// @brief ImGui panel showing detailed model information
class ModelInfoPanel
{
public:
    void Draw(const SceneGraph& scene);
    void DrawContent(const SceneGraph& scene);
};
