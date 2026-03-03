/// @file test_ContainerHashMap.cpp
/// @brief Container HashMap, HashSet, HashMultiMap, HashMultiSet tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/HashMap.h"
#include "Container/HashSet.h"
#include "Container/HashMultiMap.h"
#include "Container/HashMultiSet.h"

#include <string>
#include <vector>
#include <algorithm>

using namespace gx::container;

// ============================================================================
// HashMap — Construction
// ============================================================================

TEST(HashMapConstruction, Default)
{
    HashMap<int, int> m;
    EXPECT_EQ(m.Size(), 0u);
    EXPECT_TRUE(m.Empty());
}

TEST(HashMapConstruction, WithInitialCapacity)
{
    HashMap<int, int> m(32);
    EXPECT_EQ(m.Size(), 0u);
    EXPECT_GE(m.Capacity(), 32u);
}

TEST(HashMapConstruction, InitializerList)
{
    HashMap<int, gx::String> m = {
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };
    EXPECT_EQ(m.Size(), 3u);
}

TEST(HashMapConstruction, CopyConstructor)
{
    HashMap<int, int> m1 = {{1, 10}, {2, 20}};
    HashMap<int, int> m2(m1);
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_TRUE(m2.Contains(1));
    EXPECT_TRUE(m2.Contains(2));
}

TEST(HashMapConstruction, MoveConstructor)
{
    HashMap<int, int> m1 = {{1, 10}, {2, 20}};
    HashMap<int, int> m2(std::move(m1));
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_EQ(m1.Size(), 0u);
}

// ============================================================================
// HashMap — Insert
// ============================================================================

TEST(HashMapInsert, InsertCopy)
{
    HashMap<int, int> m;
    auto result = m.Insert({1, 10});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, 10);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(HashMapInsert, InsertDuplicate)
{
    HashMap<int, int> m;
    m.Insert({1, 10});
    auto result = m.Insert({1, 20});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->second, 10); // original value
}

TEST(HashMapInsert, InsertMultipleElements)
{
    HashMap<int, int> m;
    for (int i = 0; i < 100; ++i)
        m.Insert({i, i * 10});
    EXPECT_EQ(m.Size(), 100u);
    for (int i = 0; i < 100; ++i)
        EXPECT_TRUE(m.Contains(i));
}

TEST(HashMapInsert, InsertMove)
{
    HashMap<int, gx::String> m;
    std::pair<const int, gx::String> p(1, "hello");
    auto result = m.Insert(std::move(p));
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->second, "hello");
}

// ============================================================================
// HashMap — operator[]
// ============================================================================

TEST(HashMapOperatorBracket, InsertAndAccess)
{
    HashMap<int, int> m;
    m[1] = 10;
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(HashMapOperatorBracket, DefaultConstructOnMiss)
{
    HashMap<int, int> m;
    int& val = m[42];
    EXPECT_EQ(val, 0); // default constructed
    EXPECT_EQ(m.Size(), 1u);
}

TEST(HashMapOperatorBracket, ModifyExisting)
{
    HashMap<int, int> m;
    m[1] = 10;
    m[1] = 20;
    EXPECT_EQ(m[1], 20);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(HashMapOperatorBracket, MoveKey)
{
    HashMap<gx::String, int> m;
    gx::String key = "hello";
    m[std::move(key)] = 42;
    EXPECT_EQ(m.Size(), 1u);
}

// ============================================================================
// HashMap — Find / Contains
// ============================================================================

TEST(HashMapFind, FindExisting)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    auto it = m.Find(2);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, 20);
}

TEST(HashMapFind, FindNonExisting)
{
    HashMap<int, int> m = {{1, 10}};
    auto it = m.Find(99);
    EXPECT_EQ(it, m.end());
}

TEST(HashMapFind, ContainsTrue)
{
    HashMap<int, int> m = {{1, 10}};
    EXPECT_TRUE(m.Contains(1));
}

TEST(HashMapFind, ContainsFalse)
{
    HashMap<int, int> m = {{1, 10}};
    EXPECT_FALSE(m.Contains(99));
}

