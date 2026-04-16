/// @file CutsceneSystem.cpp
/// @brief タイムラインベースのカットシーンシステム実装
#include "pch_common.h"
#include "Core/CutsceneSystem.h"
#include <algorithm>
#include <cmath>

namespace gx
{

void CutsceneSystem::AddAction(const CutsceneAction& action)
{
    m_actions.push_back(action);
}

void CutsceneSystem::Clear()
{
    m_actions.clear();
    m_state = CutsceneState::Stopped;
    m_currentTime = 0.0f;
    m_finishedFired = false;
}

void CutsceneSystem::Play()
{
    m_state = CutsceneState::Playing;
    m_currentTime = 0.0f;
    m_finishedFired = false;

    // 全アクションのランタイム状態をリセット
    for (auto& action : m_actions)
    {
        action.started = false;
        action.completed = false;
    }
}

void CutsceneSystem::Pause()
{
    if (m_state == CutsceneState::Playing)
    {
        m_state = CutsceneState::Paused;
    }
}

void CutsceneSystem::Resume()
{
    if (m_state == CutsceneState::Paused)
    {
        m_state = CutsceneState::Playing;
    }
}

void CutsceneSystem::Stop()
{
    m_state = CutsceneState::Stopped;
    m_currentTime = 0.0f;
    m_finishedFired = false;

    for (auto& action : m_actions)
    {
        action.started = false;
        action.completed = false;
    }
}

void CutsceneSystem::Update(float deltaTime)
{
    if (m_state != CutsceneState::Playing)
        return;

    m_currentTime += deltaTime;

    for (auto& action : m_actions)
    {
        if (action.completed)
            continue;

        // アクションを開始すべきか確認
        if (!action.started && m_currentTime >= action.startTime)
        {
            action.started = true;

            // 即時アクションの処理
            switch (action.type)
            {
            case CutsceneActionType::PlaySound:
                if (m_callbacks.onPlaySound)
                    m_callbacks.onPlaySound(action.soundName);
                action.completed = true;
                break;

            case CutsceneActionType::ShowDialogue:
                if (m_callbacks.onShowDialogue)
                    m_callbacks.onShowDialogue(action.speakerName, action.dialogueText);
                action.completed = true;
                break;

            case CutsceneActionType::Custom:
                if (action.callback)
                    action.callback();
                action.completed = true;
                break;

            default:
                break;
            }
        }

        // 継続アクションの処理（移動、回転、フェード、待機）
        if (action.started && !action.completed)
        {
            float elapsed = m_currentTime - action.startTime;
            float progress = 0.0f;

            if (action.duration > 0.0f)
            {
                progress = elapsed / action.duration;
                progress = (std::min)(progress, 1.0f);
                progress = (std::max)(progress, 0.0f);
            }
            else
            {
                progress = 1.0f;
            }

            switch (action.type)
            {
            case CutsceneActionType::MoveTo:
                if (m_callbacks.onMoveTo)
                {
                    Vector3 interpolated;
                    interpolated.x = action.targetPosition.x * progress;
                    interpolated.y = action.targetPosition.y * progress;
                    interpolated.z = action.targetPosition.z * progress;
                    m_callbacks.onMoveTo(interpolated, progress);
                }
                break;

            case CutsceneActionType::RotateTo:
                if (m_callbacks.onRotateTo)
                {
                    Vector3 interpolated;
                    interpolated.x = action.targetRotation.x * progress;
                    interpolated.y = action.targetRotation.y * progress;
                    interpolated.z = action.targetRotation.z * progress;
                    m_callbacks.onRotateTo(interpolated, progress);
                }
                break;

            case CutsceneActionType::FadeTo:
                if (m_callbacks.onFadeTo)
                {
                    float interpolatedAlpha = action.targetAlpha * progress;
                    m_callbacks.onFadeTo(interpolatedAlpha);
                }
                break;

            case CutsceneActionType::Wait:
                // 待機は単に時間が経過するのを待つだけ
                break;

            default:
                break;
            }

            if (progress >= 1.0f)
            {
                action.completed = true;
            }
        }
    }

    // 全アクションが完了したか確認
    if (!m_finishedFired && IsFinished())
    {
        m_finishedFired = true;
        if (m_onFinished)
            m_onFinished();
    }
}

float CutsceneSystem::GetTotalDuration() const
{
    float maxEnd = 0.0f;
    for (const auto& action : m_actions)
    {
        float end = action.startTime + action.duration;
        if (end > maxEnd)
            maxEnd = end;
    }
    return maxEnd;
}

float CutsceneSystem::GetProgress() const
{
    float total = GetTotalDuration();
    if (total <= 0.0f)
        return 0.0f;

    float progress = m_currentTime / total;
    return (std::min)((std::max)(progress, 0.0f), 1.0f);
}

bool CutsceneSystem::IsFinished() const
{
    if (m_actions.empty())
        return true;

    for (const auto& action : m_actions)
    {
        if (!action.completed)
            return false;
    }
    return true;
}

} // namespace gx
