#pragma once
/// @file converter.h
/// @brief 変換処理のオーケストレーター
///
/// CLIオプションに基づいてインポーターとエクスポーターを選択・実行する。

#include <string>
#include "shader_model.h"

namespace gxconv
{

/// @brief 変換オプション (CLIパラメータから構築)
struct ConvertOptions
{
    std::string inputPath;                 ///< 入力ファイルパス (.obj/.fbx/.gltf/.glb)
    std::string outputPath;                ///< 出力ファイルパス (省略時は拡張子を自動変更)
    bool infoOnly        = false;          ///< trueなら変換せずファイル情報を表示
    bool useIndex16      = false;          ///< 16bitインデックスを強制
    bool excludeAnimations = false;        ///< アニメーションデータを除外
    bool animOnly        = false;          ///< アニメーションのみを.gxanとしてエクスポート
    bool hasShaderModelOverride = false;   ///< シェーダーモデル上書きが指定されたか
    gxfmt::ShaderModel shaderModelOverride = gxfmt::ShaderModel::Standard; ///< 上書き用シェーダーモデル
    float toonOutlineWidth = 0.0f;         ///< Toonアウトライン幅 (0=未指定)
};

/// @brief 変換処理の統括クラス
/// @details インポート→中間表現→エクスポートのパイプラインを管理する。
class Converter
{
public:
    /// @brief オプションに基づいて変換を実行する
    /// @param options 変換オプション
    /// @return 終了コード (0=成功)
    int Run(const ConvertOptions& options);

private:
    /// @brief .gxmd/.gxanファイルの情報を表示する (--infoオプション)
    /// @param path 対象ファイルパス
    /// @return 終了コード (0=成功)
    int ShowInfo(const std::string& path);
};

} // namespace gxconv
