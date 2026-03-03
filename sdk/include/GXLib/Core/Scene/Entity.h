#pragma once
/// @file Entity.h
/// @brief エンティティ（ゲームオブジェクト）

#include "pch_common.h"
#include "Core/Scene/Component.h"
#include "Math/Transform3D.h"
#include "Math/Collision/Collision3D.h"

namespace gx
{
/// @addtogroup grp_scene
/// @{

/// @brief エンティティのバウンディング情報（フラスタムカリング用）
struct BoundsInfo
{
    AABB3D localAABB;                ///< モデルのローカルAABB
    float  boundingSphereRadius = 0.0f; ///< バウンディング球半径
    bool   hasBounds = false;        ///< バウンディング情報が設定されているか
};

/// @brief エンティティ（ゲームオブジェクト）
///
/// Transform3Dを内蔵し、コンポーネントを追加して機能を拡張する。
/// 親子階層をサポートする。
class Entity
{
public:
    /// @brief コンストラクタ
    /// @param name エンティティ名（デフォルト: "Entity"）
    Entity(const gx::String& name = "Entity");
    ~Entity();

    /// @brief エンティティ名を取得する
    /// @return エンティティ名への参照
    const gx::String& GetName() const { return m_name; }

    /// @brief エンティティ名を設定する
    /// @param name 新しいエンティティ名
    void SetName(const gx::String& name) { m_name = name; }

    /// @brief 親エンティティを設定する（親子階層）
    /// @param parent 親エンティティへのポインタ（nullptrでルートに戻す）
    void SetParent(Entity* parent);

    /// @brief 親エンティティを取得する
    /// @return 親エンティティへのポインタ（ルートの場合nullptr）
    Entity* GetParent() const { return m_parent; }

    /// @brief 子エンティティ一覧を取得する
    /// @return 子エンティティポインタの配列
    const gx::Vector<Entity*>& GetChildren() const { return m_children; }

    /// @brief Transform3Dを取得する（可変）
    /// @return Transform3Dへの参照
    Transform3D& GetTransform() { return m_transform; }

    /// @brief Transform3Dを取得する（const）
    /// @return Transform3Dへのconst参照
    const Transform3D& GetTransform() const { return m_transform; }

    /// @brief ワールド行列を取得する（親の変換を考慮）
    /// @return ワールド変換行列
    Matrix4x4 GetWorldMatrix() const;

    /// @brief コンポーネントを追加する
    /// @return 追加されたコンポーネントへのポインタ
    template<typename T>
    T* AddComponent()
    {
        auto comp = std::make_unique<T>();
        T* ptr = comp.get();
        comp->m_entity = this;
        ComponentType type = comp->GetType();
        int idx = static_cast<int>(type);
        if (idx < static_cast<int>(ComponentType::_Count))
        {
            m_componentLookup[idx] = static_cast<int>(m_components.size());
        }
        m_components.push_back(std::move(comp));
        return ptr;
    }

    /// @brief 指定型のコンポーネントを取得する
    /// @return コンポーネントへのポインタ（未登録の場合nullptr）
    template<typename T>
    T* GetComponent() const
    {
        constexpr ComponentType type = T::k_Type;
        int idx = static_cast<int>(type);
        if (idx < static_cast<int>(ComponentType::_Count) && m_componentLookup[idx] >= 0)
        {
            return static_cast<T*>(m_components[m_componentLookup[idx]].get());
        }
        return nullptr;
    }

    /// @brief 指定型のコンポーネントが存在するか判定する
    /// @return コンポーネントが存在すればtrue
    template<typename T>
    bool HasComponent() const
    {
        return GetComponent<T>() != nullptr;
    }

    /// @brief 指定型のコンポーネントを削除する
    template<typename T>
    void RemoveComponent()
    {
        constexpr ComponentType type = T::k_Type;
        int idx = static_cast<int>(type);
        if (idx >= static_cast<int>(ComponentType::_Count)) return;
        int compIdx = m_componentLookup[idx];
        if (compIdx < 0) return;

        // ルックアップを更新
        m_componentLookup[idx] = -1;

        // 最後の要素と入れ替えて削除
        if (compIdx < static_cast<int>(m_components.size()) - 1)
        {
            ComponentType swapType = m_components.back()->GetType();
            int swapIdx = static_cast<int>(swapType);
            if (swapIdx < static_cast<int>(ComponentType::_Count))
            {
                m_componentLookup[swapIdx] = compIdx;
            }
            std::swap(m_components[compIdx], m_components.back());
        }
        m_components.pop_back();
    }

    /// @brief ComponentType指定でコンポーネントを取得する
    /// @param type コンポーネント種別
    /// @return コンポーネントへのポインタ（未登録の場合nullptr）
    Component* GetComponentByType(ComponentType type) const;

    /// @brief 全コンポーネント一覧を取得する
    /// @return コンポーネント配列への参照
    const gx::Vector<std::unique_ptr<Component>>& GetComponents() const { return m_components; }

    /// @brief アクティブ状態を取得する
    /// @return アクティブならtrue
    bool IsActive() const { return m_active; }

    /// @brief アクティブ状態を設定する
    /// @param active アクティブにするならtrue
    void SetActive(bool active) { m_active = active; }

    /// @brief エンティティIDを取得する
    /// @return エンティティID
    uint32_t GetID() const { return m_id; }

    /// @brief エンティティIDを設定する
    /// @param id 新しいID
    void SetID(uint32_t id) { m_id = id; }

    /// @brief バウンディング情報を設定する
    /// @param aabb ローカル空間のAABB
    void SetBounds(const AABB3D& aabb);

    /// @brief バウンディング情報を取得する
    /// @return BoundsInfoへのconst参照
    const BoundsInfo& GetBounds() const { return m_bounds; }

    /// @brief ワールド空間のバウンディング球を取得する（フラスタムカリング用）
    /// @return ワールド空間のバウンディング球
    Sphere GetWorldBoundingSphere() const;

private:
    uint32_t m_id = 0;
    gx::String m_name;
    bool m_active = true;
    Transform3D m_transform;
    Entity* m_parent = nullptr;
    gx::Vector<Entity*> m_children;
    gx::Vector<std::unique_ptr<Component>> m_components;
    int m_componentLookup[static_cast<int>(ComponentType::_Count)];
    BoundsInfo m_bounds;
};

/// @}
} // namespace gx
