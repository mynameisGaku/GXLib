#pragma once
/// @file PropertyInspector.h
/// @brief ImGui ベースのプロパティインスペクター -- エンティティのコンポーネントとプロパティを編集
///
/// 選択中エンティティの各コンポーネントを折りたたみヘッダーで表示し、
/// リフレクション (TypeInfo/PropertyMeta) を使ってプロパティをウィジェットで編集する。
/// @addtogroup grp_editor
/// @{

#include "pch_graphics.h"

namespace gx
{

class Entity;
class Scene;
class TypeInfo;
struct PropertyMeta;

/// @brief リフレクションベースのプロパティエディタパネル
class PropertyInspector
{
public:
    /// @brief インスペクターを描画する
    /// @param entity 表示するエンティティ（nullptr の場合は「No entity selected」表示）
    /// @param scene エンティティが所属するシーン（コンポーネント追加に使用）
    void Draw(Entity* entity, Scene* scene);

    /// @brief パネルの表示/非表示を切り替える
    void Toggle() { m_visible = !m_visible; }

    /// @brief パネルが表示中かどうかを返す
    bool IsVisible() const { return m_visible; }

    /// @brief パネルの表示/非表示を設定する
    void SetVisible(bool v) { m_visible = v; }

private:
    bool m_visible = true;

    /// @brief コンポーネントの全プロパティを描画する
    void DrawComponent(const TypeInfo* typeInfo, void* componentData);

    /// @brief 単一プロパティを適切なウィジェットで描画する
    void DrawProperty(const PropertyMeta& prop, void* base);
};

} // namespace gx
/// @}
