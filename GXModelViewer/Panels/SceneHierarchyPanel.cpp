/// @file SceneHierarchyPanel.cpp
/// @brief Scene hierarchy panel implementation

#include "SceneHierarchyPanel.h"
#include <imgui.h>

void SceneHierarchyPanel::Draw(SceneGraph& scene)
{
    if (!ImGui::Begin("Hierarchy"))
    {
        ImGui::End();
        return;
    }

    // "+" button to add a new empty entity
    if (ImGui::Button("+"))
    {
        int count = scene.GetEntityCount();
        // Count active entities for naming
        int activeCount = 0;
        for (int i = 0; i < count; ++i)
        {
            if (scene.GetEntity(i))
                ++activeCount;
        }
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "Entity_%d", activeCount);
        int newIdx = scene.AddEntity(nameBuf);
        scene.selectedEntity = newIdx;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("Scene Entities");

    ImGui::Separator();

    // List all entities
    int entityCount = scene.GetEntityCount();
    for (int i = 0; i < entityCount; ++i)
    {
        const SceneEntity* entity = scene.GetEntity(i);
        if (!entity)
            continue; // skip removed slots

        ImGui::PushID(i);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
                                 | ImGuiTreeNodeFlags_SpanAvailWidth
                                 | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        if (scene.selectedEntity == i)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Use visibility as a visual indicator
        if (!entity->visible)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        bool nodeOpen = ImGui::TreeNodeEx(entity->name.c_str(), flags);
        (void)nodeOpen; // leaf node, always closed

        if (!entity->visible)
            ImGui::PopStyleColor();

        // Click to select
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            scene.selectedEntity = i;
        }

        // Right-click context menu
        if (ImGui::BeginPopupContextItem("EntityContext"))
        {
            if (ImGui::MenuItem("Add Empty"))
            {
                char nameBuf[64];
                snprintf(nameBuf, sizeof(nameBuf), "Entity_%d", entityCount);
                int newIdx = scene.AddEntity(nameBuf);
                scene.selectedEntity = newIdx;
            }
            if (ImGui::MenuItem("Delete"))
            {
                scene.RemoveEntity(i);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // Right-click on empty space
    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Add Empty"))
        {
            char nameBuf[64];
            snprintf(nameBuf, sizeof(nameBuf), "Entity_%d", entityCount);
            int newIdx = scene.AddEntity(nameBuf);
            scene.selectedEntity = newIdx;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
