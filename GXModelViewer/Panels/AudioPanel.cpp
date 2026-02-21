/// @file AudioPanel.cpp
/// @brief オーディオミキサー設定パネル実装
///
/// AudioMixerの定義済みバス（Master/BGM/SE/Voice）のボリュームを
/// スライダーで調整する。各AudioBus::SetVolume()で即座に反映される。

#include "AudioPanel.h"
#include <imgui.h>
#include "Audio/AudioMixer.h"
#include "Audio/AudioBus.h"

void AudioPanel::Draw(GX::AudioMixer& mixer)
{
    if (!ImGui::Begin("Audio"))
    {
        ImGui::End();
        return;
    }
    DrawContent(mixer);
    ImGui::End();
}

void AudioPanel::DrawContent(GX::AudioMixer& mixer)
{
    if (ImGui::CollapsingHeader("Buses", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Master Bus
        {
            float vol = mixer.GetMasterBus().GetVolume();
            if (ImGui::SliderFloat("Master", &vol, 0.0f, 1.0f))
                mixer.GetMasterBus().SetVolume(vol);
        }

        ImGui::Separator();

        // BGM Bus
        {
            float vol = mixer.GetBGMBus().GetVolume();
            if (ImGui::SliderFloat("BGM", &vol, 0.0f, 1.0f))
                mixer.GetBGMBus().SetVolume(vol);
        }

        // SE Bus
        {
            float vol = mixer.GetSEBus().GetVolume();
            if (ImGui::SliderFloat("SE", &vol, 0.0f, 1.0f))
                mixer.GetSEBus().SetVolume(vol);
        }

        // Voice Bus
        {
            float vol = mixer.GetVoiceBus().GetVolume();
            if (ImGui::SliderFloat("Voice", &vol, 0.0f, 1.0f))
                mixer.GetVoiceBus().SetVolume(vol);
        }
    }

    if (ImGui::CollapsingHeader("Info"))
    {
        ImGui::Text("Master Volume: %.2f", mixer.GetMasterBus().GetVolume());
        ImGui::Text("BGM Volume: %.2f", mixer.GetBGMBus().GetVolume());
        ImGui::Text("SE Volume: %.2f", mixer.GetSEBus().GetVolume());
        ImGui::Text("Voice Volume: %.2f", mixer.GetVoiceBus().GetVolume());
    }
}
