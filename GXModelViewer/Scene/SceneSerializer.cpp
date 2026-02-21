/// @file SceneSerializer.cpp
/// @brief シーンJSON保存/読み込み実装
///
/// nlohmann/jsonを使用。version=1フォーマット。
/// エンティティごとにname, visible, parentIndex, transform(TRS), materialOverride,
/// modelPath(インポート元パス)を保存する。モデル実体の復元はアプリ側で行う。

#include "SceneSerializer.h"
#include <json.hpp>
#include <fstream>

using json = nlohmann::json;

// ============================================================
// Transform3D <-> JSON 変換ヘルパー
// ============================================================
static json SerializeTransform(const GX::Transform3D& transform)
{
    const auto& pos = transform.GetPosition();
    const auto& rot = transform.GetRotation();
    const auto& scl = transform.GetScale();

    return {
        {"position", {pos.x, pos.y, pos.z}},
        {"rotation", {rot.x, rot.y, rot.z}},
        {"scale",    {scl.x, scl.y, scl.z}}
    };
}

static void DeserializeTransform(GX::Transform3D& transform, const json& j)
{
    if (j.contains("position") && j["position"].is_array() && j["position"].size() == 3)
    {
        transform.SetPosition(
            j["position"][0].get<float>(),
            j["position"][1].get<float>(),
            j["position"][2].get<float>());
    }

    if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() == 3)
    {
        transform.SetRotation(
            j["rotation"][0].get<float>(),
            j["rotation"][1].get<float>(),
            j["rotation"][2].get<float>());
    }

    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() == 3)
    {
        transform.SetScale(
            j["scale"][0].get<float>(),
            j["scale"][1].get<float>(),
            j["scale"][2].get<float>());
    }
}

// ============================================================
// MaterialConstants <-> JSON 変換ヘルパー
// ============================================================
static json SerializeMaterial(const GX::Material& mat)
{
    json j;
    const auto& c = mat.constants;

    j["albedo"]    = {c.albedoFactor.x, c.albedoFactor.y, c.albedoFactor.z, c.albedoFactor.w};
    j["metallic"]  = c.metallicFactor;
    j["roughness"] = c.roughnessFactor;
    j["ao"]        = c.aoStrength;
    j["emissiveStrength"] = c.emissiveStrength;
    j["emissive"]  = {c.emissiveFactor.x, c.emissiveFactor.y, c.emissiveFactor.z};

    return j;
}

static void DeserializeMaterial(GX::Material& mat, const json& j)
{
    auto& c = mat.constants;

    if (j.contains("albedo") && j["albedo"].is_array() && j["albedo"].size() == 4)
    {
        c.albedoFactor.x = j["albedo"][0].get<float>();
        c.albedoFactor.y = j["albedo"][1].get<float>();
        c.albedoFactor.z = j["albedo"][2].get<float>();
        c.albedoFactor.w = j["albedo"][3].get<float>();
    }

    if (j.contains("metallic"))
        c.metallicFactor = j["metallic"].get<float>();
    if (j.contains("roughness"))
        c.roughnessFactor = j["roughness"].get<float>();
    if (j.contains("ao"))
        c.aoStrength = j["ao"].get<float>();
    if (j.contains("emissiveStrength"))
        c.emissiveStrength = j["emissiveStrength"].get<float>();

    if (j.contains("emissive") && j["emissive"].is_array() && j["emissive"].size() == 3)
    {
        c.emissiveFactor.x = j["emissive"][0].get<float>();
        c.emissiveFactor.y = j["emissive"][1].get<float>();
        c.emissiveFactor.z = j["emissive"][2].get<float>();
    }
}

// ============================================================
// SceneSerializer
// ============================================================

bool SceneSerializer::SaveToFile(const SceneGraph& scene, const std::string& filePath)
{
    json root;
    root["version"] = 1;

    json entitiesArray = json::array();
    int entityCount = scene.GetEntityCount();

    for (int i = 0; i < entityCount; ++i)
    {
        const SceneEntity* entity = scene.GetEntity(i);
        if (!entity)
            continue; // skip removed slots

        json entityJson;
        entityJson["name"]    = entity->name;
        entityJson["visible"] = entity->visible;
        entityJson["parentIndex"] = entity->parentIndex;

        // Transform
        entityJson["transform"] = SerializeTransform(entity->transform);

        // Material override
        entityJson["useMaterialOverride"] = entity->useMaterialOverride;
        if (entity->useMaterialOverride)
        {
            entityJson["materialOverride"] = SerializeMaterial(entity->materialOverride);
        }

        entityJson["modelPath"] = entity->sourcePath;

        entitiesArray.push_back(entityJson);
    }

    root["entities"] = entitiesArray;
    root["selectedEntity"] = scene.selectedEntity;

    // Write to file
    std::ofstream file(filePath);
    if (!file.is_open())
        return false;

    file << root.dump(2);
    return file.good();
}

bool SceneSerializer::LoadFromFile(SceneGraph& scene, const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    json root;
    try
    {
        file >> root;
    }
    catch (const json::parse_error&)
    {
        return false;
    }

    if (!root.contains("entities") || !root["entities"].is_array())
        return false;

    // Load entities
    for (const auto& entityJson : root["entities"])
    {
        std::string name = entityJson.value("name", "Unnamed");
        int idx = scene.AddEntity(name);
        SceneEntity* entity = scene.GetEntity(idx);
        if (!entity)
            continue;

        entity->visible = entityJson.value("visible", true);
        entity->parentIndex = entityJson.value("parentIndex", -1);

        // Transform
        if (entityJson.contains("transform"))
        {
            DeserializeTransform(entity->transform, entityJson["transform"]);
        }

        // Material override
        entity->useMaterialOverride = entityJson.value("useMaterialOverride", false);
        if (entity->useMaterialOverride && entityJson.contains("materialOverride"))
        {
            DeserializeMaterial(entity->materialOverride, entityJson["materialOverride"]);
        }

        // Model path: stored for re-import by the application
        entity->sourcePath = entityJson.value("modelPath", "");
    }

    // Restore selection
    scene.selectedEntity = root.value("selectedEntity", -1);

    return true;
}
