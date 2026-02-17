#pragma once
/// @file PropertyPanel.h
/// @brief Property inspector panel for selected entities

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"

/// @brief ImGui panel that displays and edits properties of the selected entity
class PropertyPanel
{
public:
    /// Draw the property panel.
    /// @param scene The scene graph (reads selectedEntity).
    /// @param matManager Material manager for material operations.
    /// @param texManager Texture manager for texture operations.
    void Draw(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager);

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
