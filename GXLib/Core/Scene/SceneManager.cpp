/// @file SceneManager.cpp
/// @brief SceneManager の実装
#include "pch_common.h"
#include "Core/Scene/SceneManager.h"

namespace gx
{

void SceneManager::ChangeScene(SceneFactory factory, const SceneTransitionDesc& desc)
{
    BeginTransition(std::move(factory), desc, true);
}

void SceneManager::PushScene(SceneFactory factory, const SceneTransitionDesc& desc)
{
    BeginTransition(std::move(factory), desc, false);
}

void SceneManager::PopScene(const SceneTransitionDesc& desc)
{
    if (m_sceneStack.empty()) return;

    m_pendingFactory = nullptr;
    m_pendingAction = PendingAction::Pop;
    m_fadeOutDuration = desc.fadeOutDuration;
    m_fadeInDuration = desc.fadeInDuration;

    if (m_fadeOutDuration > 0.0f)
    {
        m_transitionState = SceneTransitionState::FadeOut;
        m_transitionTimer = 0.0f;
        m_transitionAlpha = 0.0f;
    }
    else
    {
        m_sceneStack.pop_back();
        if (m_fadeInDuration > 0.0f)
        {
            m_transitionState = SceneTransitionState::FadeIn;
            m_transitionTimer = 0.0f;
            m_transitionAlpha = 1.0f;
        }
    }
}

void SceneManager::Update(float deltaTime)
{
    switch (m_transitionState)
    {
    case SceneTransitionState::None:
        if (!m_sceneStack.empty())
            m_sceneStack.back()->Update(deltaTime);
        break;

    case SceneTransitionState::FadeOut:
        m_transitionTimer += deltaTime;
        m_transitionAlpha = (m_fadeOutDuration > 0.0f)
            ? std::min(m_transitionTimer / m_fadeOutDuration, 1.0f)
            : 1.0f;
        if (m_transitionAlpha >= 1.0f)
        {
            m_transitionState = SceneTransitionState::Loading;
        }
        break;

    case SceneTransitionState::Loading:
        FinishTransition();
        if (m_fadeInDuration > 0.0f)
        {
            m_transitionState = SceneTransitionState::FadeIn;
            m_transitionTimer = 0.0f;
            m_transitionAlpha = 1.0f;
        }
        else
        {
            m_transitionState = SceneTransitionState::None;
            m_transitionAlpha = 0.0f;
        }
        break;

    case SceneTransitionState::FadeIn:
        m_transitionTimer += deltaTime;
        m_transitionAlpha = (m_fadeInDuration > 0.0f)
            ? std::max(1.0f - m_transitionTimer / m_fadeInDuration, 0.0f)
            : 0.0f;
        if (m_transitionAlpha <= 0.0f)
        {
            m_transitionState = SceneTransitionState::None;
            m_transitionAlpha = 0.0f;
        }
        if (!m_sceneStack.empty())
            m_sceneStack.back()->Update(deltaTime);
        break;
    }
}

Scene* SceneManager::GetCurrentScene() const
{
    if (m_sceneStack.empty()) return nullptr;
    return m_sceneStack.back().get();
}

void SceneManager::BeginTransition(SceneFactory factory, const SceneTransitionDesc& desc, bool clearStack)
{
    m_pendingFactory = std::move(factory);
    m_pendingAction = clearStack ? PendingAction::Change : PendingAction::Push;
    m_fadeOutDuration = desc.fadeOutDuration;
    m_fadeInDuration = desc.fadeInDuration;

    if (m_fadeOutDuration > 0.0f && !m_sceneStack.empty())
    {
        m_transitionState = SceneTransitionState::FadeOut;
        m_transitionTimer = 0.0f;
        m_transitionAlpha = 0.0f;
    }
    else
    {
        m_transitionState = SceneTransitionState::Loading;
    }
}

void SceneManager::FinishTransition()
{
    switch (m_pendingAction)
    {
    case PendingAction::Change:
        m_sceneStack.clear();
        if (m_pendingFactory)
            m_sceneStack.push_back(m_pendingFactory());
        break;

    case PendingAction::Push:
        if (m_pendingFactory)
            m_sceneStack.push_back(m_pendingFactory());
        break;

    case PendingAction::Pop:
        if (!m_sceneStack.empty())
            m_sceneStack.pop_back();
        break;
    }
    m_pendingFactory = nullptr;
}

} // namespace gx