TEST(HashMapFind, ContainsEmpty)
{
    HashMap<int, int> m;
    EXPECT_FALSE(m.Contains(0));
}

TEST(HashMapFind, FindConst)
{
    const HashMap<int, int> m = {{1, 10}, {2, 20}};
    auto it = m.Find(1);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, 10);
}

// ============================================================================
// HashMap — At
// ============================================================================

TEST(HashMapAt, AtExisting)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}};
    EXPECT_EQ(m.At(1), 10);
    EXPECT_EQ(m.At(2), 20);
}

TEST(HashMapAt, AtConst)
{
    const HashMap<int, int> m = {{1, 10}};
    EXPECT_EQ(m.At(1), 10);
}

// ============================================================================
// HashMap — Erase
// ============================================================================

TEST(HashMapErase, EraseByKey)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    size_t count = m.Erase(2);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(m.Size(), 2u);
    EXPECT_FALSE(m.Contains(2));
}

TEST(HashMapErase, EraseNonExisting)
{
    HashMap<int, int> m = {{1, 10}};
    size_t count = m.Erase(99);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(HashMapErase, EraseByIterator)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    auto it = m.Find(2);
    m.Erase(it);
    EXPECT_EQ(m.Size(), 2u);
    EXPECT_FALSE(m.Contains(2));
}

TEST(HashMapErase, EraseAllElements)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    m.Erase(1);
    m.Erase(2);
    m.Erase(3);
    EXPECT_TRUE(m.Empty());
}

// ============================================================================
// HashMap — Size / Empty / Clear
// ============================================================================

TEST(HashMapCapacity, SizeAfterInsert)
{
    HashMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({2, 20});
    EXPECT_EQ(m.Size(), 2u);
}

TEST(HashMapCapacity, EmptyInitially)
{
    HashMap<int, int> m;
    EXPECT_TRUE(m.Empty());
}

TEST(HashMapCapacity, NotEmptyAfterInsert)
{
    HashMap<int, int> m;
    m.Insert({1, 10});
    EXPECT_FALSE(m.Empty());
}

TEST(HashMapCapacity, Clear)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    m.Clear();
    EXPECT_EQ(m.Size(), 0u);
    EXPECT_TRUE(m.Empty());
}

// ============================================================================
// HashMap — Iteration
// ============================================================================

TEST(HashMapIteration, IterateAll)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    int sum = 0;
    for (auto& [k, v] : m)
        sum += v;
    EXPECT_EQ(sum, 60);
}

TEST(HashMapIteration, IterateEmpty)
{
    HashMap<int, int> m;
    int count = 0;
    for ([[maybe_unused]] auto& p : m)
        ++count;
    EXPECT_EQ(count, 0);
}

TEST(HashMapIteration, ConstIteration)
{
    const HashMap<int, int> m = {{1, 10}, {2, 20}};
    int sum = 0;
    for (auto& [k, v] : m)
        sum += v;
    EXPECT_EQ(sum, 30);
}

// ============================================================================
// HashMap — Copy / Move
// ============================================================================

TEST(HashMapCopyMove, CopyAssignment)
{
    HashMap<int, int> m1 = {{1, 10}, {2, 20}};
    HashMap<int, int> m2;
    m2 = m1;
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_EQ(m2.At(1), 10);
}

TEST(HashMapCopyMove, MoveAssignment)
{
    HashMap<int, int> m1 = {{1, 10}, {2, 20}};
    HashMap<int, int> m2;
    m2 = std::move(m1);
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_EQ(m1.Size(), 0u);
}

TEST(HashMapCopyMove, SelfAssignment)
{
    HashMap<int, int> m = {{1, 10}};
    m = m;
    EXPECT_EQ(m.Size(), 1u);
}

// ============================================================================
// HashMap — Rehash / Load Factor
// ============================================================================

TEST(HashMapRehash, RehashGrows)
{
    HashMap<int, int> m;
    m.Rehash(64);
    EXPECT_GE(m.Capacity(), 64u);
}

