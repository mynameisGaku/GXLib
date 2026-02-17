#pragma once
/// @file obj_importer.h
/// @brief OBJ/MTL importer using tinyobjloader

#include <string>

namespace gxconv
{

struct Scene;

class ObjImporter
{
public:
    bool Import(const std::string& filePath, Scene& outScene);
};

} // namespace gxconv
