#pragma once
/// @file SceneHierarchyPanel.h
/// @brief Scene hierarchy panel showing all entities

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Skeleton.h"

/// @brief ImGui panel that displays the scene hierarchy and allows selection
class SceneHierarchyPanel
{
public:
    /// Draw the hierarchy panel.
    /// @param scene The scene graph to display and manipulate.
    void Draw(SceneGraph& scene);

private:
    void DrawBoneTree(const GX::Skeleton* skeleton, SceneGraph& scene, int jointIndex);
};
