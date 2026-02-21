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
    return ptr;
}

void Scene::DestroyEntity(Entity* entity)
{
    if (!entity) return;
    m_pendingDestroy.push_back(entity);
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

    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive()) continue;
        ++stats.totalEntities;

        // フラスタムカリング
        if (frustum && entity->GetBounds().hasBounds)
        {
            Sphere worldSphere = entity->GetWorldBoundingSphere();
            if (!Collision3D::TestFrustumVsSphere(*frustum, worldSphere))
            {
                ++stats.culledEntities;
                continue;
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

    // Phase 4: Draw remaining static models individually
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
