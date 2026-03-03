/// @file InventorySystem.cpp
/// @brief アイテムインベントリ管理システム実装

#include "pch_common.h"
#include "Core/InventorySystem.h"

#include <algorithm>

namespace gx
{

// ============================================================================
// アイテム定義
// ============================================================================

void InventorySystem::RegisterItemDef(const ItemDef& def)
{
    m_itemDefs[def.itemId] = def;
}

const ItemDef* InventorySystem::GetItemDef(const gx::String& itemId) const
{
    auto it = m_itemDefs.find(itemId);
    if (it == m_itemDefs.end()) return nullptr;
    return &it->second;
}

size_t InventorySystem::GetItemDefCount() const
{
    return m_itemDefs.size();
}

// ============================================================================
// アイテム追加・削除
// ============================================================================

bool InventorySystem::AddItem(const gx::String& itemId, int quantity)
{
    if (quantity <= 0) return false;

    // アイテム定義が存在するか確認
    const ItemDef* def = GetItemDef(itemId);
    if (!def) return false;

    // 既存スロットを検索
    InventorySlot* slot = FindSlot(itemId);

    if (slot)
    {
        // スタック上限チェック
        int newQty = slot->quantity + quantity;
        if (newQty > def->maxStack)
        {
            return false; // スタック上限超過
        }

        int oldQty = slot->quantity;
        slot->quantity = newQty;
        NotifyChanged(itemId, oldQty, newQty, InventoryChangedEvent::Modified);
        return true;
    }

    // 新規スロット — スロット上限チェック
    if (m_slots.size() >= m_maxSlots)
    {
        return false;
    }

    // スタック上限チェック（新規スロットでも）
    if (quantity > def->maxStack)
    {
        return false;
    }

    InventorySlot newSlot;
    newSlot.itemId   = itemId;
    newSlot.quantity  = quantity;
    m_slots.push_back(std::move(newSlot));

    NotifyChanged(itemId, 0, quantity, InventoryChangedEvent::Added);
    return true;
}

bool InventorySystem::RemoveItem(const gx::String& itemId, int quantity)
{
    if (quantity <= 0) return false;

    InventorySlot* slot = FindSlot(itemId);
    if (!slot) return false;

    if (slot->quantity < quantity) return false;

    int oldQty = slot->quantity;
    int newQty = oldQty - quantity;

    if (newQty == 0)
    {
        // スロット削除
        auto it = std::find_if(m_slots.begin(), m_slots.end(),
            [&](const InventorySlot& s) { return s.itemId == itemId; });
        if (it != m_slots.end())
        {
            m_slots.erase(it);
        }
        NotifyChanged(itemId, oldQty, 0, InventoryChangedEvent::Removed);
    }
    else
    {
        slot->quantity = newQty;
        NotifyChanged(itemId, oldQty, newQty, InventoryChangedEvent::Modified);
    }

    return true;
}

// ============================================================================
// クエリ
// ============================================================================

bool InventorySystem::HasItem(const gx::String& itemId, int minQuantity) const
{
    const InventorySlot* slot = FindSlot(itemId);
    if (!slot) return false;
    return slot->quantity >= minQuantity;
}

int InventorySystem::GetItemCount(const gx::String& itemId) const
{
    const InventorySlot* slot = FindSlot(itemId);
    if (!slot) return 0;
    return slot->quantity;
}

const InventorySlot* InventorySystem::GetSlot(const gx::String& itemId) const
{
    return FindSlot(itemId);
}

// ============================================================================
// メタデータ
// ============================================================================

void InventorySystem::SetItemMetadata(const gx::String& itemId,
                                       const gx::String& key,
                                       const gx::String& value)
{
    InventorySlot* slot = FindSlot(itemId);
    if (!slot) return;

    slot->metadata[key] = value;
}

gx::String InventorySystem::GetItemMetadata(const gx::String& itemId,
                                              const gx::String& key) const
{
    const InventorySlot* slot = FindSlot(itemId);
    if (!slot) return {};

    auto it = slot->metadata.find(key);
    if (it == slot->metadata.end()) return {};
    return it->second;
}

// ============================================================================
// コレクション操作
// ============================================================================

gx::Vector<InventorySlot> InventorySystem::GetAllItems() const
{
    return m_slots;
}

gx::Vector<InventorySlot> InventorySystem::GetItemsByCategory(const gx::String& category) const
{
    gx::Vector<InventorySlot> result;
    for (const auto& slot : m_slots)
    {
        const ItemDef* def = GetItemDef(slot.itemId);
        if (def && def->category == category)
        {
            result.push_back(slot);
        }
    }
    return result;
}

size_t InventorySystem::GetSlotCount() const
{
    return m_slots.size();
}

size_t InventorySystem::GetMaxSlots() const
{
    return m_maxSlots;
}

void InventorySystem::SetMaxSlots(size_t maxSlots)
{
    m_maxSlots = maxSlots;
}

float InventorySystem::GetTotalWeight() const
{
    float total = 0.0f;
    for (const auto& slot : m_slots)
    {
        const ItemDef* def = GetItemDef(slot.itemId);
        if (def)
        {
            total += def->weight * static_cast<float>(slot.quantity);
        }
    }
    return total;
}

void InventorySystem::Clear()
{
    m_slots.clear();
}

// ============================================================================
// 内部ヘルパー
// ============================================================================

InventorySlot* InventorySystem::FindSlot(const gx::String& itemId)
{
    auto it = std::find_if(m_slots.begin(), m_slots.end(),
        [&](const InventorySlot& s) { return s.itemId == itemId; });
    if (it == m_slots.end()) return nullptr;
    return &(*it);
}

const InventorySlot* InventorySystem::FindSlot(const gx::String& itemId) const
{
    auto it = std::find_if(m_slots.begin(), m_slots.end(),
        [&](const InventorySlot& s) { return s.itemId == itemId; });
    if (it == m_slots.end()) return nullptr;
    return &(*it);
}

void InventorySystem::NotifyChanged(const gx::String& itemId, int oldQty, int newQty,
                                     InventoryChangedEvent::Action action)
{
    if (OnChanged)
    {
        InventoryChangedEvent evt;
        evt.itemId      = itemId;
        evt.oldQuantity = oldQty;
        evt.newQuantity = newQty;
        evt.action      = action;
        OnChanged(evt);
    }
}

} // namespace gx
