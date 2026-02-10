/// @file GameScene.cpp
/// @brief GameScene の実装。オブジェクト管理の流れをまとめる。

#include "GameScene.h"
#include "FrameworkApp.h"

#include <algorithm>
#include <utility>

namespace GXFW
{

void GameScene::OnEnter(SceneContext& ctx)
{
    m_ctx = &ctx;
    OnSceneEnter(ctx);
    FlushPendingAdds();
}

void GameScene::OnExit(SceneContext& ctx)
{
    OnSceneExit(ctx);

    for (auto& obj : m_objects)
    {
        if (obj)
            obj->OnDestroy();
    }
    m_objects.clear();
    m_pendingAdd.clear();
    m_ctx = nullptr;
}

void GameScene::Update(SceneContext& ctx, float dt)
{
    m_ctx = &ctx;
    OnSceneUpdate(ctx, dt);
    FlushPendingAdds();
    UpdateObjects(dt);
    DestroyPending();
}

void GameScene::Render(SceneContext& ctx)
{
    m_ctx = &ctx;
    OnSceneRender(ctx);
    RenderObjects();
}

void GameScene::RenderUI(SceneContext& ctx)
{
    m_ctx = &ctx;
    OnSceneRenderUI(ctx);
    RenderUIObjects();
}

void GameScene::ClearObjects()
{
    for (auto& obj : m_objects)
    {
        if (obj)
            obj->OnDestroy();
    }
    m_objects.clear();
    m_pendingAdd.clear();
}

void GameScene::QueueAdd(std::unique_ptr<GameObject> obj)
{
    if (!obj)
        return;

    if (m_ctx)
        obj->Attach(m_ctx);

    m_pendingAdd.push_back(std::move(obj));
}

void GameScene::FlushPendingAdds()
{
    if (m_pendingAdd.empty())
        return;

    for (auto& obj : m_pendingAdd)
    {
        if (!obj)
            continue;
        if (m_ctx)
            obj->Attach(m_ctx);
        m_objects.push_back(std::move(obj));
    }
    m_pendingAdd.clear();
}

void GameScene::UpdateObjects(float dt)
{
    for (auto& obj : m_objects)
    {
        if (!obj || !obj->IsActive())
            continue;
        obj->StartIfNeeded();
        obj->OnUpdate(dt);
    }
}

void GameScene::RenderObjects()
{
    for (auto& obj : m_objects)
    {
        if (!obj || !obj->IsActive())
            continue;
        obj->OnRender();
    }
}

void GameScene::RenderUIObjects()
{
    for (auto& obj : m_objects)
    {
        if (!obj || !obj->IsActive())
            continue;
        obj->OnRenderUI();
    }
}

void GameScene::DestroyPending()
{
    if (m_objects.empty())
        return;

    auto it = std::remove_if(m_objects.begin(), m_objects.end(),
                             [](const std::unique_ptr<GameObject>& obj)
                             {
                                 return obj && obj->IsPendingDestroy();
                             });
    for (auto iter = it; iter != m_objects.end(); ++iter)
    {
        if (*iter)
            (*iter)->OnDestroy();
    }
    m_objects.erase(it, m_objects.end());
}

} // namespace GXFW
