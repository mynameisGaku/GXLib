#pragma once
/// @file AudioPanel.h
/// @brief オーディオミキサー設定パネル

namespace gx { class AudioMixer; }

/// @brief オーディオミキサー設定パネル
class AudioPanel
{
public:
    void Draw(gx::AudioMixer& mixer);
    void DrawContent(gx::AudioMixer& mixer);
};