TEST(HashMapRehash, ReserveElements)
{
    HashMap<int, int> m;
    m.Reserve(100);
    // After reserving for 100 elements, should be able to insert without rehash
    for (int i = 0; i < 100; ++i)
        m.Insert({i, i});
    EXPECT_EQ(m.Size(), 100u);
}

TEST(HashMapRehash, LoadFactor)
{
    HashMap<int, int> m;
    EXPECT_FLOAT_EQ(m.LoadFactor(), 0.0f);
    m.Reserve(10);
    m.Insert({1, 1});
    EXPECT_GT(m.LoadFactor(), 0.0f);
    EXPECT_LE(m.LoadFactor(), m.MaxLoadFactor());
}

TEST(HashMapRehash, MaxLoadFactor)
{
    HashMap<int, int> m;
    EXPECT_FLOAT_EQ(m.MaxLoadFactor(), 0.75f);
}

TEST(HashMapRehash, AutoRehashOnInsert)
{
    HashMap<int, int> m;
    // Insert many elements to trigger multiple rehashes
    for (int i = 0; i < 500; ++i)
        m.Insert({i, i});
    EXPECT_EQ(m.Size(), 500u);
    // Load factor should stay below max
    EXPECT_LE(m.LoadFactor(), m.MaxLoadFactor());
}

// ============================================================================
// HashMap — InsertOrAssign / TryEmplace
// ============================================================================

TEST(HashMapInsertOrAssign, InsertNew)
{
    HashMap<int, int> m;
    auto [it, inserted] = m.InsertOrAssign(1, 10);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, 10);
}

TEST(HashMapInsertOrAssign, AssignExisting)
{
    HashMap<int, int> m = {{1, 10}};
    auto [it, inserted] = m.InsertOrAssign(1, 20);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, 20);
}

TEST(HashMapTryEmplace, EmplaceNew)
{
    HashMap<int, gx::String> m;
    auto [it, inserted] = m.TryEmplace(1, "hello");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "hello");
}

TEST(HashMapTryEmplace, NoEmplaceExisting)
{
    HashMap<int, gx::String> m;
    m.TryEmplace(1, "hello");
    auto [it, inserted] = m.TryEmplace(1, "world");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "hello"); // unchanged
}

TEST(HashMapTryEmplace, MoveKey)
{
    HashMap<gx::String, int> m;
    gx::String key = "hello";
    auto [it, inserted] = m.TryEmplace(std::move(key), 42);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, 42);
}

// ============================================================================
// HashMap — Validate
// ============================================================================

TEST(HashMapValidate, ValidateEmpty)
{
    HashMap<int, int> m;
    EXPECT_TRUE(m.Validate());
}

TEST(HashMapValidate, ValidateAfterInserts)
{
    HashMap<int, int> m;
    for (int i = 0; i < 100; ++i)
        m.Insert({i, i * 10});
    EXPECT_TRUE(m.Validate());
}

TEST(HashMapValidate, ValidateAfterErases)
{
    HashMap<int, int> m;
    for (int i = 0; i < 50; ++i)
        m.Insert({i, i});
    for (int i = 0; i < 25; ++i)
        m.Erase(i);
    EXPECT_TRUE(m.Validate());
}

// ============================================================================
// HashMap — Comparison
// ============================================================================

TEST(HashMapComparison, EqualMaps)
{
    HashMap<int, int> a = {{1, 10}, {2, 20}};
    HashMap<int, int> b = {{2, 20}, {1, 10}};
    EXPECT_TRUE(a == b);
}

TEST(HashMapComparison, NotEqualMaps)
{
    HashMap<int, int> a = {{1, 10}};
    HashMap<int, int> b = {{1, 10}, {2, 20}};
    EXPECT_TRUE(a != b);
}

// ============================================================================
// HashMap — Count / EqualRange
// ============================================================================

TEST(HashMapCount, CountExisting)
{
    HashMap<int, int> m = {{1, 10}, {2, 20}};
    EXPECT_EQ(m.Count(1), 1u);
}

