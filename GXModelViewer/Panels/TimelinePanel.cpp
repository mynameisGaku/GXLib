/// @file TimelinePanel.cpp
/// @brief タイムラインパネル実装
///
/// クリップ選択コンボ、トランスポートボタン（巻戻し/再生/停止）、
/// タイムスクラブスライダー、速度プリセット(0.25x〜4x)、ルートロック制御を表示する。

#include "TimelinePanel.h"
#include <imgui.h>
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Model.h"

void TimelinePanel::Draw(GX::Animator* animator, const GX::Model* model,
                          float deltaTime, int* selectedClipIndex)
{
    if (!ImGui::Begin("Timeline"))
    {
        ImGui::End();
        return;
    }

    if (!animator || !model)
    {
        ImGui::TextDisabled("No animated model selected.");
        ImGui::End();
        return;
    }

    // --- Animation clip selector ---
    const auto& animations = model->GetAnimations();
    uint32_t animCount = model->GetAnimationCount();

    if (animCount > 0 && selectedClipIndex)
    {
        // Build combo items
        int curIdx = *selectedClipIndex;
        if (curIdx < 0) curIdx = 0;

        const char* previewName = (curIdx < static_cast<int>(animCount))
            ? animations[curIdx].GetName().c_str()
            : "---";

        if (ImGui::BeginCombo("Animation Clip", previewName))
        {
            for (uint32_t i = 0; i < animCount; ++i)
            {
                bool selected = (static_cast<int>(i) == curIdx);
                char label[128];
                snprintf(label, sizeof(label), "[%u] %s (%.2fs)",
                         i, animations[i].GetName().c_str(), animations[i].GetDuration());
                if (ImGui::Selectable(label, selected))
                {
                    *selectedClipIndex = static_cast<int>(i);
                    animator->Play(&animations[i], true, m_playbackSpeed);
                    m_playing = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    else
    {
        ImGui::TextDisabled("No animation clips available.");
    }

    ImGui::Separator();

    // --- Current clip info ---
    const GX::AnimationClip* currentClip = animator->GetCurrentClip();
    float duration = currentClip ? currentClip->GetDuration() : 0.0f;
    float currentTime = animator->GetCurrentTime();

    const char* modeName = "Simple";
    if (animator->GetAnimMode() == GX::Animator::AnimMode::BlendStack)
        modeName = "BlendStack";
    else if (animator->GetAnimMode() == GX::Animator::AnimMode::StateMachine)
        modeName = "StateMachine";

    ImGui::Text("Mode: %s  |  %s", modeName,
                animator->IsPlaying() ? (animator->IsPaused() ? "Paused" : "Playing") : "Stopped");

    // --- Transport controls ---
    // Rewind
    if (ImGui::Button("|<"))
    {
        animator->SetCurrentTime(0.0f);
    }
    ImGui::SameLine();

    // Play/Pause toggle
    if (ImGui::Button(m_playing && !animator->IsPaused() ? "||" : ">"))
    {
        if (!animator->IsPlaying() && currentClip)
        {
            animator->Play(currentClip, true, m_playbackSpeed);
            m_playing = true;
        }
        else if (animator->IsPaused())
        {
            animator->Resume();
            m_playing = true;
        }
        else
        {
            animator->Pause();
            m_playing = false;
        }
    }
    ImGui::SameLine();

    // Stop
    if (ImGui::Button("[]"))
    {
        m_playing = false;
        animator->Stop();
        animator->SetCurrentTime(0.0f);
    }

    // --- Time display and scrub ---
    ImGui::Text("Time: %.3f / %.3f s", currentTime, duration);

    if (duration > 0.0f)
    {
        float scrubTime = currentTime;
        if (ImGui::SliderFloat("##Scrub", &scrubTime, 0.0f, duration, "%.3f s"))
        {
            animator->SetCurrentTime(scrubTime);
            if (!animator->IsPlaying() && currentClip)
            {
                animator->Play(currentClip, true, 0.0f);
                animator->Pause();
            }
            m_playing = false;
        }
    }

    ImGui::Separator();

    // --- Playback speed ---
    if (ImGui::SliderFloat("Speed", &m_playbackSpeed, 0.0f, 4.0f, "%.2fx"))
    {
        animator->SetSpeed(m_playbackSpeed);
    }

    // Speed presets
    const float presets[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
    const char* presetLabels[] = { "0.25x", "0.5x", "1x", "2x", "4x" };
    for (int i = 0; i < 5; ++i)
    {
        if (i > 0) ImGui::SameLine();
        if (ImGui::Button(presetLabels[i]))
        {
            m_playbackSpeed = presets[i];
            animator->SetSpeed(m_playbackSpeed);
        }
    }

    ImGui::Separator();
    {
        bool lockPos = animator->IsRootPositionLocked();
        if (ImGui::Checkbox("Lock Root Position", &lockPos))
            animator->SetLockRootPosition(lockPos);
        ImGui::SameLine();
        bool lockRot = animator->IsRootRotationLocked();
        if (ImGui::Checkbox("Lock Root Rotation", &lockRot))
            animator->SetLockRootRotation(lockRot);
    }

    ImGui::End();
}
