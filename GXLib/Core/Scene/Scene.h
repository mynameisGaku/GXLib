#pragma once
/// @file Scene.h
/// @brief シーン（エンティティコンテナ）

#include "pch.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/Components.h"

namespace GX
{

class Camera3D;
class Renderer3D;

/// @brief シーンのデバッグ描画フラグ
enum SceneDebugFlags : uint32_t
{
    SceneDebug_None            = 0,
    SceneDebug_BoundingSpheres = 1 << 0,
    SceneDebug_AABBs           = 1 << 1,
    SceneDebug_Frustum         = 1 << 2,
    SceneDebug_LODLevels       = 1 << 3,
};

/// @brief シーン（エンティティコンテナ＆更新・描画管理）
class Scene
{
public:
    /// @brief 自動インスタンシングの閾値（この数以上の同一モデルでインスタンシング発動）
    static constexpr uint32_t k_InstancingThreshold = 4;

    /// @brief 描画統計情報
    struct RenderStats
    {
        uint32_t totalEntities = 0;
        uint32_t visibleEntities = 0;
        uint32_t culledEntities = 0;
        uint32_t drawCalls = 0;
        uint32_t instancedBatches = 0;
        uint32_t instancedEntities = 0;
    };

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

    // --- シーン更新 ---
    void Update(float deltaTime);

    // --- Renderer3Dへの描画発行 ---
    /// @brief 全エンティティを描画（カリングなし — 後方互換）
    void Render(Renderer3D& renderer);

    /// @brief カメラ付き描画（フラスタムカリング有効）
    void Render(Renderer3D& renderer, const Camera3D& camera);

    /// @brief 直近のRender()呼び出しの描画統計を取得する
    RenderStats GetLastRenderStats() const { return m_lastRenderStats; }

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

private:
    /// @brief 内部描画（フラスタムがnullの場合はカリングなし）
    void RenderInternal(Renderer3D& renderer, const Frustum* frustum,
                         const Camera3D* camera = nullptr);

    std::string m_name;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<Entity*> m_rootEntities;
    uint32_t m_nextEntityID = 1;
    std::vector<Entity*> m_pendingDestroy;
    RenderStats m_lastRenderStats;
    uint32_t m_debugFlags = 0;
};

} // namespace GX