TEST(HashMapCount, CountNonExisting)
{
    HashMap<int, int> m = {{1, 10}};
    EXPECT_EQ(m.Count(99), 0u);
}

// ============================================================================
// HashMap — Swap
// ============================================================================

TEST(HashMapSwap, SwapMaps)
{
    HashMap<int, int> a = {{1, 10}};
    HashMap<int, int> b = {{2, 20}, {3, 30}};
    a.Swap(b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 1u);
    EXPECT_TRUE(a.Contains(2));
    EXPECT_TRUE(b.Contains(1));
}

// ============================================================================
// HashMap — String keys
// ============================================================================

TEST(HashMapStringKey, InsertAndFind)
{
    HashMap<gx::String, int> m;
    m.Insert({"hello", 1});
    m.Insert({"world", 2});
    EXPECT_EQ(m.Find("hello")->second, 1);
    EXPECT_EQ(m.Find("world")->second, 2);
}

// ============================================================================
// HashSet — Basic
// ============================================================================

TEST(HashSetBasic, DefaultConstruction)
{
    HashSet<int> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(HashSetBasic, InitializerList)
{
    HashSet<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(s.Size(), 5u);
}

TEST(HashSetBasic, InsertNew)
{
    HashSet<int> s;
    auto result = s.Insert(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(HashSetBasic, InsertDuplicate)
{
    HashSet<int> s = {1, 2, 3};
    auto result = s.Insert(2);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(s.Size(), 3u);
}

TEST(HashSetBasic, FindExisting)
{
    HashSet<int> s = {10, 20, 30};
    auto it = s.Find(20);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 20);
}

TEST(HashSetBasic, FindNonExisting)
{
    HashSet<int> s = {10, 20, 30};
    EXPECT_EQ(s.Find(99), s.end());
}

TEST(HashSetBasic, Contains)
{
    HashSet<int> s = {1, 2, 3};
    EXPECT_TRUE(s.Contains(2));
    EXPECT_FALSE(s.Contains(99));
}

TEST(HashSetBasic, EraseByKey)
{
    HashSet<int> s = {1, 2, 3};
    s.Erase(2);
    EXPECT_EQ(s.Size(), 2u);
    EXPECT_FALSE(s.Contains(2));
}

TEST(HashSetBasic, EraseByIterator)
{
    HashSet<int> s = {1, 2, 3};
    auto it = s.Find(2);
    s.Erase(it);
    EXPECT_EQ(s.Size(), 2u);
}

TEST(HashSetBasic, Clear)
{
    HashSet<int> s = {1, 2, 3};
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(HashSetBasic, Iteration)
{
    HashSet<int> s = {1, 2, 3, 4, 5};
    gx::Vector<int> values;
    for (int v : s)
        values.push_back(v);
    std::sort(values.begin(), values.end());
    EXPECT_EQ(values, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(HashSetBasic, CopyConstructor)
{
    HashSet<int> s1 = {1, 2, 3};
    HashSet<int> s2(s1);
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_TRUE(s2.Contains(1));
}

TEST(HashSetBasic, MoveConstructor)
{
    HashSet<int> s1 = {1, 2, 3};
    HashSet<int> s2(std::move(s1));
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_EQ(s1.Size(), 0u);
}

TEST(HashSetBasic, InsertMany)
{
    HashSet<int> s;
    for (int i = 0; i < 200; ++i)
        s.Insert(i);
    EXPECT_EQ(s.Size(), 200u);
}

TEST(HashSetBasic, Count)
{
    HashSet<int> s = {1, 2, 3};
    EXPECT_EQ(s.Count(1), 1u);
    EXPECT_EQ(s.Count(99), 0u);
}

TEST(HashSetBasic, Validate)
{
    HashSet<int> s = {1, 2, 3, 4, 5};
    EXPECT_TRUE(s.Validate());
}

TEST(HashSetBasic, WithCapacity)
{
    HashSet<int> s(64);
    EXPECT_GE(s.Capacity(), 64u);
    EXPECT_TRUE(s.Empty());
}

TEST(HashSetBasic, EqualSets)
{
    HashSet<int> a = {1, 2, 3};
    HashSet<int> b = {3, 2, 1};
    EXPECT_TRUE(a == b);
}

TEST(HashSetBasic, NotEqualSets)
{
    HashSet<int> a = {1, 2, 3};
    HashSet<int> b = {1, 2, 4};
    EXPECT_TRUE(a != b);
}

TEST(HashSetBasic, CopyAssignment)
{
    HashSet<int> s1 = {1, 2, 3};
    HashSet<int> s2;
    s2 = s1;
    EXPECT_EQ(s2.Size(), 3u);
}

TEST(HashSetBasic, MoveAssignment)
{
    HashSet<int> s1 = {1, 2, 3};
    HashSet<int> s2;
    s2 = std::move(s1);
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_EQ(s1.Size(), 0u);
}

// ============================================================================
// HashMultiMap
// ============================================================================

TEST(HashMultiMapBasic, DefaultConstruction)
{
    HashMultiMap<int, int> m;
    EXPECT_TRUE(m.Empty());
}

TEST(HashMultiMapBasic, InsertDuplicateKeys)
{
    HashMultiMap<int, gx::String> m;
    m.Insert({1, "a"});
    m.Insert({1, "b"});
    m.Insert({1, "c"});
    EXPECT_EQ(m.Size(), 3u);
}

TEST(HashMultiMapBasic, Count)
{
    HashMultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    m.Insert({1, 30});
    m.Insert({2, 40});
    EXPECT_EQ(m.Count(1), 3u);
    EXPECT_EQ(m.Count(2), 1u);
    EXPECT_EQ(m.Count(99), 0u);
}

TEST(HashMultiMapBasic, FindReturnsFirst)
{
    HashMultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    auto it = m.Find(1);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->first, 1);
}

TEST(HashMultiMapBasic, EqualRange)
{
    HashMultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    m.Insert({1, 30});
    m.Insert({2, 40});

    auto [first, last] = m.EqualRange(1);
    int count = 0;
    int sum = 0;
    for (auto it = first; it != last; ++it) {
        EXPECT_EQ(it->first, 1);
        sum += it->second;
        ++count;
    }
    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum, 60);
}

TEST(HashMultiMapBasic, EqualRangeEmpty)
{
    HashMultiMap<int, int> m;
    auto [first, last] = m.EqualRange(1);
    EXPECT_EQ(first, last);
}

TEST(HashMultiMapBasic, EraseByKey)
{
    HashMultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    m.Insert({2, 30});
    size_t count = m.Erase(1);
    EXPECT_EQ(count, 1u); // Erase(key) removes only one
    EXPECT_EQ(m.Size(), 2u);
}

TEST(HashMultiMapBasic, InitializerList)
{
    HashMultiMap<int, int> m = {{1, 10}, {1, 20}, {2, 30}};
    EXPECT_EQ(m.Size(), 3u);
}

TEST(HashMultiMapBasic, CopyConstructor)
{
    HashMultiMap<int, int> m1 = {{1, 10}, {1, 20}};
    HashMultiMap<int, int> m2(m1);
    EXPECT_EQ(m2.Size(), 2u);
}

TEST(HashMultiMapBasic, MoveConstructor)
{
    HashMultiMap<int, int> m1 = {{1, 10}, {1, 20}};
    HashMultiMap<int, int> m2(std::move(m1));
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_EQ(m1.Size(), 0u);
}

TEST(HashMultiMapBasic, Clear)
{
    HashMultiMap<int, int> m = {{1, 10}, {1, 20}};
    m.Clear();
    EXPECT_TRUE(m.Empty());
}

TEST(HashMultiMapBasic, Contains)
{
    HashMultiMap<int, int> m = {{1, 10}, {1, 20}};
    EXPECT_TRUE(m.Contains(1));
    EXPECT_FALSE(m.Contains(99));
}

TEST(HashMultiMapBasic, Validate)
{
    HashMultiMap<int, int> m;
    for (int i = 0; i < 50; ++i)
        m.Insert({i % 10, i});
    EXPECT_TRUE(m.Validate());
}

TEST(HashMultiMapBasic, Iteration)
{
    HashMultiMap<int, int> m = {{1, 10}, {2, 20}, {1, 30}};
    int sum = 0;
    for (auto& [k, v] : m)
        sum += v;
    EXPECT_EQ(sum, 60);
}

TEST(HashMultiMapBasic, WithCapacity)
{
    HashMultiMap<int, int> m(32);
    EXPECT_TRUE(m.Empty());
    EXPECT_GE(m.Capacity(), 32u);
}

// ============================================================================
// HashMultiSet
// ============================================================================

TEST(HashMultiSetBasic, DefaultConstruction)
{
    HashMultiSet<int> s;
    EXPECT_TRUE(s.Empty());
}

TEST(HashMultiSetBasic, InsertDuplicates)
{
    HashMultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(1);
    EXPECT_EQ(s.Size(), 3u);
}

TEST(HashMultiSetBasic, Count)
{
    HashMultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    EXPECT_EQ(s.Count(1), 2u);
    EXPECT_EQ(s.Count(2), 1u);
    EXPECT_EQ(s.Count(99), 0u);
}

TEST(HashMultiSetBasic, Find)
{
    HashMultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    auto it = s.Find(1);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 1);
}

TEST(HashMultiSetBasic, EqualRange)
{
    HashMultiSet<int> s;
    s.Insert(5);
    s.Insert(5);
    s.Insert(5);
    s.Insert(10);

    auto [first, last] = s.EqualRange(5);
    int count = 0;
    for (auto it = first; it != last; ++it) {
        EXPECT_EQ(*it, 5);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST(HashMultiSetBasic, EqualRangeNotFound)
{
    HashMultiSet<int> s = {1, 2, 3};
    auto [first, last] = s.EqualRange(99);
    EXPECT_EQ(first, last);
}

TEST(HashMultiSetBasic, EraseByKey)
{
    HashMultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    s.Erase(1); // removes one
    EXPECT_EQ(s.Size(), 2u);
}

TEST(HashMultiSetBasic, InitializerList)
{
    HashMultiSet<int> s = {1, 1, 2, 3, 3};
    EXPECT_EQ(s.Size(), 5u);
}

TEST(HashMultiSetBasic, CopyConstructor)
{
    HashMultiSet<int> s1 = {1, 1, 2};
    HashMultiSet<int> s2(s1);
    EXPECT_EQ(s2.Size(), 3u);
}

TEST(HashMultiSetBasic, MoveConstructor)
{
    HashMultiSet<int> s1 = {1, 1, 2};
    HashMultiSet<int> s2(std::move(s1));
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_EQ(s1.Size(), 0u);
}

TEST(HashMultiSetBasic, Clear)
{
    HashMultiSet<int> s = {1, 1, 2};
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(HashMultiSetBasic, Contains)
{
    HashMultiSet<int> s = {1, 2, 3};
    EXPECT_TRUE(s.Contains(1));
    EXPECT_FALSE(s.Contains(99));
}

TEST(HashMultiSetBasic, Validate)
{
    HashMultiSet<int> s;
    for (int i = 0; i < 50; ++i)
        s.Insert(i % 10);
    EXPECT_TRUE(s.Validate());
}

TEST(HashMultiSetBasic, Iteration)
{
    HashMultiSet<int> s = {1, 2, 3};
    int sum = 0;
    for (int v : s)
        sum += v;
    EXPECT_EQ(sum, 6);
}

TEST(HashMultiSetBasic, ManyDuplicates)
{
    HashMultiSet<int> s;
    for (int i = 0; i < 100; ++i)
        s.Insert(42);
    EXPECT_EQ(s.Size(), 100u);
    EXPECT_EQ(s.Count(42), 100u);
}

TEST(HashMultiSetBasic, WithCapacity)
{
    HashMultiSet<int> s(32);
    EXPECT_TRUE(s.Empty());
    EXPECT_GE(s.Capacity(), 32u);
}
