/// @file SceneHierarchyPanel.cpp
/// @brief シーン階層パネルの ImGui 実装
#include "pch_graphics.h"
#include "Editor/SceneHierarchyPanel.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"

#include <imgui.h>

namespace gx
{

// ============================================================================
// メイン描画
// ============================================================================

void SceneHierarchyPanel::Draw(Scene* scene)
{
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Scene Hierarchy", &m_visible))
    {
        ImGui::End();
        return;
    }

    if (!scene)
    {
        ImGui::TextDisabled("No scene loaded");
        ImGui::End();
        return;
    }

    // シーン名表示
    ImGui::Text("Scene: %s", scene->GetName().c_str());
    ImGui::Separator();

    // 新規エンティティ作成ボタン
    if (ImGui::Button("+ Create Entity"))
    {
        Entity* newEntity = scene->CreateEntity("New Entity");
        m_selected = newEntity;
    }

    ImGui::Separator();

    // ルートエンティティを再帰描画
    const auto& roots = scene->GetRootEntities();
    for (auto* entity : roots)
    {
        DrawEntityNode(scene, entity);
    }

    // 空白領域クリックで選択解除
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        && !ImGui::IsAnyItemHovered())
    {
        m_selected = nullptr;
    }

    ImGui::End();
}

// ============================================================================
// エンティティノード描画（再帰）
// ============================================================================

void SceneHierarchyPanel::DrawEntityNode(Scene* scene, Entity* entity)
{
    if (!entity) return;

    const auto& children = entity->GetChildren();
    bool hasChildren = !children.empty();

    // ツリーノードフラグ設定
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (m_selected == entity)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // 非アクティブエンティティはテキスト色を暗くする
    if (!entity->IsActive())
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

    // ラベル: エンティティ名 + ユニークID
    char label[256];
    snprintf(label, sizeof(label), "%s##%u", entity->GetName().c_str(), entity->GetID());

    bool nodeOpen = ImGui::TreeNodeEx(label, flags);

    if (!entity->IsActive())
        ImGui::PopStyleColor();

    // クリックで選択
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        m_selected = entity;
    }

    // ドラッグソース（親子付け替え用）
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        Entity* dragEntity = entity;
        ImGui::SetDragDropPayload("ENTITY_NODE", &dragEntity, sizeof(Entity*));
        ImGui::Text("Move: %s", entity->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // ドロップターゲット（子に追加）
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE"))
        {
            Entity* droppedEntity = *static_cast<Entity**>(payload->Data);
            if (droppedEntity && droppedEntity != entity)
            {
                // 循環参照チェック: entity が droppedEntity の子孫でないことを確認
                bool isDescendant = false;
                Entity* check = entity->GetParent();
                while (check)
                {
                    if (check == droppedEntity)
                    {
                        isDescendant = true;
                        break;
                    }
                    check = check->GetParent();
                }

                if (!isDescendant)
                {
                    droppedEntity->SetParent(entity);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // 右クリックコンテキストメニュー
    if (ImGui::BeginPopupContextItem())
    {
        m_selected = entity;

        if (ImGui::MenuItem("Create Child"))
        {
            Entity* child = scene->CreateEntity("Child Entity");
            child->SetParent(entity);
        }

        if (ImGui::MenuItem("Duplicate"))
        {
            Entity* dup = scene->CreateEntity(entity->GetName());
            dup->GetTransform().SetPosition(entity->GetTransform().GetPosition());
            dup->GetTransform().SetRotation(entity->GetTransform().GetRotation());
            dup->GetTransform().SetScale(entity->GetTransform().GetScale());
            if (entity->GetParent())
                dup->SetParent(entity->GetParent());
        }

        ImGui::Separator();

        bool active = entity->IsActive();
        if (ImGui::MenuItem(active ? "Deactivate" : "Activate"))
        {
            entity->SetActive(!active);
        }

        if (ImGui::MenuItem("Unparent") && entity->GetParent())
        {
            entity->SetParent(nullptr);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            if (m_selected == entity)
                m_selected = nullptr;
            scene->DestroyEntity(entity);
            ImGui::EndPopup();
            if (hasChildren && nodeOpen)
                ImGui::TreePop();
            return;
        }

        ImGui::EndPopup();
    }

    // 子ノードを再帰描画
    if (hasChildren && nodeOpen)
    {
        for (auto* child : children)
        {
            DrawEntityNode(scene, child);
        }
        ImGui::TreePop();
    }
}

} // namespace gx
