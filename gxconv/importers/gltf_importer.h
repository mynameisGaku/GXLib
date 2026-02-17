#pragma once
/// @file gltf_importer.h
/// @brief glTF/GLB importer using cgltf

#include <string>

namespace gxconv
{

struct Scene;

class GltfImporter
{
public:
    bool Import(const std::string& filePath, Scene& outScene);
};

} // namespace gxconv
