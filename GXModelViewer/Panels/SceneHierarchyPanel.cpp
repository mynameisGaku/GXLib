/// @file SceneHierarchyPanel.cpp
/// @brief シーン階層パネル実装
///
/// エンティティをTreeNodeExで一覧表示し、クリックで選択、右クリックで追加/削除。
/// ドラッグ&ドロップで親子関係を変更する。プリミティブ追加はPendingAction経由。

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
        m_pendingPrimitive = PrimitiveRequest::Empty;
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

        // Show parent indicator
        char label[128];
        if (entity->parentIndex >= 0)
        {
            const SceneEntity* parent = scene.GetEntity(entity->parentIndex);
            if (parent)
                snprintf(label, sizeof(label), "%s  (-> %s)", entity->name.c_str(), parent->name.c_str());
            else
                snprintf(label, sizeof(label), "%s", entity->name.c_str());
        }
        else
        {
            snprintf(label, sizeof(label), "%s", entity->name.c_str());
        }

        bool nodeOpen = ImGui::TreeNodeEx(label, flags);

        if (!entity->visible)
            ImGui::PopStyleColor();

        // Click to select
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            scene.selectedEntity = i;
            scene.selectedBone = -1;
        }

        // --- Drag source ---
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("ENTITY_INDEX", &i, sizeof(int));
            ImGui::Text("Move: %s", entity->name.c_str());
            ImGui::EndDragDropSource();
        }

        // --- Drop target ---
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_INDEX"))
            {
                int draggedIdx = *(const int*)payload->Data;
                if (draggedIdx != i && !scene.IsAncestor(draggedIdx, i))
                {
                    const SceneEntity* dragged = scene.GetEntity(draggedIdx);
                    if (dragged)
                    {
                        m_reparentEntity    = draggedIdx;
                        m_reparentOldParent = dragged->parentIndex;
                        m_reparentNewParent = i;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Right-click context menu
        if (ImGui::BeginPopupContextItem("EntityContext"))
        {
            DrawContextMenu(scene, entityCount);
            if (ImGui::MenuItem("Unparent") && entity->parentIndex >= 0)
            {
                // Use reparent mechanism to unparent
                m_reparentEntity    = i;
                m_reparentOldParent = entity->parentIndex;
                m_reparentNewParent = -1;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                scene.RemoveEntity(i);
            }
            ImGui::EndPopup();
        }

        // Draw bone tree inside expanded skinned entity
        if (hasSkeleton && nodeOpen)
        {
            const gx::Skeleton* skeleton = entity->model->GetSkeleton();
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

    // Drop on empty space = unparent (set to root)
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_INDEX"))
        {
            int draggedIdx = *(const int*)payload->Data;
            const SceneEntity* dragged = scene.GetEntity(draggedIdx);
            if (dragged && dragged->parentIndex >= 0)
            {
                m_reparentEntity    = draggedIdx;
                m_reparentOldParent = dragged->parentIndex;
                m_reparentNewParent = -1;
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Right-click on empty space
    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
    {
        DrawContextMenu(scene, entityCount);
        ImGui::EndPopup();
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawContextMenu(SceneGraph& /*scene*/, int /*entityCount*/)
{
    if (ImGui::MenuItem("Add Empty"))
        m_pendingPrimitive = PrimitiveRequest::Empty;

    if (ImGui::BeginMenu("Add Primitive"))
    {
        if (ImGui::MenuItem("Cube"))
            m_pendingPrimitive = PrimitiveRequest::Cube;
        if (ImGui::MenuItem("Sphere"))
            m_pendingPrimitive = PrimitiveRequest::Sphere;
        if (ImGui::MenuItem("Plane"))
            m_pendingPrimitive = PrimitiveRequest::Plane;
        if (ImGui::MenuItem("Cylinder"))
            m_pendingPrimitive = PrimitiveRequest::Cylinder;
        if (ImGui::MenuItem("Cone"))
            m_pendingPrimitive = PrimitiveRequest::Cone;
        ImGui::EndMenu();
    }
}

void SceneHierarchyPanel::DrawBoneTree(const gx::Skeleton* skeleton, SceneGraph& scene, int jointIndex)
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
