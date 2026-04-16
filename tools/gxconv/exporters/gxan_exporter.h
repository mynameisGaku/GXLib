#pragma once
/// @file gxan_exporter.h
/// @brief GXAN standalone animation exporter

#include <string>

namespace gxconv
{

struct Scene;

/// @brief GXANアニメーション形式のエクスポーター
class GxanExporter
{
public:
    /// @brief 中間表現のアニメーションデータをGXANファイルとして書き出す
    /// @param scene エクスポート対象のシーン（animations配列を使用）
    /// @param outputPath 出力ファイルパス (.gxan)
    /// @return 成功時true
    bool Export(const Scene& scene, const std::string& outputPath);
};

} // namespace gxconv
