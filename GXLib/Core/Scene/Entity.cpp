#include "pch.h"
/// @file Entity.cpp
/// @brief エンティティ実装

#include "Core/Scene/Entity.h"

namespace GX
{

Entity::Entity(const std::string& name)
    : m_name(name)
{
    for (int i = 0; i < static_cast<int>(ComponentType::_Count); ++i)
    {
        m_componentLookup[i] = -1;
    }
}

Entity::~Entity()
{
    // 親子関係をクリーンアップ
    if (m_parent)
    {
        auto& siblings = m_parent->m_children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }
    // 子エンティティの親参照をクリア
    for (auto* child : m_children)
    {
        child->m_parent = nullptr;
    }
}

void Entity::SetParent(Entity* parent)
{
    // 現在の親から自分を除去
    if (m_parent)
    {
        auto& siblings = m_parent->m_children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    m_parent = parent;

    // 新しい親の子リストに追加
    if (m_parent)
    {
        m_parent->m_children.push_back(this);
    }
}

XMMATRIX Entity::GetWorldMatrix() const
{
    XMMATRIX local = m_transform.GetWorldMatrix();
    if (m_parent)
    {
        return local * m_parent->GetWorldMatrix();
    }
    return local;
}

Component* Entity::GetComponentByType(ComponentType type) const
{
    int idx = static_cast<int>(type);
    if (idx < static_cast<int>(ComponentType::_Count) && m_componentLookup[idx] >= 0)
    {
        return m_components[m_componentLookup[idx]].get();
    }
    return nullptr;
}

void Entity::SetBounds(const AABB3D& aabb)
{
    m_bounds.localAABB = aabb;
    Vector3 center = aabb.Center();
    Vector3 halfExt = aabb.HalfExtents();
    m_bounds.boundingSphereRadius = std::sqrt(
        halfExt.x * halfExt.x + halfExt.y * halfExt.y + halfExt.z * halfExt.z);
    m_bounds.hasBounds = true;
}

Sphere Entity::GetWorldBoundingSphere() const
{
    Vector3 localCenter = m_bounds.localAABB.Center();
    XMMATRIX world = GetWorldMatrix();
    XMVECTOR cen = XMVector3Transform(
        XMVectorSet(localCenter.x, localCenter.y, localCenter.z, 1.0f), world);

    // スケールの最大成分でスフィア半径をスケーリング
    XMVECTOR scaleX = XMVector3Length(world.r[0]);
    XMVECTOR scaleY = XMVector3Length(world.r[1]);
    XMVECTOR scaleZ = XMVector3Length(world.r[2]);
    float maxScale = (std::max)({XMVectorGetX(scaleX), XMVectorGetX(scaleY), XMVectorGetX(scaleZ)});

    XMFLOAT3 wc;
    XMStoreFloat3(&wc, cen);
    return Sphere(Vector3(wc.x, wc.y, wc.z), m_bounds.boundingSphereRadius * maxScale);
}

} // namespace GX
