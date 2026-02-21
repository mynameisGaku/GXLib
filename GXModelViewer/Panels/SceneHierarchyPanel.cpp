/// @file SceneHierarchyPanel.cpp
/// @brief シーン階層パネル実装
///
/// エンティティをTreeNodeExで一覧表示し、クリックで選択、右クリックで追加/削除。
/// スキンドモデルはノード展開時にボーンツリーも表示する。

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

        bool hasSkeleton = entity->model && entity->model->HasSkeleton();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth
                                 | ImGuiTreeNodeFlags_OpenOnArrow;

        if (!hasSkeleton)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        if (scene.selectedEntity == i)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Use visibility as a visual indicator
        if (!entity->visible)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        bool nodeOpen = ImGui::TreeNodeEx(entity->name.c_str(), flags);

        if (!entity->visible)
            ImGui::PopStyleColor();

        // Click to select
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            scene.selectedEntity = i;
            scene.selectedBone = -1;
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

        // Draw bone tree inside expanded skinned entity
        if (hasSkeleton && nodeOpen)
        {
            const GX::Skeleton* skeleton = entity->model->GetSkeleton();
            const auto& joints = skeleton->GetJoints();
            for (size_t j = 0; j < joints.size(); ++j)
            {
                if (joints[j].parentIndex == -1)
                    DrawBoneTree(skeleton, scene, static_cast<int>(j));
            }
            ImGui::TreePop();
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

void SceneHierarchyPanel::DrawBoneTree(const GX::Skeleton* skeleton, SceneGraph& scene, int jointIndex)
{
    const auto& joints = skeleton->GetJoints();
    if (jointIndex < 0 || jointIndex >= static_cast<int>(joints.size()))
        return;

    // Find children
    bool hasChildren = false;
    for (size_t i = 0; i < joints.size(); ++i)
    {
        if (joints[i].parentIndex == jointIndex)
        {
            hasChildren = true;
            break;
        }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
    if (jointIndex == scene.selectedBone) flags |= ImGuiTreeNodeFlags_Selected;

    char label[128];
    snprintf(label, sizeof(label), "[%d] %s", jointIndex, joints[jointIndex].name.c_str());

    bool open = ImGui::TreeNodeEx(label, flags);

    if (ImGui::IsItemClicked())
        scene.selectedBone = jointIndex;

    if (open)
    {
        for (size_t i = 0; i < joints.size(); ++i)
        {
            if (joints[i].parentIndex == jointIndex)
                DrawBoneTree(skeleton, scene, static_cast<int>(i));
        }
        ImGui::TreePop();
    }
}
