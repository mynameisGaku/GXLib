/// @file test_InventorySystem.cpp
/// @brief InventorySystem 単体テスト

#include <gtest/gtest.h>
#include "Core/InventorySystem.h"

using namespace gx;

// ヘルパー: テスト用アイテム定義を生成
static ItemDef MakeItem(const gx::String& id, const gx::String& category = "misc",
                        int maxStack = 99, float weight = 1.0f)
{
    ItemDef def;
    def.itemId      = id;
    def.displayName = id;
    def.description = "Test item: " + id;
    def.maxStack    = maxStack;
    def.weight      = weight;
    def.category    = category;
    return def;
}

TEST(InventorySystemTest, ConstructDestruct)
{
    InventorySystem inv;
    EXPECT_EQ(inv.GetSlotCount(), 0u);
    EXPECT_EQ(inv.GetMaxSlots(), 100u);
    EXPECT_EQ(inv.GetItemDefCount(), 0u);
}

TEST(InventorySystemTest, RegisterItemDef)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("sword"));
    inv.RegisterItemDef(MakeItem("shield"));
    EXPECT_EQ(inv.GetItemDefCount(), 2u);
}

TEST(InventorySystemTest, GetItemDef)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("potion", "consumable", 10, 0.5f));

    const ItemDef* def = inv.GetItemDef("potion");
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->itemId, "potion");
    EXPECT_EQ(def->category, "consumable");
    EXPECT_EQ(def->maxStack, 10);
    EXPECT_FLOAT_EQ(def->weight, 0.5f);

    EXPECT_EQ(inv.GetItemDef("nonexistent"), nullptr);
}

TEST(InventorySystemTest, AddItem)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("sword", "weapon", 1, 5.0f));

    EXPECT_TRUE(inv.AddItem("sword"));
    EXPECT_EQ(inv.GetSlotCount(), 1u);
    EXPECT_EQ(inv.GetItemCount("sword"), 1);
}

TEST(InventorySystemTest, AddItemStack)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("arrow", "ammo", 99, 0.1f));

    EXPECT_TRUE(inv.AddItem("arrow", 10));
    EXPECT_EQ(inv.GetItemCount("arrow"), 10);
    EXPECT_TRUE(inv.AddItem("arrow", 5));
    EXPECT_EQ(inv.GetItemCount("arrow"), 15);
}

TEST(InventorySystemTest, AddItemOverMax)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("potion", "consumable", 5, 0.5f));

    EXPECT_TRUE(inv.AddItem("potion", 3));
    // 3 + 5 = 8 > maxStack(5), should fail
    EXPECT_FALSE(inv.AddItem("potion", 5));
    EXPECT_EQ(inv.GetItemCount("potion"), 3); // unchanged

    // 未登録アイテムの追加は失敗
    EXPECT_FALSE(inv.AddItem("unknown_item"));
}

TEST(InventorySystemTest, RemoveItem)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("gem"));

    inv.AddItem("gem", 5);
    EXPECT_TRUE(inv.RemoveItem("gem", 5));
    EXPECT_EQ(inv.GetSlotCount(), 0u);
    EXPECT_EQ(inv.GetItemCount("gem"), 0);
}

TEST(InventorySystemTest, RemovePartial)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("coin"));

    inv.AddItem("coin", 10);
    EXPECT_TRUE(inv.RemoveItem("coin", 3));
    EXPECT_EQ(inv.GetItemCount("coin"), 7);
    EXPECT_EQ(inv.GetSlotCount(), 1u);
}

TEST(InventorySystemTest, RemoveNotExist)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("key"));

    EXPECT_FALSE(inv.RemoveItem("key", 1)); // not in inventory

    inv.AddItem("key", 2);
    EXPECT_FALSE(inv.RemoveItem("key", 5)); // not enough
    EXPECT_EQ(inv.GetItemCount("key"), 2);  // unchanged
}

TEST(InventorySystemTest, HasItem)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("torch"));

    EXPECT_FALSE(inv.HasItem("torch"));
    inv.AddItem("torch", 3);
    EXPECT_TRUE(inv.HasItem("torch"));
    EXPECT_TRUE(inv.HasItem("torch", 3));
    EXPECT_FALSE(inv.HasItem("torch", 4));
}

TEST(InventorySystemTest, GetItemCount)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("herb"));

    EXPECT_EQ(inv.GetItemCount("herb"), 0);
    inv.AddItem("herb", 7);
    EXPECT_EQ(inv.GetItemCount("herb"), 7);
    EXPECT_EQ(inv.GetItemCount("nonexistent"), 0);
}

