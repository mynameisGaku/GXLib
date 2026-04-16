/// @file test_ContainerIntrusiveHash.cpp
/// @brief IntrusiveHashSet, IntrusiveHashMap, IntrusiveHashMultiSet, IntrusiveHashMultiMap tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Hash.h"
#include "Container/IntrusiveHashSet.h"
#include "Container/IntrusiveHashMap.h"
#include "Container/IntrusiveHashMultiSet.h"
#include "Container/IntrusiveHashMultiMap.h"

#include <vector>
#include <algorithm>

using namespace gx::container;

// ============================================================================
// Test types for IntrusiveHashSet
// ============================================================================

struct HSItem
{
    int value = 0;
    IntrusiveHashNode hashNode{};

    HSItem() = default;
    explicit HSItem(int v) : value(v) {}
};

struct HSItemHash
{
    size_t operator()(const HSItem& item) const noexcept
    {
        return gx::container::Hash<int>()(item.value);
    }
};

struct HSItemEqual
{
    bool operator()(const HSItem& a, const HSItem& b) const noexcept
    {
        return a.value == b.value;
    }
};

using HSet = IntrusiveHashSet<HSItem, &HSItem::hashNode, HSItemHash, HSItemEqual>;

// ============================================================================
// IntrusiveHashSet Tests
// ============================================================================

TEST(IntrusiveHashSet, DefaultConstruction)
{
    HSet set;
    EXPECT_TRUE(set.Empty());
    EXPECT_EQ(set.Size(), 0u);
    EXPECT_EQ(set.BucketCount(), 0u);
}

TEST(IntrusiveHashSet, ConstructWithBucketCount)
{
    HSet set(32);
    EXPECT_TRUE(set.Empty());
    EXPECT_EQ(set.BucketCount(), 32u);
}

TEST(IntrusiveHashSet, Insert)
{
    HSet set;
    HSItem a(1);
    set.Insert(a);
    EXPECT_EQ(set.Size(), 1u);
    EXPECT_FALSE(set.Empty());
}

TEST(IntrusiveHashSet, InsertMultiple)
{
    HSet set;
    HSItem items[5] = {HSItem(10), HSItem(20), HSItem(30), HSItem(40), HSItem(50)};
    for (auto& item : items)
        set.Insert(item);
    EXPECT_EQ(set.Size(), 5u);
}

TEST(IntrusiveHashSet, Find)
{
    HSet set;
    HSItem a(42), b(99);
    set.Insert(a);
    set.Insert(b);
    HSItem query(42);
    auto* found = set.Find(query);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, 42);
}

TEST(IntrusiveHashSet, FindMissing)
{
    HSet set;
    HSItem a(42);
    set.Insert(a);
    HSItem query(999);
    auto* found = set.Find(query);
    EXPECT_EQ(found, nullptr);
}

TEST(IntrusiveHashSet, FindWithPredicate)
{
    HSet set;
    HSItem a(1), b(2), c(3);
    set.Insert(a);
    set.Insert(b);
    set.Insert(c);
    auto* found = set.Find(2, gx::container::Hash<int>()(2),
        [](const HSItem& item, const int& key) { return item.value == key; });
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, 2);
}

TEST(IntrusiveHashSet, Remove)
{
    HSet set;
    HSItem a(1), b(2), c(3);
    set.Insert(a);
    set.Insert(b);
    set.Insert(c);
    set.Remove(b);
    EXPECT_EQ(set.Size(), 2u);
    HSItem query(2);
    EXPECT_EQ(set.Find(query), nullptr);
}

TEST(IntrusiveHashSet, RemoveAndReinsert)
{
    HSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    set.Remove(a);
    EXPECT_EQ(set.Size(), 1u);
    // Reset the node for reinsertion
    a.hashNode = {};
    set.Insert(a);
    EXPECT_EQ(set.Size(), 2u);
}

TEST(IntrusiveHashSet, Clear)
{
    HSet set;
    HSItem a(1), b(2), c(3);
    set.Insert(a);
    set.Insert(b);
    set.Insert(c);
    set.Clear();
    EXPECT_TRUE(set.Empty());
    EXPECT_EQ(set.Size(), 0u);
    // Nodes should be reset
    EXPECT_EQ(a.hashNode.hashNext, nullptr);
}

TEST(IntrusiveHashSet, Rehash)
{
    HSet set(4);
    HSItem items[10];
    for (int i = 0; i < 10; ++i)
    {
        items[i] = HSItem(i);
        set.Insert(items[i]);
    }
    set.Rehash(32);
    EXPECT_EQ(set.BucketCount(), 32u);
    EXPECT_EQ(set.Size(), 10u);
    // Verify all items still findable
    for (int i = 0; i < 10; ++i)
    {
        HSItem query(i);
        ASSERT_NE(set.Find(query), nullptr);
    }
}

