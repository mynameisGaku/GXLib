#pragma once
/// @file fbx_importer.h
/// @brief FBX importer using ufbx

#include <string>

namespace gxconv
{

struct Scene;

class FbxImporter
{
public:
    bool Import(const std::string& filePath, Scene& outScene);
};

} // namespace gxconv
