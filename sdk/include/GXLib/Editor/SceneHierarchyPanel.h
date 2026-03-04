#pragma once
/// @file SceneHierarchyPanel.h
/// @brief ImGui ベースのシーン階層パネル -- エンティティのツリー表示と選択
///
/// Scene のエンティティ階層をツリービューで表示し、クリック選択、
/// 右クリックコンテキストメニュー（削除・複製・子エンティティ作成）、
/// ドラッグ＆ドロップによる親子付け替えをサポートする。
/// @addtogroup grp_editor
/// @{

#include "pch_graphics.h"

namespace gx
{

class Scene;
class Entity;

/// @brief シーン階層ツリービューパネル
class SceneHierarchyPanel
{
public:
    /// @brief パネルを描画する
    /// @param scene 表示するシーン（nullptr の場合は「Not available」表示）
    void Draw(Scene* scene);

    /// @brief 選択中のエンティティを取得する
    /// @return 選択中のエンティティへのポインタ（未選択時は nullptr）
    Entity* GetSelectedEntity() const { return m_selected; }

    /// @brief 選択エンティティを設定する
    /// @param e 選択するエンティティ（nullptr で選択解除）
    void SetSelectedEntity(Entity* e) { m_selected = e; }

    /// @brief パネルの表示/非表示を切り替える
    void Toggle() { m_visible = !m_visible; }

    /// @brief パネルが表示中かどうかを返す
    bool IsVisible() const { return m_visible; }

    /// @brief パネルの表示/非表示を設定する
    void SetVisible(bool v) { m_visible = v; }

private:
    Entity* m_selected = nullptr;
    bool m_visible = true;

    /// @brief エンティティノードを再帰描画する
    void DrawEntityNode(Scene* scene, Entity* entity);
};

} // namespace gx
/// @}
