#pragma once
/// @file SceneSerializer.h
/// @brief Scene save/load to JSON

#include "SceneGraph.h"
#include <string>

/// @brief Serializes/deserializes a SceneGraph to/from JSON files
class SceneSerializer
{
public:
    /// Save the scene to a JSON file.
    /// @param scene The scene graph to serialize.
    /// @param filePath Output file path.
    /// @return true on success.
    static bool SaveToFile(const SceneGraph& scene, const std::string& filePath);

    /// Load a scene from a JSON file.
    /// @param scene The scene graph to populate (existing entities are cleared by adding new ones).
    /// @param filePath Input file path.
    /// @return true on success.
    static bool LoadFromFile(SceneGraph& scene, const std::string& filePath);
};
