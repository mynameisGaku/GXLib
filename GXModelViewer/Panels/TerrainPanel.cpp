/// @file TerrainPanel.cpp
/// @brief テレイン編集パネル実装（プレースホルダー）

#include "TerrainPanel.h"
#include <imgui.h>

void TerrainPanel::Draw()
{
    if (!ImGui::Begin("Terrain"))
    {
        ImGui::End();
        return;
    }

    // --- Heightmap ---
    if (ImGui::CollapsingHeader("Heightmap", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Path: %s", m_heightmapPath.empty() ? "(none)" : m_heightmapPath.c_str());
        if (ImGui::Button("Load Heightmap..."))
        {
            // Placeholder: would open file dialog
            // m_heightmapPath = ...;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##Heightmap"))
        {
            m_heightmapPath.clear();
        }
    }

    // --- Height Scale ---
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Height Scale", &m_heightScale, 0.1f, 100.0f, "%.1f");
    }

    // --- Texture Layers ---
    if (ImGui::CollapsingHeader("Texture Layers (Splat Map)"))
    {
        const char* layerNames[] = { "Layer 0 (Base)", "Layer 1", "Layer 2", "Layer 3" };
        for (int i = 0; i < 4; ++i)
        {
            ImGui::PushID(i);
            ImGui::Text("%s:", layerNames[i]);
            ImGui::SameLine();
            ImGui::Text("%s", m_splatTextures[i].empty() ? "(none)" : m_splatTextures[i].c_str());
            if (ImGui::Button("Browse..."))
            {
                // Placeholder: would open file dialog
                // m_splatTextures[i] = ...;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear"))
            {
                m_splatTextures[i].clear();
            }
            ImGui::PopID();
        }
    }

    // --- LOD ---
    if (ImGui::CollapsingHeader("LOD"))
    {
        ImGui::SliderInt("LOD Level", &m_lodLevel, 0, 6);
        ImGui::TextWrapped("Higher LOD = more detail, lower = faster rendering.");
    }

    ImGui::End();
}
