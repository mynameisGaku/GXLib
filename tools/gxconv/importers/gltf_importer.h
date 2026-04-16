#pragma once
/// @file gltf_importer.h
/// @brief glTF/GLBファイルのインポーター (cgltf使用)

#include <string>

namespace gxconv
{

struct Scene;

/// @brief glTF 2.0形式の3Dモデルを中間表現に読み込む
/// @details cgltfライブラリでパースする。PBR metallic-roughnessワークフローに対応。
///          メッシュ、マテリアル、スケルトン、アニメーション全てに対応する。
class GltfImporter
{
public:
    /// @brief glTF/GLBファイルを読み込んで中間表現に変換する
    /// @param filePath 入力ファイルパス (.gltf/.glb)
    /// @param outScene 変換結果の格納先
    /// @return 成功時true
    bool Import(const std::string& filePath, Scene& outScene);
};

} // namespace gxconv
