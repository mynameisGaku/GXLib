/// @file EntityBridge.cpp
/// @brief EntityBridge の実装
#include "pch_common.h"
#include "ECS/EntityBridge.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Logger.h"

#include <cstring>

namespace gx { namespace ecs {

// ---------------------------------------------------------------------------
// 単一のレガシーEntityをインポート
// ---------------------------------------------------------------------------
EntityID EntityBridge::ImportEntity(World& world, const Entity& entity)
{
    uint32_t sceneId = entity.GetID();

    // 既にインポート済みか確認
    auto it = s_entityMap.find(sceneId);
    if (it != s_entityMap.end())
        return it->second;

    EntityID ecsId = world.CreateEntity();

    // 位置
    {
        auto& pos = world.AddComponent<BridgePosition>(ecsId);
        const Vector3& p = entity.GetTransform().GetPosition();
        pos.x = p.x;
        pos.y = p.y;
        pos.z = p.z;
    }

    // 回転（Transform3Dからのオイラー角）
    {
        auto& rot = world.AddComponent<BridgeRotation>(ecsId);
        const Vector3& r = entity.GetTransform().GetRotation();
        rot.x = r.x;
        rot.y = r.y;
        rot.z = r.z;
    }

    // スケール
    {
        auto& scl = world.AddComponent<BridgeScale>(ecsId);
        const Vector3& s = entity.GetTransform().GetScale();
        scl.x = s.x;
        scl.y = s.y;
        scl.z = s.z;
    }

    // 名前
    {
        auto& nm = world.AddComponent<BridgeName>(ecsId);
        strncpy(nm.name, entity.GetName().c_str(), sizeof(nm.name) - 1);
        nm.name[sizeof(nm.name) - 1] = '\0';
    }

    // マッピングを記録
    s_entityMap[sceneId] = ecsId;
    s_reverseMap[ecsId] = sceneId;

    return ecsId;
}

// ---------------------------------------------------------------------------
// ECSエンティティをレガシーEntityにエクスポート
// ---------------------------------------------------------------------------
void EntityBridge::ExportEntity(const World& world, EntityID ecsEntity, Entity& entity)
{
    // GetComponentにはnon-const Worldが必要なためconstを除去
    // （GetComponentはデータを読むだけだが、アーキタイプ検索のためAPIがnon-const）
    World& w = const_cast<World&>(world);

    auto* pos = w.GetComponent<BridgePosition>(ecsEntity);
    if (pos)
    {
        entity.GetTransform().SetPosition(pos->x, pos->y, pos->z);
    }

    auto* rot = w.GetComponent<BridgeRotation>(ecsEntity);
    if (rot)
    {
        entity.GetTransform().SetRotation(rot->x, rot->y, rot->z);
    }

    auto* scl = w.GetComponent<BridgeScale>(ecsEntity);
    if (scl)
    {
        entity.GetTransform().SetScale(scl->x, scl->y, scl->z);
    }

    auto* nm = w.GetComponent<BridgeName>(ecsEntity);
    if (nm)
    {
        entity.SetName(nm->name);
    }
}

// ---------------------------------------------------------------------------
// 同期: Scene -> World
// ---------------------------------------------------------------------------
void EntityBridge::SyncSceneToWorld(World& world, const Scene& scene)
{
    for (const auto& entityPtr : scene.GetEntities())
    {
        ImportEntity(world, *entityPtr);
    }
    GX_LOG_INFO("EntityBridge: synced %u entities from Scene to World",
                static_cast<uint32_t>(scene.GetEntities().size()));
}

// ---------------------------------------------------------------------------
// 同期: World -> Scene
// ---------------------------------------------------------------------------
void EntityBridge::SyncWorldToScene(const World& world, Scene& scene)
{
    uint32_t count = 0;
    for (const auto& [sceneId, ecsId] : s_entityMap)
    {
        Entity* entity = scene.FindEntityByID(sceneId);
        if (entity)
        {
            ExportEntity(world, ecsId, *entity);
            ++count;
        }
    }
    GX_LOG_INFO("EntityBridge: synced %u entities from World to Scene", count);
}

}} // namespace gx::ecs
