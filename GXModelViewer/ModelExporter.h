#pragma once
/// @file ModelExporter.h
/// @brief GX::Model -> GXMD/GXAN export (with edited materials)

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"
#include <string>

/// @brief Exports GX::Model data to GXMD/GXAN binary files
class ModelExporter
{
public:
    /// Export the selected entity's model as GXMD (includes edited materials + texture paths).
    static bool ExportToGxmd(const SceneEntity& entity,
                             GX::MaterialManager& matManager,
                             GX::TextureManager& texManager,
                             const std::string& outputPath);

    /// Export the selected entity's animations as GXAN.
    static bool ExportToGxan(const SceneEntity& entity,
                             const std::string& outputPath);
};
