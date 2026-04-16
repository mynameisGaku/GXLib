#include "pch_common.h"
/// @file Scene.cpp
/// @brief シーン実装（エンティティ管理＋スクリプト更新）

#include "Core/Scene/Scene.h"
#include "Core/Logger.h"

namespace gx
{

Scene::Scene(const gx::String& name)
    : m_name(name)
{
}

Scene::~Scene() = default;

Entity* Scene::CreateEntity(const gx::String& name)
{
    auto entity = std::make_unique<Entity>(name);
    entity->SetID(m_nextEntityID++);
    Entity* ptr = entity.get();
    m_entities.push_back(std::move(entity));
    m_rootEntities.push_back(ptr);
    return ptr;
}

void Scene::DestroyEntity(Entity* entity)
{
    if (!entity) return;
    m_pendingDestroy.push_back(entity);
}

Entity* Scene::FindEntity(const gx::String& name) const
{
    for (const auto& entity : m_entities)
    {
        if (entity->GetName() == name)
            return entity.get();
    }
    return nullptr;
}

Entity* Scene::FindEntityByID(uint32_t id) const
{
    for (const auto& entity : m_entities)
    {
        if (entity->GetID() == id)
            return entity.get();
    }
    return nullptr;
}

void Scene::Update(float deltaTime)
{
    // ScriptComponent のスタートと更新
    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive()) continue;

        auto* script = entity->GetComponent<ScriptComponent>();
        if (script && script->IsEnabled())
        {
            if (!script->started && script->onStart)
            {
                script->onStart();
                script->started = true;
            }
            if (script->onUpdate)
            {
                script->onUpdate(deltaTime);
            }
        }
    }

    // 保留中の破棄を処理
    for (auto* entity : m_pendingDestroy)
    {
        // ScriptComponent の onDestroy 呼び出し
        auto* script = entity->GetComponent<ScriptComponent>();
        if (script && script->onDestroy)
        {
            script->onDestroy();
        }

        // ルートエンティティリストから除去
        m_rootEntities.erase(
            std::remove(m_rootEntities.begin(), m_rootEntities.end(), entity),
            m_rootEntities.end());

        // エンティティリストから除去
        m_entities.erase(
            std::remove_if(m_entities.begin(), m_entities.end(),
                [entity](const std::unique_ptr<Entity>& e) { return e.get() == entity; }),
            m_entities.end());
    }
    m_pendingDestroy.clear();
}

} // namespace gx
