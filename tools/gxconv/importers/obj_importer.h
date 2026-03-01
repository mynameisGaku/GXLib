#pragma once
/// @file obj_importer.h
/// @brief OBJ/MTLファイルのインポーター (tinyobjloader使用)

#include <string>

namespace gxconv
{

struct Scene;

/// @brief OBJ形式の3Dモデルを中間表現に読み込む
/// @details MTLマテリアルの照明モデルからシェーダーモデルを自動判定する。
///          スキニングデータには非対応 (OBJ仕様にボーン情報がないため)。
class ObjImporter
{
public:
    /// @brief OBJファイルを読み込んで中間表現に変換する
    /// @param filePath 入力OBJファイルパス
    /// @param outScene 変換結果の格納先
    /// @return 成功時true
    bool Import(const std::string& filePath, Scene& outScene);
};

} // namespace gxconv
