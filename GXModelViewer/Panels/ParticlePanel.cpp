/// @file ParticlePanel.cpp
/// @brief 3Dパーティクルシステム設定パネル実装
///
/// ParticleSystem3Dに登録された全エミッターの一覧表示・選択・パラメータ編集を行う。
/// ParticleEmitterConfigのフィールドを直接編集し、リアルタイムにエフェクトを調整できる。

#include "ParticlePanel.h"
#include <imgui.h>
#include "Graphics/3D/ParticleSystem3D.h"
#include "Graphics/3D/ParticleEmitter.h"

void ParticlePanel::Draw(GX::ParticleSystem3D& system)
{
    if (!ImGui::Begin("Particles"))
    {
        ImGui::End();
        return;
    }
    DrawContent(system);
    ImGui::End();
}

void ParticlePanel::DrawContent(GX::ParticleSystem3D& system)
{
    ImGui::Text("Total Particles: %u", system.GetTotalParticleCount());
    ImGui::Separator();

    int emitterCount = system.GetEmitterCount();
    ImGui::Text("Emitters: %d", emitterCount);

    if (ImGui::Button("Add Emitter"))
    {
        GX::ParticleEmitterConfig config;
        system.AddEmitter(config);
    }

    ImGui::Separator();

    // Emitter list
    for (int i = 0; i < emitterCount; ++i)
    {
        ImGui::PushID(i);

        bool isSelected = (m_selectedEmitter == i);
        char label[64];
        snprintf(label, sizeof(label), "Emitter %d  (%u particles)",
                 i, system.GetEmitter(i).GetParticleCount());

        if (ImGui::Selectable(label, isSelected))
            m_selectedEmitter = i;

        ImGui::PopID();
    }

    // Selected emitter details
    if (m_selectedEmitter >= 0 && m_selectedEmitter < emitterCount)
    {
        GX::ParticleEmitter& emitter = system.GetEmitter(m_selectedEmitter);
        GX::ParticleEmitterConfig& config = emitter.GetConfigMutable();

        ImGui::Separator();
        ImGui::Text("Emitter %d Settings", m_selectedEmitter);

        // Active toggle
        bool active = emitter.IsActive();
        if (ImGui::Checkbox("Active", &active))
            emitter.SetActive(active);

        // Emission
        if (ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Emission Rate", &config.emissionRate, 1.0f, 0.0f, 1000.0f);
            int maxP = static_cast<int>(config.maxParticles);
            if (ImGui::DragInt("Max Particles", &maxP, 10, 1, 10000))
                config.maxParticles = static_cast<uint32_t>(maxP);
        }

        // Lifetime & Speed
        if (ImGui::CollapsingHeader("Lifetime & Speed", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Life Min", &config.lifeMin, 0.01f, 0.0f, 30.0f);
            ImGui::DragFloat("Life Max", &config.lifeMax, 0.01f, 0.0f, 30.0f);
            ImGui::DragFloat("Speed Min", &config.speedMin, 0.1f, 0.0f, 500.0f);
            ImGui::DragFloat("Speed Max", &config.speedMax, 0.1f, 0.0f, 500.0f);
        }

        // Size
        if (ImGui::CollapsingHeader("Size"))
        {
            ImGui::DragFloat("Size Min", &config.sizeMin, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Size Max", &config.sizeMax, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Size Over Life", &config.sizeOverLife, 0.01f, -5.0f, 5.0f);
        }

        // Color
        if (ImGui::CollapsingHeader("Color"))
        {
            float startCol[4] = { config.colorStart.x, config.colorStart.y,
                                   config.colorStart.z, config.colorStart.w };
            if (ImGui::ColorEdit4("Start Color", startCol))
            {
                config.colorStart.x = startCol[0];
                config.colorStart.y = startCol[1];
                config.colorStart.z = startCol[2];
                config.colorStart.w = startCol[3];
            }

            float endCol[4] = { config.colorEnd.x, config.colorEnd.y,
                                 config.colorEnd.z, config.colorEnd.w };
            if (ImGui::ColorEdit4("End Color", endCol))
            {
                config.colorEnd.x = endCol[0];
                config.colorEnd.y = endCol[1];
                config.colorEnd.z = endCol[2];
                config.colorEnd.w = endCol[3];
            }
        }

        // Shape
        if (ImGui::CollapsingHeader("Shape"))
        {
            const char* shapeNames[] = { "Point", "Sphere", "Cone", "Box" };
            int shapeIdx = static_cast<int>(config.shape);
            if (ImGui::Combo("Shape", &shapeIdx, shapeNames, 4))
                config.shape = static_cast<GX::ParticleShape>(shapeIdx);

            if (config.shape == GX::ParticleShape::Sphere ||
                config.shape == GX::ParticleShape::Cone)
            {
                ImGui::DragFloat("Radius", &config.shapeRadius, 0.1f, 0.0f, 50.0f);
            }
            if (config.shape == GX::ParticleShape::Cone)
            {
                ImGui::DragFloat("Cone Angle", &config.coneAngle, 1.0f, 0.0f, 90.0f);
            }
            if (config.shape == GX::ParticleShape::Box)
            {
                ImGui::DragFloat3("Box Half Extents", &config.boxHalfExtents.x, 0.1f, 0.0f, 50.0f);
            }
        }

        // Physics
        if (ImGui::CollapsingHeader("Physics"))
        {
            ImGui::DragFloat3("Gravity", &config.gravity.x, 0.1f);
            ImGui::DragFloat("Drag", &config.drag, 0.01f, 0.0f, 1.0f);
        }

        // Blend mode
        if (ImGui::CollapsingHeader("Rendering"))
        {
            const char* blendNames[] = { "Alpha", "Additive" };
            int blendIdx = static_cast<int>(config.blend);
            if (ImGui::Combo("Blend", &blendIdx, blendNames, 2))
                config.blend = static_cast<GX::ParticleBlend>(blendIdx);
        }

        // Burst buttons
        ImGui::Separator();
        if (ImGui::Button("Burst 50"))
            emitter.Burst(50);
        ImGui::SameLine();
        if (ImGui::Button("Burst 200"))
            emitter.Burst(200);
    }
}
