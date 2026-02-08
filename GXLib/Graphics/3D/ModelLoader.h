#pragma once
/// @file ModelLoader.h
/// @brief glTF 2.0モデルローダー

#include "pch.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief glTF 2.0モデルローダー
class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    /// glTFファイルを読み込んでModelを返す
    /// @param filePath glTFファイルパス（.gltf or .glb）
    /// @param device D3D12デバイス
    /// @param texManager テクスチャマネージャー
    /// @param matManager マテリアルマネージャー
    /// @return 読み込んだModel（失敗時はnullptr）
    std::unique_ptr<Model> LoadFromFile(const std::wstring& filePath,
                                         ID3D12Device* device,
                                         TextureManager& texManager,
                                         MaterialManager& matManager);
};

} // namespace GX
