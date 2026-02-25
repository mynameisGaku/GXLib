#include "pch.h"
/// @file Scene.cpp
/// @brief シーン実装

#include "Core/Scene/Scene.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Core/Logger.h"

namespace GX
{

Scene::Scene(const std::string& name)
    : m_name(name)
{
}

Scene::~Scene() = default;

Entity* Scene::CreateEntity(const std::string& name)
{
    auto entity = std::make_unique<Entity>(name);
    entity->SetID(m_nextEntityID++);
    Entity* ptr = entity.get();
    m_entities.push_back(std::move(entity));
    m_rootEntities.push_back(ptr);
    m_spatialIndexDirty = true;
    return ptr;
}

void Scene::DestroyEntity(Entity* entity)
{
    if (!entity) return;
    m_pendingDestroy.push_back(entity);
    m_spatialIndexDirty = true;
}

Entity* Scene::FindEntity(const std::string& name) const
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

        // SkinnedMeshRenderer の Animator 更新
        auto* skinned = entity->GetComponent<SkinnedMeshRendererComponent>();
        if (skinned && skinned->IsEnabled() && skinned->animator)
        {
            skinned->animator->Update(deltaTime);
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

void Scene::Render(Renderer3D& renderer)
{
    RenderInternal(renderer, nullptr, nullptr);
}

void Scene::Render(Renderer3D& renderer, const Camera3D& camera)
{
    XMMATRIX vp = camera.GetViewMatrix() * camera.GetProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);
    RenderInternal(renderer, &frustum, &camera);
}

void Scene::RebuildSpatialIndex()
{
    // シーン全体を覆うAABBを計算
    Vector3 sceneMin(-500.0f, -500.0f, -500.0f);
    Vector3 sceneMax(500.0f, 500.0f, 500.0f);

    // エンティティの範囲からシーンバウンドを拡張
    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive() || !entity->GetBounds().hasBounds)
            continue;
        Sphere ws = entity->GetWorldBoundingSphere();
        Vector3 center(ws.center.x, ws.center.y, ws.center.z);
        Vector3 extent(ws.radius, ws.radius, ws.radius);
        Vector3 eMin = center - extent;
        Vector3 eMax = center + extent;
        sceneMin.x = (std::min)(sceneMin.x, eMin.x);
        sceneMin.y = (std::min)(sceneMin.y, eMin.y);
        sceneMin.z = (std::min)(sceneMin.z, eMin.z);
        sceneMax.x = (std::max)(sceneMax.x, eMax.x);
        sceneMax.y = (std::max)(sceneMax.y, eMax.y);
        sceneMax.z = (std::max)(sceneMax.z, eMax.z);
    }

    AABB3D sceneBounds(sceneMin, sceneMax);
    m_spatialIndex = std::make_unique<Octree<Entity*>>(sceneBounds, 6, 16);

    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive() || !entity->GetBounds().hasBounds)
            continue;
        Sphere ws = entity->GetWorldBoundingSphere();
        Vector3 center(ws.center.x, ws.center.y, ws.center.z);
        Vector3 extent(ws.radius, ws.radius, ws.radius);
        AABB3D entityAABB(center - extent, center + extent);
        m_spatialIndex->Insert(entity.get(), entityAABB);
    }

    m_spatialIndexDirty = false;
}

