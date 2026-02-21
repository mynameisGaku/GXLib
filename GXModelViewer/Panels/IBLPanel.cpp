/// @file IBLPanel.cpp
/// @brief IBL（環境照明）設定パネル実装
///
/// Skyboxのプロシージャルパラメータからキューブマップを生成してIBLを更新し、
/// 拡散照射マップ・鏡面プリフィルタマップ・BRDF LUTの状態を表示する。

#include "IBLPanel.h"
#include <imgui.h>
#include "Graphics/3D/IBL.h"
#include "Graphics/3D/Skybox.h"
#include "Graphics/3D/Renderer3D.h"

void IBLPanel::Draw(GX::IBL& ibl, GX::Skybox& skybox, GX::Renderer3D& renderer)
{
    if (!ImGui::Begin("IBL"))
    {
        ImGui::End();
        return;
    }
    DrawContent(ibl, skybox, renderer);
    ImGui::End();
}

void IBLPanel::DrawContent(GX::IBL& ibl, GX::Skybox& skybox, GX::Renderer3D& renderer)
{
    bool ready = ibl.IsReady();
    ImGui::Text("IBL Status: %s", ready ? "Ready" : "Not Initialized");

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Intensity", ImGuiTreeNodeFlags_DefaultOpen))
    {
        m_intensity = ibl.GetIntensity();
        if (ImGui::SliderFloat("IBL Intensity", &m_intensity, 0.0f, 5.0f))
        {
            ibl.SetIntensity(m_intensity);
        }
    }

    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Generate from Skybox"))
        {
            const auto& topCol = skybox.GetTopColor();
            const auto& botCol = skybox.GetBottomColor();
            const auto& sunDir = skybox.GetSunDirection();
            float sunInt = skybox.GetSunIntensity();
            ibl.UpdateFromSkybox(topCol, botCol, sunDir, sunInt);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Regenerate IBL maps from current skybox settings");
        }
    }

    if (ready && ImGui::CollapsingHeader("Debug Info"))
    {
        ImGui::Text("Irradiance SRV: 0x%llX",
                     static_cast<unsigned long long>(ibl.GetIrradianceSRV().ptr));
        ImGui::Text("Prefiltered SRV: 0x%llX",
                     static_cast<unsigned long long>(ibl.GetPrefilteredSRV().ptr));
        ImGui::Text("BRDF LUT SRV: 0x%llX",
                     static_cast<unsigned long long>(ibl.GetBRDFLUTSRV().ptr));
    }
}