TEST(IntrusiveHashSet, LoadFactor)
{
    HSet set(10);
    EXPECT_FLOAT_EQ(set.LoadFactor(), 0.0f);
    HSItem a(1);
    set.Insert(a);
    EXPECT_FLOAT_EQ(set.LoadFactor(), 0.1f);
}

TEST(IntrusiveHashSet, AutoRehash)
{
    HSet set;
    HSItem items[20];
    for (int i = 0; i < 20; ++i)
    {
        items[i] = HSItem(i);
        set.Insert(items[i]);
    }
    EXPECT_EQ(set.Size(), 20u);
    EXPECT_GT(set.BucketCount(), 16u);
}

TEST(IntrusiveHashSet, Iteration)
{
    HSet set;
    HSItem items[5] = {HSItem(1), HSItem(2), HSItem(3), HSItem(4), HSItem(5)};
    for (auto& item : items)
        set.Insert(item);

    gx::Vector<int> result;
    for (auto it = set.begin(); it != set.end(); ++it)
        result.push_back(it->value);

    std::sort(result.begin(), result.end());
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(IntrusiveHashSet, MoveConstruction)
{
    HSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    HSet moved(std::move(set));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_TRUE(set.Empty());
}

TEST(IntrusiveHashSet, MoveAssignment)
{
    HSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    HSet other;
    other = std::move(set);
    EXPECT_EQ(other.Size(), 2u);
    EXPECT_TRUE(set.Empty());
}

TEST(IntrusiveHashSet, Validate)
{
    HSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    EXPECT_TRUE(set.Validate());
}

TEST(IntrusiveHashSet, EmptyIteration)
{
    HSet set;
    int count = 0;
    for (auto it = set.begin(); it != set.end(); ++it)
        ++count;
    EXPECT_EQ(count, 0);
}

TEST(IntrusiveHashSet, EmptyFind)
{
    HSet set;
    HSItem query(1);
    EXPECT_EQ(set.Find(query), nullptr);
}

TEST(IntrusiveHashSet, ManyElements)
{
    HSet set;
    constexpr int N = 100;
    HSItem items[N];
    for (int i = 0; i < N; ++i)
    {
        items[i] = HSItem(i);
        set.Insert(items[i]);
    }
    EXPECT_EQ(set.Size(), static_cast<size_t>(N));
    for (int i = 0; i < N; ++i)
    {
        HSItem query(i);
        ASSERT_NE(set.Find(query), nullptr);
    }
}

// ============================================================================
// Test types for IntrusiveHashMap
// ============================================================================

struct HMItem
{
    int key = 0;
    gx::String value;
    IntrusiveHashNode hashNode{};

    HMItem() = default;
    HMItem(int k, const gx::String& v) : key(k), value(v) {}
};

struct HMKeyExtract
{
    const int& operator()(const HMItem& item) const noexcept { return item.key; }
};

struct HMKeyHash
{
    size_t operator()(const int& key) const noexcept { return gx::container::Hash<int>()(key); }
};

struct HMKeyEqual
{
    bool operator()(const int& a, const int& b) const noexcept { return a == b; }
};

using HMap = IntrusiveHashMap<HMItem, &HMItem::hashNode, int, HMKeyExtract, HMKeyHash, HMKeyEqual>;

// ============================================================================
// IntrusiveHashMap Tests
// ============================================================================

TEST(IntrusiveHashMap, DefaultConstruction)
{
    HMap map;
    EXPECT_TRUE(map.Empty());
    EXPECT_EQ(map.Size(), 0u);
}

TEST(IntrusiveHashMap, ConstructWithBucketCount)
{
    HMap map(32);
    EXPECT_TRUE(map.Empty());
    EXPECT_EQ(map.BucketCount(), 32u);
}

TEST(IntrusiveHashMap, Insert)
{
    HMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_EQ(map.Size(), 1u);
}

TEST(IntrusiveHashMap, FindByKey)
{
    HMap map;
    HMItem a(1, "one"), b(2, "two"), c(3, "three");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);

    auto* found = map.Find(2);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, "two");
}

TEST(IntrusiveHashMap, FindMissing)
{
    HMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_EQ(map.Find(999), nullptr);
}

TEST(IntrusiveHashMap, Contains)
{
    HMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_TRUE(map.Contains(1));
    EXPECT_FALSE(map.Contains(2));
}

TEST(IntrusiveHashMap, Remove)
{
    HMap map;
    HMItem a(1, "one"), b(2, "two"), c(3, "three");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);
    map.Remove(b);
    EXPECT_EQ(map.Size(), 2u);
    EXPECT_FALSE(map.Contains(2));
}

