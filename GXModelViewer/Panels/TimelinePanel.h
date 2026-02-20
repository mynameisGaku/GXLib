#pragma once
/// @file TimelinePanel.h
/// @brief Animation timeline panel with play/pause/scrub controls

namespace GX { class Animator; class Model; }

/// @brief ImGui panel for animation timeline playback control
class TimelinePanel
{
public:
    void Draw(GX::Animator* animator, const GX::Model* model,
              float deltaTime, int* selectedClipIndex);

private:
    bool  m_playing       = false;
    float m_playbackSpeed = 1.0f;
};
