#include "pch.h"
/// @file SceneSerializer.cpp
/// @brief シーン直列化実装

#include "Core/Scene/SceneSerializer.h"
#include "Core/Logger.h"
#include "ThirdParty/json.hpp"

using json = nlohmann::json;

namespace GX
{

// --- ヘルパー: XMFLOAT3 → JSON ---
static json Float3ToJson(const XMFLOAT3& v)
{
    return json::array({ v.x, v.y, v.z });
}

// --- ヘルパー: JSON → XMFLOAT3 ---
static XMFLOAT3 JsonToFloat3(const json& j)
{
    return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
}

// --- ヘルパー: XMFLOAT4 → JSON ---
static json Float4ToJson(const XMFLOAT4& v)
{
    return json::array({ v.x, v.y, v.z, v.w });
}

// --- ヘルパー: JSON → XMFLOAT4 ---
static XMFLOAT4 JsonToFloat4(const json& j)
{
    return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>() };
}

// --- エンティティ → JSON ---
static json SerializeEntity(const Entity& entity)
{
    json j;
    j["id"] = entity.GetID();
    j["name"] = entity.GetName();
    j["active"] = entity.IsActive();
    j["parent"] = entity.GetParent() ? static_cast<int>(entity.GetParent()->GetID()) : -1;

    // Transform
    auto& transform = entity.GetTransform();
    j["transform"]["position"] = Float3ToJson(transform.GetPosition());
    j["transform"]["rotation"] = Float3ToJson(transform.GetRotation());
    j["transform"]["scale"] = Float3ToJson(transform.GetScale());

    // Components
    json components = json::array();
    for (const auto& comp : entity.GetComponents())
    {
        json compJson;
        switch (comp->GetType())
        {
        case ComponentType::MeshRenderer:
        {
            auto* mr = static_cast<const MeshRendererComponent*>(comp.get());
            compJson["type"] = "MeshRenderer";
            compJson["castShadow"] = mr->castShadow;
            compJson["receiveShadow"] = mr->receiveShadow;
            break;
        }
        case ComponentType::SkinnedMeshRenderer:
        {
            compJson["type"] = "SkinnedMeshRenderer";
            break;
        }
        case ComponentType::Camera:
        {
            auto* cam = static_cast<const CameraComponent*>(comp.get());
            compJson["type"] = "Camera";
            compJson["isMain"] = cam->isMain;
            break;
        }
        case ComponentType::Light:
        {
            auto* light = static_cast<const LightComponent*>(comp.get());
            compJson["type"] = "Light";
            compJson["lightType"] = light->lightData.type;
            compJson["color"] = Float3ToJson(light->lightData.color);
            compJson["intensity"] = light->lightData.intensity;
            compJson["direction"] = Float3ToJson(light->lightData.direction);
            compJson["range"] = light->lightData.range;
            break;
        }
        case ComponentType::AudioSource:
        {
            auto* audio = static_cast<const AudioSourceComponent*>(comp.get());
            compJson["type"] = "AudioSource";
            compJson["playOnStart"] = audio->playOnStart;
            compJson["loop"] = audio->loop;
            break;
        }
        case ComponentType::Script:
        {
            compJson["type"] = "Script";
            break;
        }
        default:
            compJson["type"] = "Unknown";
            break;
        }
        compJson["enabled"] = comp->IsEnabled();
        components.push_back(compJson);
    }
    j["components"] = components;

    return j;
}

