/// @file Prefab.cpp
/// @brief Prefab の実装
#include "pch_common.h"

#include "Core/Scene/Prefab.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Logger.h"
#include "ThirdParty/json.hpp"
#include <fstream>

using json = nlohmann::json;

namespace gx
{

// --- ヘルパー: XMFLOAT3 → JSON ---
static json PrefabFloat3ToJson(const XMFLOAT3& v)
{
    return json::array({ v.x, v.y, v.z });
}

static XMFLOAT3 PrefabJsonToFloat3(const json& j)
{
    return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
}

// --- エンティティ → JSON (簡易版: Transform + 名前のみ) ---
static json SerializePrefabEntity(const Entity& entity)
{
    json j;
    j["name"] = entity.GetName();
    j["active"] = entity.IsActive();

    auto& transform = entity.GetTransform();
    j["transform"]["position"] = PrefabFloat3ToJson(transform.GetPosition());
    j["transform"]["rotation"] = PrefabFloat3ToJson(transform.GetRotation());
    j["transform"]["scale"] = PrefabFloat3ToJson(transform.GetScale());

    // コンポーネント型リスト（復元用）
    json components = json::array();
    for (const auto& comp : entity.GetComponents())
    {
        json compJson;
        switch (comp->GetType())
        {
        case ComponentType::AudioSource:
        {
            compJson["type"] = "AudioSource";
            break;
        }
        case ComponentType::Script:
        {
            compJson["type"] = "Script";
            break;
        }
        default:
            compJson["type"] = static_cast<int>(comp->GetType());
            break;
        }
        components.push_back(compJson);
    }
    j["components"] = components;

    return j;
}

bool Prefab::CaptureFromEntity(const Entity& entity)
{
    try
    {
        json j = SerializePrefabEntity(entity);
        m_json = j.dump(2);
        return true;
    }
    catch (const std::exception& e)
    {
        GX_LOG_ERROR("Prefab::CaptureFromEntity failed: %s", e.what());
        return false;
    }
}

bool Prefab::LoadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        GX_LOG_ERROR("Prefab::LoadFromFile: Failed to open %s", filePath.c_str());
        return false;
    }

    try
    {
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        // JSONの妥当性を検証
        json::parse(content);
        m_json = std::move(content);
        return true;
    }
    catch (const std::exception& e)
    {
        GX_LOG_ERROR("Prefab::LoadFromFile parse error: %s", e.what());
        return false;
    }
}

bool Prefab::SaveToFile(const std::string& filePath) const
{
    if (m_json.empty()) return false;

    std::ofstream file(filePath);
    if (!file.is_open())
    {
        GX_LOG_ERROR("Prefab::SaveToFile: Failed to open %s", filePath.c_str());
        return false;
    }

    file << m_json;
    return true;
}

Entity* Prefab::Instantiate(Scene& scene, const std::string& name) const
{
    if (m_json.empty()) return nullptr;

    try
    {
        json j = json::parse(m_json);

        std::string entityName = name.empty() ? j.value("name", "PrefabInstance") : name;
        Entity* entity = scene.CreateEntity(entityName);
        if (!entity) return nullptr;

        entity->SetActive(j.value("active", true));

        // トランスフォームを復元
        if (j.contains("transform"))
        {
            auto& t = j["transform"];
            auto& transform = entity->GetTransform();
            if (t.contains("position"))
                transform.SetPosition(PrefabJsonToFloat3(t["position"]));
            if (t.contains("rotation"))
                transform.SetRotation(PrefabJsonToFloat3(t["rotation"]));
            if (t.contains("scale"))
                transform.SetScale(PrefabJsonToFloat3(t["scale"]));
        }

        return entity;
    }
    catch (const std::exception& e)
    {
        GX_LOG_ERROR("Prefab::Instantiate failed: %s", e.what());
        return nullptr;
    }
}

} // namespace gx
