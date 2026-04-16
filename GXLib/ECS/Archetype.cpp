/// @file Archetype.cpp
/// @brief Archetype の実装
#include "pch_common.h"
#include "ECS/Archetype.h"

namespace gx { namespace ecs {

// ---------------------------------------------------------------------------
// 構築
// ---------------------------------------------------------------------------
Archetype::Archetype(ComponentMask mask,
                     const gx::Vector<std::pair<ComponentID, uint32_t>>& componentInfos)
    : m_mask(mask)
{
    m_storages.reserve(componentInfos.size());
    for (const auto& [cid, size] : componentInfos)
    {
        m_componentIndex[cid] = static_cast<uint32_t>(m_storages.size());
        m_storages.emplace_back(cid, size);
    }
}

// ---------------------------------------------------------------------------
// コンポーネント検索
// ---------------------------------------------------------------------------
bool Archetype::HasComponent(ComponentID id) const
{
    return m_componentIndex.find(id) != m_componentIndex.end();
}

// ---------------------------------------------------------------------------
// エンティティ追加
// ---------------------------------------------------------------------------
uint32_t Archetype::AddEntity(EntityID entity)
{
    uint32_t row = static_cast<uint32_t>(m_entities.size());
    m_entities.push_back(entity);

    // すべてのストレージ列にデフォルトゼロ要素を追加
    for (auto& storage : m_storages)
    {
        storage.AddElement();
    }

    return row;
}

// ---------------------------------------------------------------------------
// エンティティ削除（スワップアンドポップ）
// ---------------------------------------------------------------------------
void Archetype::RemoveEntity(uint32_t row, EntityID& swappedEntity)
{
    assert(row < m_entities.size());

    uint32_t lastRow = static_cast<uint32_t>(m_entities.size()) - 1;

    if (row != lastRow)
    {
        // 最後の行にいたエンティティが'row'にスワップされる
        swappedEntity = m_entities[lastRow];
        m_entities[row] = swappedEntity;
    }
    else
    {
        swappedEntity = k_InvalidEntity;
    }

    m_entities.pop_back();

    // すべてのストレージ列でスワップアンドポップ
    for (auto& storage : m_storages)
    {
        storage.SwapAndPop(row);
    }
}

// ---------------------------------------------------------------------------
// コンポーネントデータアクセス
// ---------------------------------------------------------------------------
void* Archetype::GetComponentData(ComponentID id, uint32_t row)
{
    auto it = m_componentIndex.find(id);
    if (it == m_componentIndex.end()) return nullptr;
    return m_storages[it->second].GetElement(row);
}

const void* Archetype::GetComponentData(ComponentID id, uint32_t row) const
{
    auto it = m_componentIndex.find(id);
    if (it == m_componentIndex.end()) return nullptr;
    return m_storages[it->second].GetElement(row);
}

ComponentStorage* Archetype::GetStorage(ComponentID id)
{
    auto it = m_componentIndex.find(id);
    if (it == m_componentIndex.end()) return nullptr;
    return &m_storages[it->second];
}

}} // namespace gx::ecs
