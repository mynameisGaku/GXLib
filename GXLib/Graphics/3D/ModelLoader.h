#pragma once
/// @file ModelLoader.h
/// @brief 3Dモデルローダー（glTF/FBX/OBJ）

#include "pch.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief 3Dモデルローダー（glTF/FBX/OBJ）
class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    /// @brief 3Dモデルファイルを読み込み、Modelを構築する
    /// @param filePath モデルファイルのパス（.gltf/.glb/.fbx/.obj）
    /// @param device D3D12デバイス
    /// @param texManager テクスチャ管理
    /// @param matManager マテリアル管理
    /// @return 読み込んだModel（失敗時はnullptr）
    /// 初学者向け: 拡張子でフォーマットを判別し、必要なデータをまとめて取り込みます。
    std::unique_ptr<Model> LoadFromFile(const std::wstring& filePath,
                                        ID3D12Device* device,
                                        TextureManager& texManager,
                                        MaterialManager& matManager);
};

} // namespace GX