TEST(InventorySystemTest, GetSlot)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("ring"));

    EXPECT_EQ(inv.GetSlot("ring"), nullptr);
    inv.AddItem("ring", 2);

    const InventorySlot* slot = inv.GetSlot("ring");
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->itemId, "ring");
    EXPECT_EQ(slot->quantity, 2);
}

TEST(InventorySystemTest, SetMetadata)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("sword"));
    inv.AddItem("sword");

    inv.SetItemMetadata("sword", "enchant", "fire");
    inv.SetItemMetadata("sword", "durability", "100");

    const InventorySlot* slot = inv.GetSlot("sword");
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->metadata.size(), 2u);
}

TEST(InventorySystemTest, GetMetadata)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("bow"));
    inv.AddItem("bow");

    inv.SetItemMetadata("bow", "level", "5");
    EXPECT_EQ(inv.GetItemMetadata("bow", "level"), "5");
    EXPECT_EQ(inv.GetItemMetadata("bow", "missing"), "");
    EXPECT_EQ(inv.GetItemMetadata("nonexistent", "key"), "");
}

TEST(InventorySystemTest, GetAllItems)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("a"));
    inv.RegisterItemDef(MakeItem("b"));
    inv.RegisterItemDef(MakeItem("c"));

    inv.AddItem("a", 1);
    inv.AddItem("b", 5);
    inv.AddItem("c", 3);

    auto all = inv.GetAllItems();
    EXPECT_EQ(all.size(), 3u);
}

TEST(InventorySystemTest, GetByCategory)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("sword", "weapon"));
    inv.RegisterItemDef(MakeItem("axe", "weapon"));
    inv.RegisterItemDef(MakeItem("potion", "consumable"));

    inv.AddItem("sword");
    inv.AddItem("axe");
    inv.AddItem("potion", 3);

    auto weapons = inv.GetItemsByCategory("weapon");
    EXPECT_EQ(weapons.size(), 2u);

    auto consumables = inv.GetItemsByCategory("consumable");
    EXPECT_EQ(consumables.size(), 1u);

    auto empty = inv.GetItemsByCategory("armor");
    EXPECT_EQ(empty.size(), 0u);
}

TEST(InventorySystemTest, MaxSlots)
{
    InventorySystem inv;
    inv.SetMaxSlots(2);
    EXPECT_EQ(inv.GetMaxSlots(), 2u);

    inv.RegisterItemDef(MakeItem("a"));
    inv.RegisterItemDef(MakeItem("b"));
    inv.RegisterItemDef(MakeItem("c"));

    EXPECT_TRUE(inv.AddItem("a"));
    EXPECT_TRUE(inv.AddItem("b"));
    EXPECT_FALSE(inv.AddItem("c")); // slot limit reached
    EXPECT_EQ(inv.GetSlotCount(), 2u);
}

TEST(InventorySystemTest, TotalWeight)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("iron", "material", 99, 2.5f));
    inv.RegisterItemDef(MakeItem("feather", "material", 99, 0.1f));

    inv.AddItem("iron", 4);     // 4 * 2.5 = 10.0
    inv.AddItem("feather", 10); // 10 * 0.1 = 1.0

    EXPECT_FLOAT_EQ(inv.GetTotalWeight(), 11.0f);
}

TEST(InventorySystemTest, Clear)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("junk"));
    inv.AddItem("junk", 50);

    EXPECT_EQ(inv.GetSlotCount(), 1u);
    inv.Clear();
    EXPECT_EQ(inv.GetSlotCount(), 0u);
    EXPECT_EQ(inv.GetItemCount("junk"), 0);
}

TEST(InventorySystemTest, OnChangedCallback)
{
    InventorySystem inv;
    inv.RegisterItemDef(MakeItem("gem"));

    gx::Vector<InventoryChangedEvent> events;
    inv.OnChanged = [&](const InventoryChangedEvent& e) {
        events.push_back(e);
    };

    inv.AddItem("gem", 3);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].itemId, "gem");
    EXPECT_EQ(events[0].oldQuantity, 0);
    EXPECT_EQ(events[0].newQuantity, 3);
    EXPECT_EQ(events[0].action, InventoryChangedEvent::Added);

    inv.AddItem("gem", 2); // stack
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[1].action, InventoryChangedEvent::Modified);
    EXPECT_EQ(events[1].oldQuantity, 3);
    EXPECT_EQ(events[1].newQuantity, 5);

    inv.RemoveItem("gem", 2);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[2].action, InventoryChangedEvent::Modified);

    inv.RemoveItem("gem", 3); // remove all
    ASSERT_EQ(events.size(), 4u);
    EXPECT_EQ(events[3].action, InventoryChangedEvent::Removed);
    EXPECT_EQ(events[3].newQuantity, 0);
}
