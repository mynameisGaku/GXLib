#pragma once
/// @file ModelLoader.h
/// @brief 3Dモデルローダー（glTF/FBX/OBJ）

#include "pch.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief 3Dモデルローダー（DxLibの MV1LoadModel 内部処理に相当）
/// glTF/FBX/OBJ/.gxmd を読み込み、Model を構築する。
/// 拡張子で自動的にフォーマットを判別し、メッシュ・マテリアル・スケルトン・アニメーションを取り込む
class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    /// @brief 3Dモデルファイルを読み込んで Model を構築する
    /// @param filePath モデルファイルのパス（.gltf/.glb/.fbx/.obj/.gxmd）
    /// @param device D3D12デバイス
    /// @param texManager テクスチャ管理（テクスチャ読み込みに使用）
    /// @param matManager マテリアル管理（マテリアル登録に使用）
    /// @return 読み込んだ Model（失敗時はnullptr）
    std::unique_ptr<Model> LoadFromFile(const std::wstring& filePath,
                                        ID3D12Device* device,
                                        TextureManager& texManager,
                                        MaterialManager& matManager);
};

} // namespace GX
