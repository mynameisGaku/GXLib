#pragma once
/// @file SceneHierarchyPanel.h
/// @brief シーン内のエンティティ親子ツリーを表示・選択するパネル
///
/// シーンを渡すとエンティティの階層構造をツリー表示する。
/// ノードの展開・折りたたみ・クリック選択に対応し、
/// Unityのヒエラルキーウィンドウに相当する機能を持つ。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include "Core/Scene/Scene.h"
#include <functional>
#include <set>

namespace gx
{

/// @brief エンティティの親子関係を表示するシーン階層ツリーパネル
class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    ~SceneHierarchyPanel() = default;

    /// @brief 表示するシーンを設定する
    void SetScene(Scene* scene);

    /// @brief 現在のシーンを取得する
    Scene* GetScene() const { return m_scene; }

    /// @brief エンティティ選択時のコールバックを設定する
    using SelectionCallback = std::function<void(Entity*)>;
    void SetOnEntitySelected(SelectionCallback cb) { m_onEntitySelected = std::move(cb); }

    /// @brief エンティティのクリックを処理する（選択する）
    void HandleClick(uint32_t entityId);

    /// @brief 現在選択中のエンティティを取得する
    Entity* GetSelectedEntity() const { return m_selectedEntity; }

    /// @brief 階層ツリーのノードを展開する
    void ExpandNode(uint32_t entityId);

    /// @brief 階層ツリーのノードを折りたたむ
    void CollapseNode(uint32_t entityId);

    /// @brief ノードの展開/折りたたみを切り替える
    void ToggleNode(uint32_t entityId);

    /// @brief ノードが展開されているかチェックする
    bool IsNodeExpanded(uint32_t entityId) const;

    /// @brief 全ノードを展開する
    void ExpandAll();

    /// @brief 全ノードを折りたたむ
    void CollapseAll();

    /// @brief 描画用の階層ノードデータ
    struct HierarchyNode
    {
        uint32_t entityId = 0;
        std::string name;
        int depth = 0;
        bool hasChildren = false;
        bool expanded = false;
        bool selected = false;
    };

    /// @brief 描画用のフラット化された階層を取得する
    std::vector<HierarchyNode> GetFlattenedHierarchy() const;

private:
    void FlattenEntity(Entity* entity, int depth, std::vector<HierarchyNode>& out) const;

    Scene* m_scene = nullptr;                      ///< 表示対象のシーン
    Entity* m_selectedEntity = nullptr;            ///< 現在選択中のエンティティ
    std::set<uint32_t> m_expandedNodes;            ///< 展開中のノードIDセット
    SelectionCallback m_onEntitySelected;          ///< エンティティ選択時コールバック
};

} // namespace gx
/// @}