// --- JSON → エンティティ復元 ---
static void DeserializeEntity(Entity* entity, const json& j,
                               SceneSerializer::ModelLoadCallback modelLoader)
{
    entity->SetActive(j.value("active", true));

    // Transform
    if (j.contains("transform"))
    {
        const auto& t = j["transform"];
        if (t.contains("position"))
            entity->GetTransform().SetPosition(JsonToFloat3(t["position"]));
        if (t.contains("rotation"))
            entity->GetTransform().SetRotation(JsonToFloat3(t["rotation"]));
        if (t.contains("scale"))
            entity->GetTransform().SetScale(JsonToFloat3(t["scale"]));
    }

    // Components
    if (j.contains("components"))
    {
        for (const auto& compJson : j["components"])
        {
            std::string type = compJson.value("type", "Unknown");
            bool enabled = compJson.value("enabled", true);

            if (type == "MeshRenderer")
            {
                auto* mr = entity->AddComponent<MeshRendererComponent>();
                mr->castShadow = compJson.value("castShadow", true);
                mr->receiveShadow = compJson.value("receiveShadow", true);
                mr->SetEnabled(enabled);
            }
            else if (type == "SkinnedMeshRenderer")
            {
                auto* smr = entity->AddComponent<SkinnedMeshRendererComponent>();
                smr->SetEnabled(enabled);
            }
            else if (type == "Camera")
            {
                auto* cam = entity->AddComponent<CameraComponent>();
                cam->isMain = compJson.value("isMain", false);
                cam->SetEnabled(enabled);
            }
            else if (type == "Light")
            {
                auto* light = entity->AddComponent<LightComponent>();
                light->lightData.type = compJson.value("lightType", 0u);
                if (compJson.contains("color"))
                    light->lightData.color = JsonToFloat3(compJson["color"]);
                light->lightData.intensity = compJson.value("intensity", 1.0f);
                if (compJson.contains("direction"))
                    light->lightData.direction = JsonToFloat3(compJson["direction"]);
                light->lightData.range = compJson.value("range", 10.0f);
                light->SetEnabled(enabled);
            }
            else if (type == "AudioSource")
            {
                auto* audio = entity->AddComponent<AudioSourceComponent>();
                audio->playOnStart = compJson.value("playOnStart", false);
                audio->loop = compJson.value("loop", false);
                audio->SetEnabled(enabled);
            }
            else if (type == "Script")
            {
                auto* script = entity->AddComponent<ScriptComponent>();
                script->SetEnabled(enabled);
            }
        }
    }
}

// --- 公開API実装 ---

std::string SceneSerializer::ToJsonString(const Scene& scene)
{
    json root;
    root["scene"]["name"] = scene.GetName();

    json entities = json::array();
    for (const auto& entity : scene.GetEntities())
    {
        entities.push_back(SerializeEntity(*entity));
    }
    root["scene"]["entities"] = entities;

    return root.dump(2);
}

bool SceneSerializer::SaveToJson(const Scene& scene, const std::string& filePath)
{
    std::string jsonStr = ToJsonString(scene);
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        Logger::Error("SceneSerializer: Failed to open file for writing: %s", filePath.c_str());
        return false;
    }
    file << jsonStr;
    file.close();
    Logger::Info("SceneSerializer: Scene saved to %s", filePath.c_str());
    return true;
}

bool SceneSerializer::FromJsonString(Scene& scene, const std::string& jsonStr,
                                      ModelLoadCallback modelLoader)
{
    json root;
    try
    {
        root = json::parse(jsonStr);
    }
    catch (const json::parse_error& e)
    {
        Logger::Error("SceneSerializer: JSON parse error: %s", e.what());
        return false;
    }

    if (!root.contains("scene")) return false;
    const auto& sceneJson = root["scene"];

    scene.SetName(sceneJson.value("name", "Untitled"));

    // パス1: 全エンティティ作成
    std::unordered_map<int, Entity*> idToEntity;
    std::unordered_map<Entity*, int> entityParentId;

    if (sceneJson.contains("entities"))
    {
        for (const auto& entityJson : sceneJson["entities"])
        {
            std::string name = entityJson.value("name", "Entity");
            Entity* entity = scene.CreateEntity(name);

            int id = entityJson.value("id", 0);
            idToEntity[id] = entity;
            entityParentId[entity] = entityJson.value("parent", -1);

            DeserializeEntity(entity, entityJson, modelLoader);
        }
    }

    // パス2: 親子関係設定
    for (auto& [entity, parentId] : entityParentId)
    {
        if (parentId >= 0)
        {
            auto it = idToEntity.find(parentId);
            if (it != idToEntity.end())
            {
                entity->SetParent(it->second);
            }
        }
    }

    Logger::Info("SceneSerializer: Loaded %u entities", scene.GetEntityCount());
    return true;
}

bool SceneSerializer::LoadFromJson(Scene& scene, const std::string& filePath,
                                    ModelLoadCallback modelLoader)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        Logger::Error("SceneSerializer: Failed to open file: %s", filePath.c_str());
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    return FromJsonString(scene, content, modelLoader);
}

} // namespace GX
