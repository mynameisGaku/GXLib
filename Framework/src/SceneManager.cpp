/// @file SceneManager.cpp
/// @brief シーン切り替え補助の実装。遷移のタイミングを管理する。
#include "SceneManager.h"

namespace GXFW
{

void SceneManager::SetScene(std::unique_ptr<Scene> scene, SceneContext& ctx)
{
    if (!m_current)
    {
        m_current = std::move(scene);
        if (m_current)
            m_current->OnEnter(ctx);
        return;
    }

    m_pending = std::move(scene);
    m_hasPending = true;
}

void SceneManager::ApplyPending(SceneContext& ctx)
{
    if (!m_hasPending)
        return;

    if (m_current)
        m_current->OnExit(ctx);

    m_current = std::move(m_pending);
    m_hasPending = false;

    if (m_current)
        m_current->OnEnter(ctx);
}

void SceneManager::Update(SceneContext& ctx, float dt)
{
    ApplyPending(ctx);
    if (m_current)
        m_current->Update(ctx, dt);
}

void SceneManager::Render(SceneContext& ctx)
{
    if (m_current)
        m_current->Render(ctx);
}

void SceneManager::RenderUI(SceneContext& ctx)
{
    if (m_current)
        m_current->RenderUI(ctx);
}

} // namespace GXFW
