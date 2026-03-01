#pragma once
/// @file fbx_importer.h
/// @brief FBXファイルのインポーター (ufbx使用)

#include <string>

namespace gxconv
{

struct Scene;

/// @brief FBX形式の3Dモデルを中間表現に読み込む
/// @details ufbxライブラリでパースし、左手Y-up座標系に変換する。
///          メッシュ、マテリアル、スケルトン、アニメーション全てに対応する。
class FbxImporter
{
public:
    /// @brief FBXファイルを読み込んで中間表現に変換する
    /// @param filePath 入力FBXファイルパス
    /// @param outScene 変換結果の格納先
    /// @return 成功時true
    bool Import(const std::string& filePath, Scene& outScene);
};

} // namespace gxconv
