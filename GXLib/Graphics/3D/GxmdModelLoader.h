#pragma once
/// @file GxmdModelLoader.h
/// @brief 独自バイナリ形式(.gxmd)のモデルローダー

#include "pch.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief GXMDバイナリ形式(.gxmd)のモデルローダー
/// gxconv で変換した .gxmd ファイルを読み込み、GX::Model に変換する。
/// gxloader::LoadGxmdW でバイナリデータを読み込み、頂点レイアウト互換を利用してゼロコピーに近い形で構築する
class GxmdModelLoader
{
public:
    /// @brief .gxmd ファイルから Model を構築する
    /// @param filePath GXMDファイルのパス
    /// @param device D3D12デバイス
    /// @param texManager テクスチャ管理（テクスチャ読み込みに使用）
    /// @param matManager マテリアル管理（マテリアル登録に使用）
    /// @return 読み込んだ Model（失敗時はnullptr）
    std::unique_ptr<Model> LoadFromGxmd(const std::wstring& filePath,
                                         ID3D12Device* device,
                                         TextureManager& texManager,
                                         MaterialManager& matManager);
};

} // namespace GX
