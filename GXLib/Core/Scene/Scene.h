#pragma once
/// @file Scene.h
/// @brief シーン（エンティティコンテナ）
///
/// エンティティの管理（作成・破棄・検索）とスクリプト更新を担当する。
/// 描画ロジックは Graphics/3D/SceneRenderer に分離されている。

#include "pch_common.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/Components.h"

namespace gx
{
/// @addtogroup grp_scene
/// @{

/// @brief シーンのデバッグ描画フラグ
enum SceneDebugFlags : uint32_t
{
    SceneDebug_None            = 0,
    SceneDebug_BoundingSpheres = 1 << 0,
    SceneDebug_AABBs           = 1 << 1,
    SceneDebug_Frustum         = 1 << 2,
    SceneDebug_LODLevels       = 1 << 3,
};

/// @brief シーン（エンティティコンテナ＆スクリプト更新）
class Scene
{
public:
    Scene(const std::string& name = "Untitled");
    ~Scene();

    // --- エンティティ管理 ---
    Entity* CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity* entity);
    Entity* FindEntity(const std::string& name) const;
    Entity* FindEntityByID(uint32_t id) const;
    const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_entities; }

    // --- 階層ルートエンティティ ---
    const std::vector<Entity*>& GetRootEntities() const { return m_rootEntities; }

    // --- シーン更新（スクリプトコンポーネントのみ）---
    void Update(float deltaTime);

    /// @brief デバッグ描画フラグ
    void SetDebugDrawFlags(uint32_t flags) { m_debugFlags = flags; }
    uint32_t GetDebugDrawFlags() const { return m_debugFlags; }

    // --- シーン名 ---
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    // --- コンポーネント検索 ---
    template<typename T>
    std::vector<T*> FindComponentsOfType() const
    {
        std::vector<T*> result;
        for (const auto& entity : m_entities)
        {
            T* comp = entity->GetComponent<T>();
            if (comp) result.push_back(comp);
        }
        return result;
    }

    /// @brief エンティティ数を取得する
    uint32_t GetEntityCount() const { return static_cast<uint32_t>(m_entities.size()); }

    /// @brief 別シーンの全エンティティを移動・統合する（アディティブ読み込み用）
    void MergeFrom(Scene& other);

    /// @brief 全エンティティを即座に破棄する
    void ClearAllEntities();

private:
    std::string m_name;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<Entity*> m_rootEntities;
    uint32_t m_nextEntityID = 1;
    std::vector<Entity*> m_pendingDestroy;
    uint32_t m_debugFlags = 0;
};

/// @}
} // namespace gx
