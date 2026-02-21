#pragma once
/// @file Entity.h
/// @brief エンティティ（ゲームオブジェクト）

#include "pch.h"
#include "Core/Scene/Component.h"
#include "Graphics/3D/Transform3D.h"
#include "Math/Collision/Collision3D.h"

namespace GX
{

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
    Entity(const std::string& name = "Entity");
    ~Entity();

    // --- 名前 ---
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    // --- 階層 ---
    void SetParent(Entity* parent);
    Entity* GetParent() const { return m_parent; }
    const std::vector<Entity*>& GetChildren() const { return m_children; }

    // --- Transform (常に存在) ---
    Transform3D& GetTransform() { return m_transform; }
    const Transform3D& GetTransform() const { return m_transform; }

    /// @brief ワールド行列（親の変換を考慮）
    XMMATRIX GetWorldMatrix() const;

    // --- コンポーネント ---
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

    template<typename T>
    bool HasComponent() const
    {
        return GetComponent<T>() != nullptr;
    }

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

    Component* GetComponentByType(ComponentType type) const;
    const std::vector<std::unique_ptr<Component>>& GetComponents() const { return m_components; }

    // --- アクティブ状態 ---
    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

    // --- ID ---
    uint32_t GetID() const { return m_id; }
    void SetID(uint32_t id) { m_id = id; }

    // --- バウンディング ---
    void SetBounds(const AABB3D& aabb);
    const BoundsInfo& GetBounds() const { return m_bounds; }

    /// @brief ワールド空間のバウンディング球を取得（フラスタムカリング用）
    Sphere GetWorldBoundingSphere() const;

private:
    uint32_t m_id = 0;
    std::string m_name;
    bool m_active = true;
    Transform3D m_transform;
    Entity* m_parent = nullptr;
    std::vector<Entity*> m_children;
    std::vector<std::unique_ptr<Component>> m_components;
    int m_componentLookup[static_cast<int>(ComponentType::_Count)];
    BoundsInfo m_bounds;
};

} // namespace GX
