#pragma once
/// @file SceneSerializer.h
/// @brief シーンのJSON保存/読み込み
///
/// エンティティの名前、Transform、マテリアルオーバーライド、モデルパスを
/// nlohmann/jsonでシリアライズ/デシリアライズする。モデル自体のリロードは
/// アプリケーション側(GXModelViewerApp)で行う。

#include "SceneGraph.h"
#include <string>

/// @brief SceneGraphをJSONファイルに保存/復元するシリアライザ
class SceneSerializer
{
public:
    /// @brief シーンをJSONファイルに保存する
    /// @param scene 保存対象のシーングラフ
    /// @param filePath 出力先ファイルパス
    /// @return 成功時true
    static bool SaveToFile(const SceneGraph& scene, const std::string& filePath);

    /// @brief JSONファイルからシーンを読み込む（エンティティが追加される）
    /// @param scene 読み込み先のシーングラフ
    /// @param filePath 入力ファイルパス
    /// @return 成功時true
    static bool LoadFromFile(SceneGraph& scene, const std::string& filePath);
};
