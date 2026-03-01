/// @file SceneHierarchyPanel.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"

#include "Editor/SceneHierarchyPanel.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// SetScene
// ============================================================================

void SceneHierarchyPanel::SetScene(Scene* scene)
{
    m_scene = scene;
    m_selectedEntity = nullptr;
}

// ============================================================================
// HandleClick
// ============================================================================

void SceneHierarchyPanel::HandleClick(uint32_t entityId)
{
    if (!m_scene)
        return;

    Entity* found = m_scene->FindEntityByID(entityId);
    m_selectedEntity = found;

    if (found && m_onEntitySelected)
    {
        m_onEntitySelected(found);
    }
}

// ============================================================================
// 展開 / 折りたたみ / 切り替え
// ============================================================================

void SceneHierarchyPanel::ExpandNode(uint32_t entityId)
{
    m_expandedNodes.insert(entityId);
}

void SceneHierarchyPanel::CollapseNode(uint32_t entityId)
{
    m_expandedNodes.erase(entityId);
}

void SceneHierarchyPanel::ToggleNode(uint32_t entityId)
{
    if (m_expandedNodes.count(entityId) > 0)
    {
        m_expandedNodes.erase(entityId);
    }
    else
    {
        m_expandedNodes.insert(entityId);
    }
}

bool SceneHierarchyPanel::IsNodeExpanded(uint32_t entityId) const
{
    return m_expandedNodes.count(entityId) > 0;
}

// ============================================================================
// ExpandAll / CollapseAll
// ============================================================================

void SceneHierarchyPanel::ExpandAll()
{
    if (!m_scene)
        return;

    const auto& entities = m_scene->GetEntities();
    for (const auto& entity : entities)
    {
        m_expandedNodes.insert(entity->GetID());
    }
}

void SceneHierarchyPanel::CollapseAll()
{
    m_expandedNodes.clear();
}

// ============================================================================
// GetFlattenedHierarchy
// ============================================================================

std::vector<SceneHierarchyPanel::HierarchyNode> SceneHierarchyPanel::GetFlattenedHierarchy() const
{
    std::vector<HierarchyNode> result;

    if (!m_scene)
        return result;

    const auto& roots = m_scene->GetRootEntities();
    for (Entity* root : roots)
    {
        // 実際のルートエンティティのみ処理する（親なし）
        // Scene::GetRootEntities() は後から親を設定されたが
        // ルートリストから除去されていないエンティティを含む可能性がある
        if (root && root->GetParent() == nullptr)
        {
            FlattenEntity(root, 0, result);
        }
    }

    return result;
}

// ============================================================================
// FlattenEntity（再帰）
// ============================================================================

void SceneHierarchyPanel::FlattenEntity(Entity* entity, int depth,
                                          std::vector<HierarchyNode>& out) const
{
    if (!entity)
        return;

    HierarchyNode node;
    node.entityId    = entity->GetID();
    node.name        = entity->GetName();
    node.depth       = depth;
    node.hasChildren = !entity->GetChildren().empty();
    node.expanded    = IsNodeExpanded(entity->GetID());
    node.selected    = (m_selectedEntity == entity);

    out.push_back(std::move(node));

    // 展開済みかつ子がある場合、depth+1で子に再帰
    if (node.expanded && node.hasChildren)
    {
        const auto& children = entity->GetChildren();
        for (Entity* child : children)
        {
            FlattenEntity(child, depth + 1, out);
        }
    }
}

} // namespace gx
