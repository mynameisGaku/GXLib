/// @file LightingPanel.cpp
/// @brief Lighting panel implementation

#include "LightingPanel.h"
#include <imgui.h>
#include <cstdio>
#include <algorithm>

#ifndef XM_PI
#define XM_PI 3.14159265358979323846f
#endif

void LightingPanel::Initialize()
{
    // Set up default lights matching GXModelViewerApp's initial setup
    m_lights.clear();

    GX::LightData dirLight = GX::Light::CreateDirectional(
        { 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
    m_lights.push_back(dirLight);

    GX::LightData pointLight = GX::Light::CreatePoint(
        { -3.0f, 4.0f, -3.0f }, 20.0f, { 1.0f, 0.95f, 0.9f }, 2.0f);
    m_lights.push_back(pointLight);

    m_ambientColor[0] = 0.15f;
    m_ambientColor[1] = 0.15f;
    m_ambientColor[2] = 0.18f;
    m_dirty = true;
}

void LightingPanel::Draw(GX::Renderer3D& renderer)
{
    if (!ImGui::Begin("Lighting"))
    {
        ImGui::End();
        return;
    }

    // Ambient color
    if (ImGui::ColorEdit3("Ambient", m_ambientColor))
    {
        m_dirty = true;
    }

    ImGui::Separator();

    // Add Light button
    bool canAdd = static_cast<int>(m_lights.size()) < k_MaxLights;
    if (!canAdd) ImGui::BeginDisabled();
    if (ImGui::Button("Add Light"))
    {
        GX::LightData newLight = GX::Light::CreateDirectional(
            { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f);
        m_lights.push_back(newLight);
        m_dirty = true;
    }
    if (!canAdd) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Text("%d / %d lights", static_cast<int>(m_lights.size()), k_MaxLights);

    ImGui::Separator();

    // List of lights
    int deleteIndex = -1;
    for (int i = 0; i < static_cast<int>(m_lights.size()); ++i)
    {
        ImGui::PushID(i);

        GX::LightData& light = m_lights[i];

        char header[64];
        const char* typeNames[] = { "Directional", "Point", "Spot" };
        uint32_t typeIdx = light.type;
        if (typeIdx > 2) typeIdx = 0;
        snprintf(header, sizeof(header), "Light %d (%s)", i, typeNames[typeIdx]);

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Type combo
            int currentType = static_cast<int>(light.type);
            if (currentType > 2) currentType = 0;
            if (ImGui::Combo("Type", &currentType, typeNames, 3))
            {
                light.type = static_cast<uint32_t>(currentType);
                m_dirty = true;
            }

            // Direction (Directional and Spot)
            if (light.type == static_cast<uint32_t>(GX::LightType::Directional) ||
                light.type == static_cast<uint32_t>(GX::LightType::Spot))
            {
                if (ImGui::DragFloat3("Direction", &light.direction.x, 0.01f, -1.0f, 1.0f))
                {
                    // Normalize direction
                    float len = sqrtf(light.direction.x * light.direction.x +
                                      light.direction.y * light.direction.y +
                                      light.direction.z * light.direction.z);
                    if (len > 0.0001f)
                    {
                        light.direction.x /= len;
                        light.direction.y /= len;
                        light.direction.z /= len;
                    }
                    m_dirty = true;
                }
            }

            // Position (Point and Spot)
            if (light.type == static_cast<uint32_t>(GX::LightType::Point) ||
                light.type == static_cast<uint32_t>(GX::LightType::Spot))
            {
                if (ImGui::DragFloat3("Position", &light.position.x, 0.1f))
                {
                    m_dirty = true;
                }
            }

            // Color
            if (ImGui::ColorEdit3("Color", &light.color.x))
            {
                m_dirty = true;
            }

            // Intensity
            if (ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 20.0f))
            {
                m_dirty = true;
            }

            // Range (Point and Spot)
            if (light.type == static_cast<uint32_t>(GX::LightType::Point) ||
                light.type == static_cast<uint32_t>(GX::LightType::Spot))
            {
                if (ImGui::SliderFloat("Range", &light.range, 0.1f, 100.0f))
                {
                    m_dirty = true;
                }
            }

            // Spot angle (Spot only)
            if (light.type == static_cast<uint32_t>(GX::LightType::Spot))
            {
                // spotAngle is stored as cos(theta); convert to degrees for editing
                float angleDeg = acosf((std::max)(-1.0f, (std::min)(1.0f, light.spotAngle))) * (180.0f / XM_PI);
                if (ImGui::SliderFloat("Spot Angle", &angleDeg, 1.0f, 90.0f, "%.1f deg"))
                {
                    light.spotAngle = cosf(angleDeg * (XM_PI / 180.0f));
                    m_dirty = true;
                }
            }

            // Delete button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Delete"))
            {
                deleteIndex = i;
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::PopID();
    }

    // Process delete (outside the loop to avoid iterator invalidation)
    if (deleteIndex >= 0 && deleteIndex < static_cast<int>(m_lights.size()))
    {
        m_lights.erase(m_lights.begin() + deleteIndex);
        m_dirty = true;
    }

    ImGui::Separator();

    // Apply button
    if (ImGui::Button("Apply Lights") || m_dirty)
    {
        XMFLOAT3 ambient = { m_ambientColor[0], m_ambientColor[1], m_ambientColor[2] };
        if (!m_lights.empty())
        {
            renderer.SetLights(m_lights.data(), static_cast<uint32_t>(m_lights.size()), ambient);
        }
        else
        {
            renderer.SetLights(nullptr, 0, ambient);
        }
        m_dirty = false;
    }

    ImGui::End();
}
