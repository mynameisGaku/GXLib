#pragma once
/// @file SceneSerializer.h
/// @brief シーン直列化（JSON保存・読み込み）

#include "pch.h"
#include "Core/Scene/Scene.h"

namespace GX
{

/// @brief シーンのJSON直列化
class SceneSerializer
{
public:
    /// @brief Model*を解決するコールバック
    using ModelLoadCallback = std::function<Model*(const std::string& path)>;

    /// @brief シーンをJSONファイルに保存する
    static bool SaveToJson(const Scene& scene, const std::string& filePath);

    /// @brief JSONファイルからシーンを読み込む
    static bool LoadFromJson(Scene& scene, const std::string& filePath,
                              ModelLoadCallback modelLoader = nullptr);

    /// @brief シーンをJSON文字列に変換する
    static std::string ToJsonString(const Scene& scene);

    /// @brief JSON文字列からシーンを復元する
    static bool FromJsonString(Scene& scene, const std::string& json,
                                ModelLoadCallback modelLoader = nullptr);
};

} // namespace GX
