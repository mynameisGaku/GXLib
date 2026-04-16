/// @file SceneSnapshot.cpp
/// @brief SceneSnapshot の実装
#include "pch_common.h"

#include "Core/Scene/SceneSnapshot.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include "Core/Logger.h"

namespace gx
{

void SceneSnapshot::Capture(const Scene& scene)
{
    m_entityData.clear();
    m_sceneName = scene.GetName();

    const auto& entities = scene.GetEntities();
    m_entityData.reserve(entities.size());

    for (const auto& entity : entities)
    {
        EntityData data;
        data.id = entity->GetID();
        data.name = entity->GetName();
        data.active = entity->IsActive();

        // Transform3D の位置・回転・スケールを保存
        const auto& pos = entity->GetTransform().GetPosition();
        data.posX = pos.x;
        data.posY = pos.y;
        data.posZ = pos.z;

        const auto& rot = entity->GetTransform().GetRotation();
        data.rotX = rot.x;
        data.rotY = rot.y;
        data.rotZ = rot.z;

        const auto& scl = entity->GetTransform().GetScale();
        data.scaleX = scl.x;
        data.scaleY = scl.y;
        data.scaleZ = scl.z;

        // 親の名前を保存（階層復元用）
        if (entity->GetParent())
        {
            data.parentName = entity->GetParent()->GetName();
        }

        // 非グラフィクスコンポーネントを保存
        for (const auto& comp : entity->GetComponents())
        {
            ComponentData cd;
            cd.type = static_cast<uint32_t>(comp->GetType());
            cd.enabled = comp->IsEnabled();

            switch (comp->GetType())
            {
            case ComponentType::AudioSource:
            {
                auto* audio = static_cast<const AudioSourceComponent*>(comp.get());
                cd.soundHandle = audio->soundHandle;
                cd.playOnStart = audio->playOnStart;
                cd.loop = audio->loop;
                break;
            }
            case ComponentType::Script:
            {
                // スクリプトの enabled 状態のみ保存（コールバックは復元不可）
                break;
            }
            case ComponentType::Transform:
            {
                auto* tc = static_cast<const TransformComponent*>(comp.get());
                cd.position = { tc->position.x, tc->position.y, tc->position.z };
                cd.rotation = { tc->rotation.x, tc->rotation.y, tc->rotation.z };
                cd.scale = { tc->scale.x, tc->scale.y, tc->scale.z };
                break;
            }
            case ComponentType::Custom:
            {
                // NameComponent / TagComponent は Custom タイプを共有
                // dynamic_cast で判別
                if (auto* nc = dynamic_cast<const NameComponent*>(comp.get()))
                {
                    cd.name = nc->name;
                }
                else if (auto* tc = dynamic_cast<const TagComponent*>(comp.get()))
                {
                    cd.tag = tc->tag;
                }
                break;
            }
            default:
                break;
            }

            data.components.push_back(std::move(cd));
        }

        m_entityData.push_back(std::move(data));
    }

    m_valid = true;
    GX_LOG_INFO("[SceneSnapshot] Captured {} entities from scene '{}'",
                m_entityData.size(), m_sceneName);
}

void SceneSnapshot::Restore(Scene& scene) const
{
    if (!m_valid)
    {
        GX_LOG_WARN("[SceneSnapshot] Restore called on invalid snapshot");
        return;
    }

    // 既存エンティティを全て破棄する
    // DestroyEntityはpendingなので、直接クリアするためにUpdateを使わない
    // 代わりに、各エンティティを破棄予約してからUpdateで処理する
    {
        gx::Vector<Entity*> toDestroy;
        for (const auto& entity : scene.GetEntities())
        {
            toDestroy.push_back(entity.get());
        }
        for (auto* e : toDestroy)
        {
            scene.DestroyEntity(e);
        }
        // pending destroyを即時処理
        scene.Update(0.0f);
    }

    scene.SetName(m_sceneName);

    // エンティティを再作成（まず全エンティティを作る）
    for (const auto& data : m_entityData)
    {
        Entity* entity = scene.CreateEntity(data.name);
        entity->SetID(data.id);
        entity->SetActive(data.active);

        // Transform3D を復元
        entity->GetTransform().SetPosition(data.posX, data.posY, data.posZ);
        entity->GetTransform().SetRotation(data.rotX, data.rotY, data.rotZ);
        entity->GetTransform().SetScale(data.scaleX, data.scaleY, data.scaleZ);

        // コンポーネントを復元
        for (const auto& cd : data.components)
        {
            ComponentType type = static_cast<ComponentType>(cd.type);
            switch (type)
            {
            case ComponentType::AudioSource:
            {
                auto* audio = entity->AddComponent<AudioSourceComponent>();
                audio->soundHandle = cd.soundHandle;
                audio->playOnStart = cd.playOnStart;
                audio->loop = cd.loop;
                audio->SetEnabled(cd.enabled);
                break;
            }
            case ComponentType::Script:
            {
                auto* script = entity->AddComponent<ScriptComponent>();
                script->SetEnabled(cd.enabled);
                break;
            }
            case ComponentType::Transform:
            {
                auto* tc = entity->AddComponent<TransformComponent>();
                tc->position = { cd.position.x, cd.position.y, cd.position.z };
                tc->rotation = { cd.rotation.x, cd.rotation.y, cd.rotation.z };
                tc->scale = { cd.scale.x, cd.scale.y, cd.scale.z };
                tc->SetEnabled(cd.enabled);
                break;
            }
            case ComponentType::Custom:
            {
                // NameComponent か TagComponent かを判別
                if (!cd.name.empty())
                {
                    auto* nc = entity->AddComponent<NameComponent>();
                    nc->name = cd.name;
                    nc->SetEnabled(cd.enabled);
                }
                else if (!cd.tag.empty())
                {
                    auto* tc = entity->AddComponent<TagComponent>();
                    tc->tag = cd.tag;
                    tc->SetEnabled(cd.enabled);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    // 親子関係を復元
    for (const auto& data : m_entityData)
    {
        if (!data.parentName.empty())
        {
            Entity* child = scene.FindEntity(data.name);
            Entity* parent = scene.FindEntity(data.parentName);
            if (child && parent)
            {
                child->SetParent(parent);
            }
        }
    }

    GX_LOG_INFO("[SceneSnapshot] Restored {} entities to scene '{}'",
                m_entityData.size(), m_sceneName);
}

} // namespace gx
