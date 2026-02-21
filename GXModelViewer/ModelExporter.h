#pragma once
/// @file ModelExporter.h
/// @brief GX::ModelからGXMD/GXANバイナリへのエクスポート
///
/// ビューア上で編集したマテリアルやテクスチャパスも含めてバイナリ出力する。
/// 内部でgxconv中間表現(Scene)に変換し、gxconv::GxmdExporter/GxanExporterで書き出す。

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"
#include <string>

/// @brief GX::ModelをGXMD/GXANバイナリにエクスポートするユーティリティ
class ModelExporter
{
public:
    /// @brief 選択エンティティのモデルをGXMDバイナリに書き出す
    /// @param entity 対象エンティティ（model必須）
    /// @param matManager マテリアルハンドルからパラメータを取得するために使用
    /// @param texManager テクスチャハンドルからファイルパスを取得するために使用
    /// @param outputPath 出力先ファイルパス
    /// @return 成功時true
    static bool ExportToGxmd(const SceneEntity& entity,
                             GX::MaterialManager& matManager,
                             GX::TextureManager& texManager,
                             const std::string& outputPath);

    /// @brief 選択エンティティのアニメーションをGXANバイナリに書き出す
    /// @param entity 対象エンティティ（model必須、アニメーション1つ以上）
    /// @param outputPath 出力先ファイルパス
    /// @return 成功時true
    static bool ExportToGxan(const SceneEntity& entity,
                             const std::string& outputPath);
};