void Scene::RenderInternal(Renderer3D& renderer, const Frustum* frustum,
                            const Camera3D* camera)
{
    RenderStats stats = {};

    // Phase 1: Collect visible entities into draw lists
    struct StaticDrawEntry
    {
        const Model* model;
        Transform3D transform;
        const Material* materialOverride;
    };
    struct SkinnedDrawEntry
    {
        const Model* model;
        Transform3D transform;
        Animator* animator;
    };

    std::vector<StaticDrawEntry> staticDraws;
    std::vector<SkinnedDrawEntry> skinnedDraws;

    // フラスタムカリング: Octree空間インデックスを使用して高速化
    // バウンド付きエンティティはOctreeクエリで取得、バウンドなしは全探索
    std::vector<Entity*> octreeResults;
    bool useOctree = frustum && m_entities.size() > 32;

    if (useOctree)
    {
        // 空間インデックスが古い場合は再構築
        if (m_spatialIndexDirty || !m_spatialIndex)
            RebuildSpatialIndex();

        if (m_spatialIndex)
            m_spatialIndex->Query(*frustum, octreeResults);
    }

    // Octree結果をセットに変換してO(1)ルックアップ
    std::unordered_map<Entity*, bool> octreeVisible;
    if (useOctree)
    {
        for (Entity* e : octreeResults)
            octreeVisible[e] = true;
    }

    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive()) continue;
        ++stats.totalEntities;

        // フラスタムカリング
        if (frustum && entity->GetBounds().hasBounds)
        {
            if (useOctree && m_spatialIndex)
            {
                // Octreeで事前フィルタ済み — マップにないものはカリング
                if (octreeVisible.find(entity.get()) == octreeVisible.end())
                {
                    ++stats.culledEntities;
                    continue;
                }
            }
            else
            {
                // フォールバック: 球テスト
                Sphere worldSphere = entity->GetWorldBoundingSphere();
                if (!Collision3D::TestFrustumVsSphere(*frustum, worldSphere))
                {
                    ++stats.culledEntities;
                    continue;
                }
            }
        }

        ++stats.visibleEntities;

        // MeshRendererComponent（静的モデル）
        auto* meshRenderer = entity->GetComponent<MeshRendererComponent>();
        if (meshRenderer && meshRenderer->IsEnabled())
        {
            const Model* drawModel = meshRenderer->model;
            if (!drawModel && meshRenderer->ownedModel)
                drawModel = meshRenderer->ownedModel.get();

            // LOD選択（LODComponentがあればモデルを切り替え）
            auto* lodComp = entity->GetComponent<LODComponent>();
            if (lodComp && lodComp->IsEnabled() && camera)
            {
                float boundingRadius = entity->GetBounds().hasBounds
                    ? entity->GetBounds().boundingSphereRadius : 1.0f;
                Model* lodModel = lodComp->lodGroup.SelectLOD(
                    *camera, entity->GetTransform(), boundingRadius);
                if (lodModel)
                    drawModel = lodModel;
                else if (lodComp->lodGroup.GetLevelCount() > 0)
                    drawModel = nullptr;  // LODカリング
            }

            if (drawModel)
            {
                const Material* matOverride = nullptr;
                if (meshRenderer->useMaterialOverride)
                    matOverride = &meshRenderer->materialOverride;
                else if (!meshRenderer->materials.empty())
                    matOverride = &meshRenderer->materials[0];

                staticDraws.push_back({ drawModel, entity->GetTransform(), matOverride });
            }
        }

        // SkinnedMeshRendererComponent（スキンドモデルは常に個別描画）
        auto* skinnedRenderer = entity->GetComponent<SkinnedMeshRendererComponent>();
        if (skinnedRenderer && skinnedRenderer->IsEnabled())
        {
            const Model* drawModel = skinnedRenderer->model;
            if (!drawModel && skinnedRenderer->ownedModel)
                drawModel = skinnedRenderer->ownedModel.get();

            if (drawModel && skinnedRenderer->animator)
            {
                skinnedDraws.push_back({ drawModel, entity->GetTransform(),
                                          skinnedRenderer->animator.get() });
            }
        }
    }

    // Phase 2: Group static draws by model for instancing
    // マテリアルオーバーライドなしのエントリのみグループ化
    // （インスタンス描画はインスタンス毎マテリアルをサポートしないため）
    std::unordered_map<const Model*, std::vector<size_t>> modelGroups;
    for (size_t i = 0; i < staticDraws.size(); ++i)
    {
        if (!staticDraws[i].materialOverride)
        {
            modelGroups[staticDraws[i].model].push_back(i);
        }
    }

    // Phase 3: Draw instanced batches (k_InstancingThreshold 以上の同一モデル)
    for (auto& [model, indices] : modelGroups)
    {
        if (indices.size() >= k_InstancingThreshold)
        {
            std::vector<Transform3D> transforms;
            transforms.reserve(indices.size());
            for (size_t idx : indices)
                transforms.push_back(staticDraws[idx].transform);

            renderer.DrawModelInstanced(*model, transforms.data(),
                                        static_cast<uint32_t>(transforms.size()));
            ++stats.drawCalls;
            ++stats.instancedBatches;
            stats.instancedEntities += static_cast<uint32_t>(indices.size());

            // インスタンス描画済みとしてマーク
            for (size_t idx : indices)
                staticDraws[idx].model = nullptr;
        }
    }

    // Phase 4: Sort remaining static draws by model to minimize PSO/VB switches
    std::sort(staticDraws.begin(), staticDraws.end(),
        [](const StaticDrawEntry& a, const StaticDrawEntry& b) {
            // nullptrs (instanced draws) go to end
            if (!a.model) return false;
            if (!b.model) return true;
            // Group by model pointer (same model → same PSO/VB/IB)
            if (a.model != b.model) return a.model < b.model;
            // Within same model, non-override first (avoids override toggling)
            if ((a.materialOverride == nullptr) != (b.materialOverride == nullptr))
                return a.materialOverride == nullptr;
            return false;
        });

    // Draw remaining static models individually
    for (auto& entry : staticDraws)
    {
        if (!entry.model) continue; // インスタンス描画済み

        if (entry.materialOverride)
            renderer.SetMaterialOverride(entry.materialOverride);

        renderer.DrawModel(*entry.model, entry.transform);
        renderer.ClearMaterialOverride();
        ++stats.drawCalls;
    }

    // Phase 5: Draw skinned models individually
    for (auto& entry : skinnedDraws)
    {
        renderer.DrawSkinnedModel(*entry.model, entry.transform, *entry.animator);
        ++stats.drawCalls;
    }

    m_lastRenderStats = stats;
}

} // namespace GX
