#pragma once
/// @file gxmd_exporter.h
/// @brief GXMD binary model exporter

#include <string>

namespace gxconv
{

struct Scene;

/// @brief GXMD出力オプション
struct ExportOptions
{
    bool useIndex16 = false;         ///< 16bitインデックスを強制する（頂点数65535以下の場合）
    bool excludeAnimations = false;  ///< アニメーションデータを除外する
};

/// @brief GXMDバイナリモデル形式のエクスポーター
class GxmdExporter
{
public:
    /// @brief 中間表現をGXMDファイルとして書き出す
    /// @param scene エクスポート対象のシーン
    /// @param outputPath 出力ファイルパス (.gxmd)
    /// @param options 出力オプション
    /// @return 成功時true
    bool Export(const Scene& scene, const std::string& outputPath,
               const ExportOptions& options = {});
};

} // namespace gxconv
