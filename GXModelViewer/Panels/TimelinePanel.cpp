/// @file TimelinePanel.cpp
/// @brief Animation timeline panel implementation

#include "TimelinePanel.h"
#include <imgui.h>
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/AnimationClip.h"

void TimelinePanel::Draw(GX::Animator* animator, float deltaTime)
{
    if (!ImGui::Begin("Timeline"))
    {
        ImGui::End();
        return;
    }

    if (!animator)
    {
        ImGui::TextDisabled("No Animator assigned.");
        ImGui::End();
        return;
    }

    // --- Current clip info ---
    // We read the clip from the animator's internal state.
    // Animator exposes IsPlaying() but not the clip directly,
    // so we display what we can and control via our own time tracking.

    const bool animatorPlaying = animator->IsPlaying();
    const char* modeName = "Simple";
    if (animator->GetAnimMode() == GX::Animator::AnimMode::BlendStack)
        modeName = "BlendStack";
    else if (animator->GetAnimMode() == GX::Animator::AnimMode::StateMachine)
        modeName = "StateMachine";

    ImGui::Text("Mode: %s", modeName);
    ImGui::Text("Animator: %s", animatorPlaying ? "Playing" : "Stopped");
    ImGui::Separator();

    // --- Transport controls ---
    // Play
    if (ImGui::Button(m_playing ? "Pause" : "Play"))
    {
        m_playing = !m_playing;
        if (m_playing)
            animator->Resume();
        else
            animator->Pause();
    }

    ImGui::SameLine();

    // Stop
    if (ImGui::Button("Stop"))
    {
        m_playing = false;
        m_currentTime = 0.0f;
        animator->Stop();
    }

    ImGui::Separator();

    // --- Time display and scrub ---
    // We track time externally for the scrub slider.
    // In Simple mode, the animator manages its own time, but the slider
    // gives the user a visual representation.
    float duration = 0.0f;

    // Try to get duration from the animator's state info.
    // Since Animator doesn't expose the current clip's duration directly,
    // we provide a reasonable default and let the user see the time.
    // For StateMachine mode, we could query the current state's clip.
    // For now, use a fixed max that adjusts with playback.
    duration = 10.0f; // fallback

    // Advance our tracking time if playing
    if (m_playing)
    {
        m_currentTime += deltaTime * m_playbackSpeed;
        if (duration > 0.0f && m_currentTime > duration)
            m_currentTime = 0.0f; // wrap
    }

    ImGui::Text("Time: %.2f / %.2f s", m_currentTime, duration);

    // Scrub slider
    if (ImGui::SliderFloat("##Scrub", &m_currentTime, 0.0f, duration, "%.2f s"))
    {
        // User is scrubbing - pause playback
        m_playing = false;
        animator->Pause();
    }

    ImGui::Separator();

    // --- Playback speed ---
    ImGui::SliderFloat("Speed", &m_playbackSpeed, 0.1f, 3.0f, "%.1fx");

    if (ImGui::Button("1x"))
        m_playbackSpeed = 1.0f;
    ImGui::SameLine();
    if (ImGui::Button("0.5x"))
        m_playbackSpeed = 0.5f;
    ImGui::SameLine();
    if (ImGui::Button("2x"))
        m_playbackSpeed = 2.0f;

    ImGui::End();
}
