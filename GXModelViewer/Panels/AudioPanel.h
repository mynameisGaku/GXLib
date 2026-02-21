#pragma once
/// @file AudioPanel.h
/// @brief オーディオミキサー設定パネル

namespace GX { class AudioMixer; }

/// @brief オーディオミキサー設定パネル
class AudioPanel
{
public:
    void Draw(GX::AudioMixer& mixer);
    void DrawContent(GX::AudioMixer& mixer);
};
