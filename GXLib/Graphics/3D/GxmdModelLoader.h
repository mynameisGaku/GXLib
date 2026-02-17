#pragma once
/// @file GxmdModelLoader.h
/// @brief GXMD model loader adapter for GXLib engine

#include "pch.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

class GxmdModelLoader
{
public:
    std::unique_ptr<Model> LoadFromGxmd(const std::wstring& filePath,
                                         ID3D12Device* device,
                                         TextureManager& texManager,
                                         MaterialManager& matManager);
};

} // namespace GX
