/// @file SkyboxPanel.cpp
/// @brief スカイボックスパネル実装
///
/// プロシージャルスカイの上下カラー・太陽方向/強度を編集し、
/// 変更時にSkybox::SetColors/SetSunで即座に反映する。

#include "SkyboxPanel.h"
#include <imgui.h>

#include "Graphics/3D/Skybox.h"

void SkyboxPanel::Draw(GX::Skybox& skybox)
{
    if (!ImGui::Begin("Skybox"))
    {
        ImGui::End();
        return;
    }
    DrawContent(skybox);
    ImGui::End();
}

void SkyboxPanel::DrawContent(GX::Skybox& skybox)
{
    // --- Cubemap Faces ---
    if (ImGui::CollapsingHeader("Cubemap Faces"))
    {
        const char* faceNames[] = {
            "+X (Right)", "-X (Left)", "+Y (Top)", "-Y (Bottom)", "+Z (Front)", "-Z (Back)"
        };
        for (int i = 0; i < 6; ++i)
        {
            ImGui::PushID(i);
            ImGui::Text("%s:", faceNames[i]);
            ImGui::SameLine();
            ImGui::Text("%s", m_cubemapFaces[i].empty() ? "(none)" : m_cubemapFaces[i].c_str());
            if (ImGui::Button("Browse..."))
            {
                // Placeholder: would open file dialog
                // m_cubemapFaces[i] = ...;
            }
            ImGui::PopID();
        }
    }

    // --- HDR Environment Map ---
    if (ImGui::CollapsingHeader("HDR Environment Map"))
    {
        ImGui::Text("Path: %s", m_hdrEnvMapPath.empty() ? "(none)" : m_hdrEnvMapPath.c_str());
        if (ImGui::Button("Load HDR..."))
        {
            // Placeholder: would open file dialog
            // m_hdrEnvMapPath = ...;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##HDR"))
        {
            m_hdrEnvMapPath.clear();
        }
    }

    // --- Procedural Sky Parameters ---
    if (ImGui::CollapsingHeader("Procedural Sky", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool colorsChanged = false;

        if (ImGui::ColorEdit3("Top Color", m_topColor))
            colorsChanged = true;

        if (ImGui::ColorEdit3("Bottom Color", m_bottomColor))
            colorsChanged = true;

        if (colorsChanged)
        {
            skybox.SetColors(
                { m_topColor[0], m_topColor[1], m_topColor[2] },
                { m_bottomColor[0], m_bottomColor[1], m_bottomColor[2] }
            );
        }

        ImGui::Separator();
        ImGui::Text("Sun");

        bool sunChanged = false;
        if (ImGui::SliderFloat3("Direction", m_sunDirection, -1.0f, 1.0f, "%.2f"))
            sunChanged = true;

        if (ImGui::SliderFloat("Intensity", &m_sunIntensity, 0.0f, 20.0f, "%.1f"))
            sunChanged = true;

        if (sunChanged)
        {
            skybox.SetSun(
                { m_sunDirection[0], m_sunDirection[1], m_sunDirection[2] },
                m_sunIntensity
            );
        }

        ImGui::Separator();
        ImGui::SliderFloat("Rotation", &m_rotation, 0.0f, 360.0f, "%.1f deg");
        ImGui::TextWrapped("Note: Rotation is stored locally and not applied to the Skybox yet.");
    }
}
