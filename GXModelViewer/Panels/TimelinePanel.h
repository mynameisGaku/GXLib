#pragma once
/// @file TimelinePanel.h
/// @brief Animation timeline panel with play/pause/scrub controls

namespace GX { class Animator; }

/// @brief ImGui panel for animation timeline playback control
class TimelinePanel
{
public:
    /// Draw the timeline panel.
    /// @param animator The animator to control (may be nullptr).
    /// @param deltaTime Frame delta time in seconds.
    void Draw(GX::Animator* animator, float deltaTime);

private:
    bool  m_playing       = false;
    float m_currentTime   = 0.0f;
    float m_playbackSpeed = 1.0f;
};
