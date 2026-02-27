#include "pch_graphics.h"
/// @file SceneRenderer.cpp
/// @brief シーン描画実装

#include "Graphics/3D/SceneRenderer.h"
#include "Graphics/3D/GraphicsComponents.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Core/Scene/Components.h"

namespace gx
{

void SceneRenderer::UpdateAnimations(Scene& scene, float deltaTime)
{
    for (const auto& entity : scene.GetEntities())
    {
        if (!entity->IsActive()) continue;

        auto* skinned = entity->GetComponent<SkinnedMeshRendererComponent>();
        if (skinned && skinned->IsEnabled() && skinned->animator)
        {
            skinned->animator->Update(deltaTime);
        }
    }
}

void SceneRenderer::Render(Scene& scene, Renderer3D& renderer)
{
    RenderInternal(scene, renderer, nullptr, nullptr);
}

void SceneRenderer::Render(Scene& scene, Renderer3D& renderer, const Camera3D& camera)
{
    XMMATRIX vp = camera.GetViewMatrix() * camera.GetProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);
    RenderInternal(scene, renderer, &frustum, &camera);
}

void SceneRenderer::RenderInternal(Scene& scene, Renderer3D& renderer,
                                     const Frustum* frustum, const Camera3D* camera)
{
    SceneRenderStats stats = {};

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

    const auto& entities = scene.GetEntities();

    for (const auto& entity : entities)
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
            if (!a.model) return false;
            if (!b.model) return true;
            if (a.model != b.model) return a.model < b.model;
            if ((a.materialOverride == nullptr) != (b.materialOverride == nullptr))
                return a.materialOverride == nullptr;
            return false;
        });

    // Draw remaining static models individually
    for (auto& entry : staticDraws)
    {
        if (!entry.model) continue;

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

} // namespace gx