TEST(IntrusiveHashMap, Clear)
{
    HMap map;
    HMItem a(1, "one"), b(2, "two");
    map.Insert(a);
    map.Insert(b);
    map.Clear();
    EXPECT_TRUE(map.Empty());
}

TEST(IntrusiveHashMap, Rehash)
{
    HMap map(4);
    HMItem items[5] = {{1, "a"}, {2, "b"}, {3, "c"}, {4, "d"}, {5, "e"}};
    for (auto& item : items)
        map.Insert(item);
    map.Rehash(32);
    EXPECT_EQ(map.BucketCount(), 32u);
    EXPECT_EQ(map.Size(), 5u);
    for (int i = 1; i <= 5; ++i)
        ASSERT_NE(map.Find(i), nullptr);
}

TEST(IntrusiveHashMap, Iteration)
{
    HMap map;
    HMItem a(1, "one"), b(2, "two"), c(3, "three");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);

    gx::Vector<int> keys;
    for (auto it = map.begin(); it != map.end(); ++it)
        keys.push_back(it->key);
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys, (gx::Vector<int>{1, 2, 3}));
}

TEST(IntrusiveHashMap, MoveConstruction)
{
    HMap map;
    HMItem a(1, "one"), b(2, "two");
    map.Insert(a);
    map.Insert(b);
    HMap moved(std::move(map));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_TRUE(map.Empty());
}

TEST(IntrusiveHashMap, Validate)
{
    HMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_TRUE(map.Validate());
}

TEST(IntrusiveHashMap, ManyElements)
{
    HMap map;
    constexpr int N = 50;
    gx::Vector<HMItem> items;
    items.reserve(N);
    for (int i = 0; i < N; ++i)
        items.emplace_back(i, "val" + std::to_string(i));
    for (auto& item : items)
        map.Insert(item);
    EXPECT_EQ(map.Size(), static_cast<size_t>(N));
    for (int i = 0; i < N; ++i)
    {
        auto* found = map.Find(i);
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->key, i);
    }
}

// ============================================================================
// Test types for IntrusiveHashMultiSet
// ============================================================================

using HMSet = IntrusiveHashMultiSet<HSItem, &HSItem::hashNode, HSItemHash, HSItemEqual>;

// ============================================================================
// IntrusiveHashMultiSet Tests
// ============================================================================

TEST(IntrusiveHashMultiSet, DefaultConstruction)
{
    HMSet set;
    EXPECT_TRUE(set.Empty());
    EXPECT_EQ(set.Size(), 0u);
}

TEST(IntrusiveHashMultiSet, InsertDuplicates)
{
    HMSet set(16);
    HSItem a(1), b(1), c(1);
    set.Insert(a);
    set.Insert(b);
    set.Insert(c);
    EXPECT_EQ(set.Size(), 3u);
}

TEST(IntrusiveHashMultiSet, Find)
{
    HMSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    HSItem query(1);
    auto* found = set.Find(query);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, 1);
}

TEST(IntrusiveHashMultiSet, FindMissing)
{
    HMSet set;
    HSItem a(1);
    set.Insert(a);
    HSItem query(999);
    EXPECT_EQ(set.Find(query), nullptr);
}

TEST(IntrusiveHashMultiSet, Count)
{
    HMSet set(16);
    HSItem items[5] = {HSItem(1), HSItem(1), HSItem(1), HSItem(2), HSItem(2)};
    for (auto& item : items)
        set.Insert(item);

    auto count1 = set.Count(1, gx::container::Hash<int>()(1),
        [](const HSItem& item, const int& key) { return item.value == key; });
    EXPECT_GE(count1, 1u);
}

TEST(IntrusiveHashMultiSet, Remove)
{
    HMSet set;
    HSItem a(1), b(1), c(2);
    set.Insert(a);
    set.Insert(b);
    set.Insert(c);
    set.Remove(a);
    EXPECT_EQ(set.Size(), 2u);
}

TEST(IntrusiveHashMultiSet, Clear)
{
    HMSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    set.Clear();
    EXPECT_TRUE(set.Empty());
}

TEST(IntrusiveHashMultiSet, Rehash)
{
    HMSet set(4);
    HSItem items[8];
    for (int i = 0; i < 8; ++i)
    {
        items[i] = HSItem(i % 3); // duplicates
        set.Insert(items[i]);
    }
    set.Rehash(32);
    EXPECT_EQ(set.Size(), 8u);
    EXPECT_EQ(set.BucketCount(), 32u);
}

