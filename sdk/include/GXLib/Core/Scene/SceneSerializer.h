#pragma once
/// @file SceneSerializer.h
/// @brief シーン直列化（JSON保存・読み込み）

#include "pch_common.h"
#include "Core/Scene/Scene.h"

namespace gx
{
/// @addtogroup grp_scene
/// @{

class Model; // forward declaration

/// @brief シーンのJSON直列化
class SceneSerializer
{
public:
    /// @brief Model*を解決するコールバック
    using ModelLoadCallback = std::function<Model*(const gx::String& path)>;

    /// @brief シーンをJSONファイルに保存する
    static bool SaveToJson(const Scene& scene, const gx::String& filePath);

    /// @brief JSONファイルからシーンを読み込む
    static bool LoadFromJson(Scene& scene, const gx::String& filePath,
                              ModelLoadCallback modelLoader = nullptr);

    /// @brief シーンをJSON文字列に変換する
    static gx::String ToJsonString(const Scene& scene);

    /// @brief JSON文字列からシーンを復元する
    static bool FromJsonString(Scene& scene, const gx::String& json,
                                ModelLoadCallback modelLoader = nullptr);
};

/// @}
} // namespace gx
