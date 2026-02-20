#pragma once
/// @file PropertyPanel.h
/// @brief Property inspector panel for selected entities

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"
#include <imgui.h>
#include "ImGuizmo.h"

/// @brief ImGui panel that displays and edits properties of the selected entity
class PropertyPanel
{
public:
    /// Draw the property panel (standalone window — without gizmo controls).
    void Draw(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager,
              ImGuizmo::OPERATION& gizmoOp, ImGuizmo::MODE& gizmoMode,
              bool& useSnap, float& snapT, float& snapR, float& snapS);

    /// Draw only the content (no Begin/End), for embedding in a tabbed container.
    void DrawContent(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager,
                     ImGuizmo::OPERATION& gizmoOp, ImGuizmo::MODE& gizmoMode,
                     bool& useSnap, float& snapT, float& snapR, float& snapS);

private:
    /// Draw the transform editing section.
    void DrawTransformSection(SceneEntity& entity);

    /// Draw the material override section.
    void DrawMaterialOverrideSection(SceneEntity& entity);

    /// Draw the model's actual materials (direct editing).
    void DrawModelMaterials(SceneEntity& entity, GX::MaterialManager& matManager);

    /// Draw shader-model-specific parameters.
    void DrawShaderModelParams(gxfmt::ShaderModelParams& params, gxfmt::ShaderModel model);

    // Name edit buffer
    char m_nameBuffer[256] = {};
};