TEST(IntrusiveHashMultiSet, MoveConstruction)
{
    HMSet set;
    HSItem a(1), b(2);
    set.Insert(a);
    set.Insert(b);
    HMSet moved(std::move(set));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_TRUE(set.Empty());
}

TEST(IntrusiveHashMultiSet, Validate)
{
    HMSet set;
    HSItem a(1);
    set.Insert(a);
    EXPECT_TRUE(set.Validate());
}

TEST(IntrusiveHashMultiSet, Iteration)
{
    HMSet set;
    HSItem a(1), b(2), c(3);
    set.Insert(a);
    set.Insert(b);
    set.Insert(c);

    int count = 0;
    for (auto it = set.begin(); it != set.end(); ++it)
        ++count;
    EXPECT_EQ(count, 3);
}

// ============================================================================
// Test types for IntrusiveHashMultiMap
// ============================================================================

using HMMap = IntrusiveHashMultiMap<HMItem, &HMItem::hashNode, int, HMKeyExtract, HMKeyHash, HMKeyEqual>;

// ============================================================================
// IntrusiveHashMultiMap Tests
// ============================================================================

TEST(IntrusiveHashMultiMap, DefaultConstruction)
{
    HMMap map;
    EXPECT_TRUE(map.Empty());
    EXPECT_EQ(map.Size(), 0u);
}

TEST(IntrusiveHashMultiMap, InsertDuplicateKeys)
{
    HMMap map;
    HMItem a(1, "one-a"), b(1, "one-b"), c(2, "two");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);
    EXPECT_EQ(map.Size(), 3u);
}

TEST(IntrusiveHashMultiMap, FindByKey)
{
    HMMap map;
    HMItem a(1, "one"), b(2, "two");
    map.Insert(a);
    map.Insert(b);
    auto* found = map.Find(1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->key, 1);
}

TEST(IntrusiveHashMultiMap, FindMissing)
{
    HMMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_EQ(map.Find(999), nullptr);
}

TEST(IntrusiveHashMultiMap, Contains)
{
    HMMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_TRUE(map.Contains(1));
    EXPECT_FALSE(map.Contains(2));
}

TEST(IntrusiveHashMultiMap, FindAll)
{
    HMMap map(16);
    HMItem a(1, "one-a"), b(1, "one-b"), c(2, "two");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);

    gx::Vector<gx::String> found;
    map.FindAll(1, [&found](HMItem& item) {
        found.push_back(item.value);
    });
    EXPECT_GE(found.size(), 1u);
}

TEST(IntrusiveHashMultiMap, Remove)
{
    HMMap map;
    HMItem a(1, "one-a"), b(1, "one-b"), c(2, "two");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);
    map.Remove(a);
    EXPECT_EQ(map.Size(), 2u);
}

TEST(IntrusiveHashMultiMap, Clear)
{
    HMMap map;
    HMItem a(1, "one"), b(2, "two");
    map.Insert(a);
    map.Insert(b);
    map.Clear();
    EXPECT_TRUE(map.Empty());
}

TEST(IntrusiveHashMultiMap, Rehash)
{
    HMMap map(4);
    HMItem items[6] = {{1, "a"}, {1, "b"}, {2, "c"}, {2, "d"}, {3, "e"}, {3, "f"}};
    for (auto& item : items)
        map.Insert(item);
    map.Rehash(32);
    EXPECT_EQ(map.Size(), 6u);
    EXPECT_EQ(map.BucketCount(), 32u);
}

TEST(IntrusiveHashMultiMap, MoveConstruction)
{
    HMMap map;
    HMItem a(1, "one"), b(2, "two");
    map.Insert(a);
    map.Insert(b);
    HMMap moved(std::move(map));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_TRUE(map.Empty());
}

TEST(IntrusiveHashMultiMap, Validate)
{
    HMMap map;
    HMItem a(1, "one");
    map.Insert(a);
    EXPECT_TRUE(map.Validate());
}

TEST(IntrusiveHashMultiMap, Iteration)
{
    HMMap map;
    HMItem a(1, "one"), b(2, "two"), c(3, "three");
    map.Insert(a);
    map.Insert(b);
    map.Insert(c);

    gx::Vector<int> keys;
    for (auto it = map.begin(); it != map.end(); ++it)
        keys.push_back(it->key);
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys, (gx::Vector<int>{1, 2, 3}));
}

TEST(IntrusiveHashMultiMap, ManyDuplicateKeys)
{
    HMMap map;
    constexpr int N = 20;
    gx::Vector<HMItem> items;
    items.reserve(N);
    for (int i = 0; i < N; ++i)
        items.emplace_back(i % 5, "val" + std::to_string(i));
    for (auto& item : items)
        map.Insert(item);
    EXPECT_EQ(map.Size(), static_cast<size_t>(N));
}
