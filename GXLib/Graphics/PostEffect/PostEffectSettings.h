#pragma once
/// @file PostEffectSettings.h
/// @brief ポストエフェクト設定のJSON読み書き

#include <string>

namespace GX
{

class PostEffectPipeline;

/// @brief ポストエフェクト設定のJSON入出力
class PostEffectSettings
{
public:
    static bool Load(const std::string& filePath, PostEffectPipeline& pipeline);
    static bool Save(const std::string& filePath, const PostEffectPipeline& pipeline);
};

} // namespace GX
