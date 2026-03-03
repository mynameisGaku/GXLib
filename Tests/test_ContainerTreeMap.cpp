/// @file test_ContainerTreeMap.cpp
/// @brief Container Map, Set, MultiMap, MultiSet tests (red-black tree)

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Map.h"
#include "Container/Set.h"
#include "Container/MultiMap.h"
#include "Container/MultiSet.h"

#include <string>
#include <vector>
#include <algorithm>

using namespace gx::container;

// ============================================================================
// Map — Construction
// ============================================================================

TEST(MapConstruction, Default)
{
    Map<int, int> m;
    EXPECT_EQ(m.Size(), 0u);
    EXPECT_TRUE(m.Empty());
}

TEST(MapConstruction, WithComparator)
{
    Less<int> cmp;
    Map<int, int> m(cmp);
    EXPECT_TRUE(m.Empty());
}

TEST(MapConstruction, InitializerList)
{
    Map<int, gx::String> m = {
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };
    EXPECT_EQ(m.Size(), 3u);
}

TEST(MapConstruction, CopyConstructor)
{
    Map<int, int> m1 = {{1, 10}, {2, 20}, {3, 30}};
    Map<int, int> m2(m1);
    EXPECT_EQ(m2.Size(), 3u);
    EXPECT_TRUE(m2.Contains(1));
    EXPECT_TRUE(m2.Contains(2));
    EXPECT_TRUE(m2.Contains(3));
}

TEST(MapConstruction, MoveConstructor)
{
    Map<int, int> m1 = {{1, 10}, {2, 20}};
    Map<int, int> m2(std::move(m1));
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_TRUE(m1.Empty());
}

// ============================================================================
// Map — Insert
// ============================================================================

TEST(MapInsert, InsertCopy)
{
    Map<int, int> m;
    auto result = m.Insert({1, 10});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, 10);
}

TEST(MapInsert, InsertDuplicate)
{
    Map<int, int> m;
    m.Insert({1, 10});
    auto result = m.Insert({1, 20});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->second, 10);
}

TEST(MapInsert, InsertMultiple)
{
    Map<int, int> m;
    for (int i = 0; i < 100; ++i)
        m.Insert({i, i * 10});
    EXPECT_EQ(m.Size(), 100u);
}

TEST(MapInsert, InsertMove)
{
    Map<int, gx::String> m;
    auto result = m.Insert({1, "hello"});
    EXPECT_TRUE(result.second);
}

// ============================================================================
// Map — operator[]
// ============================================================================

TEST(MapOperatorBracket, InsertAndAccess)
{
    Map<int, int> m;
    m[1] = 10;
    EXPECT_EQ(m[1], 10);
}

TEST(MapOperatorBracket, DefaultConstructOnMiss)
{
    Map<int, int> m;
    int& val = m[42];
    EXPECT_EQ(val, 0);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(MapOperatorBracket, ModifyExisting)
{
    Map<int, int> m;
    m[1] = 10;
    m[1] = 20;
    EXPECT_EQ(m[1], 20);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(MapOperatorBracket, MoveKey)
{
    Map<gx::String, int> m;
    gx::String key = "hello";
    m[std::move(key)] = 42;
    EXPECT_TRUE(m.Contains("hello"));
}

// ============================================================================
// Map — Find / Contains
// ============================================================================

TEST(MapFind, FindExisting)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    auto it = m.Find(2);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, 20);
}

TEST(MapFind, FindNonExisting)
{
    Map<int, int> m = {{1, 10}};
    EXPECT_EQ(m.Find(99), m.end());
}

TEST(MapFind, ContainsTrue)
{
    Map<int, int> m = {{1, 10}};
    EXPECT_TRUE(m.Contains(1));
}

TEST(MapFind, ContainsFalse)
{
    Map<int, int> m = {{1, 10}};
    EXPECT_FALSE(m.Contains(99));
}

TEST(MapFind, FindConst)
{
    const Map<int, int> m = {{1, 10}, {2, 20}};
    auto it = m.Find(1);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, 10);
}

// ============================================================================
// Map — At
// ============================================================================

TEST(MapAt, AtExisting)
{
    Map<int, int> m = {{1, 10}, {2, 20}};
    EXPECT_EQ(m.At(1), 10);
}

TEST(MapAt, AtConst)
{
    const Map<int, int> m = {{1, 10}};
    EXPECT_EQ(m.At(1), 10);
}

// ============================================================================
// Map — Erase
// ============================================================================

TEST(MapErase, EraseByKey)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    size_t count = m.Erase(2);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(m.Size(), 2u);
    EXPECT_FALSE(m.Contains(2));
}

TEST(MapErase, EraseNonExisting)
{
    Map<int, int> m = {{1, 10}};
    size_t count = m.Erase(99);
    EXPECT_EQ(count, 0u);
}

TEST(MapErase, EraseByIterator)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    auto it = m.Find(2);
    m.Erase(it);
    EXPECT_EQ(m.Size(), 2u);
    EXPECT_FALSE(m.Contains(2));
}

TEST(MapErase, EraseAllElements)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    m.Erase(1);
    m.Erase(2);
    m.Erase(3);
    EXPECT_TRUE(m.Empty());
    EXPECT_TRUE(m.Validate());
}

TEST(MapErase, EraseRange)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}, {4, 40}};
    auto first = m.Find(2);
    auto last = m.Find(4);
    m.Erase(first, last);
    EXPECT_EQ(m.Size(), 2u);
    EXPECT_TRUE(m.Contains(1));
    EXPECT_TRUE(m.Contains(4));
}

// ============================================================================
// Map — LowerBound / UpperBound / EqualRange
// ============================================================================

TEST(MapBounds, LowerBound)
{
    Map<int, int> m = {{1, 10}, {3, 30}, {5, 50}};
    auto it = m.LowerBound(3);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->first, 3);
}

TEST(MapBounds, LowerBoundBetween)
{
    Map<int, int> m = {{1, 10}, {3, 30}, {5, 50}};
    auto it = m.LowerBound(2);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->first, 3);
}

TEST(MapBounds, LowerBoundBeyond)
{
    Map<int, int> m = {{1, 10}, {3, 30}};
    auto it = m.LowerBound(10);
    EXPECT_EQ(it, m.end());
}

TEST(MapBounds, UpperBound)
{
    Map<int, int> m = {{1, 10}, {3, 30}, {5, 50}};
    auto it = m.UpperBound(3);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->first, 5);
}

TEST(MapBounds, UpperBoundBeyond)
{
    Map<int, int> m = {{1, 10}, {3, 30}};
    auto it = m.UpperBound(3);
    EXPECT_EQ(it, m.end());
}

TEST(MapBounds, EqualRange)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    auto [first, last] = m.EqualRange(2);
    EXPECT_NE(first, m.end());
    EXPECT_EQ(first->first, 2);
    ++first;
    EXPECT_EQ(first, last);
}

TEST(MapBounds, EqualRangeNotFound)
{
    Map<int, int> m = {{1, 10}, {3, 30}};
    auto [first, last] = m.EqualRange(2);
    EXPECT_EQ(first, last);
}

// ============================================================================
// Map — In-order iteration
// ============================================================================

TEST(MapIteration, InOrder)
{
    Map<int, int> m = {{5, 50}, {2, 20}, {8, 80}, {1, 10}, {3, 30}};
    gx::Vector<int> keys;
    for (auto& [k, v] : m)
        keys.push_back(k);
    EXPECT_EQ(keys, (gx::Vector<int>{1, 2, 3, 5, 8}));
}

TEST(MapIteration, ConstIteration)
{
    const Map<int, int> m = {{3, 30}, {1, 10}, {2, 20}};
    gx::Vector<int> keys;
    for (auto& [k, v] : m)
        keys.push_back(k);
    EXPECT_EQ(keys, (gx::Vector<int>{1, 2, 3}));
}

TEST(MapIteration, IterateEmpty)
{
    Map<int, int> m;
    int count = 0;
    for ([[maybe_unused]] auto& p : m)
        ++count;
    EXPECT_EQ(count, 0);
}

TEST(MapIteration, ReverseIteration)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    gx::Vector<int> keys;
    for (auto it = m.rbegin(); it != m.rend(); ++it)
        keys.push_back(it->first);
    EXPECT_EQ(keys, (gx::Vector<int>{3, 2, 1}));
}

// ============================================================================
// Map — Copy / Move
// ============================================================================

TEST(MapCopyMove, CopyAssignment)
{
    Map<int, int> m1 = {{1, 10}, {2, 20}};
    Map<int, int> m2;
    m2 = m1;
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_EQ(m2.At(1), 10);
}

TEST(MapCopyMove, MoveAssignment)
{
    Map<int, int> m1 = {{1, 10}, {2, 20}};
    Map<int, int> m2;
    m2 = std::move(m1);
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_TRUE(m1.Empty());
}

TEST(MapCopyMove, SelfAssignment)
{
    Map<int, int> m = {{1, 10}};
    m = m;
    EXPECT_EQ(m.Size(), 1u);
}

// ============================================================================
// Map — InsertOrAssign / TryEmplace
// ============================================================================

TEST(MapInsertOrAssign, InsertNew)
{
    Map<int, int> m;
    auto [it, inserted] = m.InsertOrAssign(1, 10);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, 10);
}

TEST(MapInsertOrAssign, AssignExisting)
{
    Map<int, int> m = {{1, 10}};
    auto [it, inserted] = m.InsertOrAssign(1, 20);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, 20);
}

TEST(MapTryEmplace, EmplaceNew)
{
    Map<int, gx::String> m;
    auto [it, inserted] = m.TryEmplace(1, "hello");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "hello");
}

TEST(MapTryEmplace, NoEmplaceExisting)
{
    Map<int, gx::String> m;
    m.TryEmplace(1, "hello");
    auto [it, inserted] = m.TryEmplace(1, "world");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "hello");
}

TEST(MapTryEmplace, MoveKey)
{
    Map<gx::String, int> m;
    gx::String key = "test";
    auto [it, inserted] = m.TryEmplace(std::move(key), 42);
    EXPECT_TRUE(inserted);
}

// ============================================================================
// Map — Validate
// ============================================================================

TEST(MapValidate, ValidateEmpty)
{
    Map<int, int> m;
    EXPECT_TRUE(m.Validate());
}

TEST(MapValidate, ValidateAfterInserts)
{
    Map<int, int> m;
    for (int i = 0; i < 100; ++i)
        m.Insert({i, i * 10});
    EXPECT_TRUE(m.Validate());
}

TEST(MapValidate, ValidateAfterErases)
{
    Map<int, int> m;
    for (int i = 0; i < 50; ++i)
        m.Insert({i, i});
    for (int i = 0; i < 25; ++i)
        m.Erase(i);
    EXPECT_TRUE(m.Validate());
}

TEST(MapValidate, ValidateAfterClear)
{
    Map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    m.Clear();
    EXPECT_TRUE(m.Validate());
}

// ============================================================================
// Map — Comparison
// ============================================================================

TEST(MapComparison, EqualMaps)
{
    Map<int, int> a = {{1, 10}, {2, 20}};
    Map<int, int> b = {{1, 10}, {2, 20}};
    EXPECT_TRUE(a == b);
}

TEST(MapComparison, NotEqualMaps)
{
    Map<int, int> a = {{1, 10}};
    Map<int, int> b = {{1, 10}, {2, 20}};
    EXPECT_TRUE(a != b);
}

// ============================================================================
// Map — Count / Clear / Swap
// ============================================================================

TEST(MapMisc, CountExisting)
{
    Map<int, int> m = {{1, 10}, {2, 20}};
    EXPECT_EQ(m.Count(1), 1u);
    EXPECT_EQ(m.Count(99), 0u);
}

TEST(MapMisc, Clear)
{
    Map<int, int> m = {{1, 10}, {2, 20}};
    m.Clear();
    EXPECT_TRUE(m.Empty());
    EXPECT_TRUE(m.Validate());
}

TEST(MapMisc, Swap)
{
    Map<int, int> a = {{1, 10}};
    Map<int, int> b = {{2, 20}, {3, 30}};
    a.Swap(b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 1u);
    EXPECT_TRUE(a.Contains(2));
    EXPECT_TRUE(b.Contains(1));
    EXPECT_TRUE(a.Validate());
    EXPECT_TRUE(b.Validate());
}

// ============================================================================
// Set — Basic
// ============================================================================

TEST(SetBasic, DefaultConstruction)
{
    Set<int> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(SetBasic, InitializerList)
{
    Set<int> s = {5, 3, 1, 4, 2};
    EXPECT_EQ(s.Size(), 5u);
}

TEST(SetBasic, InsertNew)
{
    Set<int> s;
    auto result = s.Insert(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(SetBasic, InsertDuplicate)
{
    Set<int> s = {1, 2, 3};
    auto result = s.Insert(2);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(s.Size(), 3u);
}

TEST(SetBasic, FindExisting)
{
    Set<int> s = {10, 20, 30};
    auto it = s.Find(20);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 20);
}

TEST(SetBasic, FindNonExisting)
{
    Set<int> s = {10, 20, 30};
    EXPECT_EQ(s.Find(99), s.end());
}

TEST(SetBasic, Contains)
{
    Set<int> s = {1, 2, 3};
    EXPECT_TRUE(s.Contains(2));
    EXPECT_FALSE(s.Contains(99));
}

TEST(SetBasic, EraseByKey)
{
    Set<int> s = {1, 2, 3};
    size_t count = s.Erase(2);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(s.Size(), 2u);
    EXPECT_FALSE(s.Contains(2));
}

TEST(SetBasic, EraseByIterator)
{
    Set<int> s = {1, 2, 3};
    auto it = s.Find(2);
    s.Erase(it);
    EXPECT_EQ(s.Size(), 2u);
}

TEST(SetBasic, OrderedIteration)
{
    Set<int> s = {5, 3, 1, 4, 2};
    gx::Vector<int> values;
    for (int v : s)
        values.push_back(v);
    EXPECT_EQ(values, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(SetBasic, Count)
{
    Set<int> s = {1, 2, 3};
    EXPECT_EQ(s.Count(1), 1u);
    EXPECT_EQ(s.Count(99), 0u);
}

TEST(SetBasic, Clear)
{
    Set<int> s = {1, 2, 3};
    s.Clear();
    EXPECT_TRUE(s.Empty());
    EXPECT_TRUE(s.Validate());
}

TEST(SetBasic, CopyConstructor)
{
    Set<int> s1 = {1, 2, 3};
    Set<int> s2(s1);
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_TRUE(s2.Validate());
}

TEST(SetBasic, MoveConstructor)
{
    Set<int> s1 = {1, 2, 3};
    Set<int> s2(std::move(s1));
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_TRUE(s1.Empty());
}

TEST(SetBasic, CopyAssignment)
{
    Set<int> s1 = {1, 2, 3};
    Set<int> s2;
    s2 = s1;
    EXPECT_EQ(s2.Size(), 3u);
}

TEST(SetBasic, MoveAssignment)
{
    Set<int> s1 = {1, 2, 3};
    Set<int> s2;
    s2 = std::move(s1);
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_TRUE(s1.Empty());
}

TEST(SetBasic, Validate)
{
    Set<int> s;
    for (int i = 0; i < 100; ++i)
        s.Insert(i);
    EXPECT_TRUE(s.Validate());
}

TEST(SetBasic, ValidateAfterErases)
{
    Set<int> s;
    for (int i = 0; i < 50; ++i)
        s.Insert(i);
    for (int i = 0; i < 25; ++i)
        s.Erase(i);
    EXPECT_TRUE(s.Validate());
    EXPECT_EQ(s.Size(), 25u);
}

TEST(SetBasic, LowerBound)
{
    Set<int> s = {1, 3, 5, 7, 9};
    auto it = s.LowerBound(4);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 5);
}

TEST(SetBasic, UpperBound)
{
    Set<int> s = {1, 3, 5, 7, 9};
    auto it = s.UpperBound(5);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 7);
}

TEST(SetBasic, EqualRange)
{
    Set<int> s = {1, 2, 3, 4, 5};
    auto [first, last] = s.EqualRange(3);
    EXPECT_NE(first, s.end());
    EXPECT_EQ(*first, 3);
    ++first;
    EXPECT_EQ(first, last);
}

TEST(SetBasic, CountRange)
{
    Set<int> s = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    // CountRange counts [lower, upper)
    EXPECT_EQ(s.CountRange(3, 7), 5u); // 3,4,5,6,7
}

TEST(SetBasic, InsertMany)
{
    Set<int> s;
    for (int i = 99; i >= 0; --i)
        s.Insert(i);
    EXPECT_EQ(s.Size(), 100u);
    EXPECT_TRUE(s.Validate());
}

TEST(SetBasic, EqualSets)
{
    Set<int> a = {1, 2, 3};
    Set<int> b = {3, 1, 2};
    EXPECT_TRUE(a == b);
}

TEST(SetBasic, NotEqualSets)
{
    Set<int> a = {1, 2, 3};
    Set<int> b = {1, 2, 4};
    EXPECT_TRUE(a != b);
}

TEST(SetBasic, ReverseIteration)
{
    Set<int> s = {1, 2, 3, 4, 5};
    gx::Vector<int> values;
    for (auto it = s.rbegin(); it != s.rend(); ++it)
        values.push_back(*it);
    EXPECT_EQ(values, (gx::Vector<int>{5, 4, 3, 2, 1}));
}

TEST(SetBasic, Swap)
{
    Set<int> a = {1, 2};
    Set<int> b = {3, 4, 5};
    a.Swap(b);
    EXPECT_EQ(a.Size(), 3u);
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_TRUE(a.Contains(3));
    EXPECT_TRUE(b.Contains(1));
    EXPECT_TRUE(a.Validate());
    EXPECT_TRUE(b.Validate());
}

// ============================================================================
// MultiMap
// ============================================================================

TEST(MultiMapBasic, DefaultConstruction)
{
    MultiMap<int, int> m;
    EXPECT_TRUE(m.Empty());
}

TEST(MultiMapBasic, InsertDuplicateKeys)
{
    MultiMap<int, gx::String> m;
    m.Insert({1, "a"});
    m.Insert({1, "b"});
    m.Insert({1, "c"});
    EXPECT_EQ(m.Size(), 3u);
}

TEST(MultiMapBasic, Count)
{
    MultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    m.Insert({1, 30});
    m.Insert({2, 40});
    EXPECT_EQ(m.Count(1), 3u);
    EXPECT_EQ(m.Count(2), 1u);
    EXPECT_EQ(m.Count(99), 0u);
}

TEST(MultiMapBasic, EqualRange)
{
    MultiMap<int, int> m;
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

TEST(MultiMapBasic, EqualRangeNotFound)
{
    MultiMap<int, int> m = {{1, 10}, {2, 20}};
    auto [first, last] = m.EqualRange(99);
    EXPECT_EQ(first, last);
}

TEST(MultiMapBasic, InOrderIteration)
{
    MultiMap<int, int> m;
    m.Insert({3, 30});
    m.Insert({1, 10});
    m.Insert({2, 20});
    m.Insert({1, 11});

    gx::Vector<int> keys;
    for (auto& [k, v] : m)
        keys.push_back(k);
    // Should be sorted
    EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
}

TEST(MultiMapBasic, EraseByKey)
{
    MultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    m.Insert({2, 30});
    size_t count = m.Erase(1);
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(m.Size(), 1u);
}

TEST(MultiMapBasic, FindReturnsFirst)
{
    MultiMap<int, int> m;
    m.Insert({1, 10});
    m.Insert({1, 20});
    auto it = m.Find(1);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, 10); // First inserted
}

TEST(MultiMapBasic, InitializerList)
{
    MultiMap<int, int> m = {{1, 10}, {1, 20}, {2, 30}};
    EXPECT_EQ(m.Size(), 3u);
}

TEST(MultiMapBasic, CopyConstructor)
{
    MultiMap<int, int> m1 = {{1, 10}, {1, 20}};
    MultiMap<int, int> m2(m1);
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_TRUE(m2.Validate());
}

TEST(MultiMapBasic, MoveConstructor)
{
    MultiMap<int, int> m1 = {{1, 10}, {1, 20}};
    MultiMap<int, int> m2(std::move(m1));
    EXPECT_EQ(m2.Size(), 2u);
    EXPECT_TRUE(m1.Empty());
}

TEST(MultiMapBasic, Clear)
{
    MultiMap<int, int> m = {{1, 10}, {1, 20}};
    m.Clear();
    EXPECT_TRUE(m.Empty());
    EXPECT_TRUE(m.Validate());
}

TEST(MultiMapBasic, Validate)
{
    MultiMap<int, int> m;
    for (int i = 0; i < 50; ++i)
        m.Insert({i % 10, i});
    EXPECT_TRUE(m.Validate());
}

TEST(MultiMapBasic, Contains)
{
    MultiMap<int, int> m = {{1, 10}, {1, 20}};
    EXPECT_TRUE(m.Contains(1));
    EXPECT_FALSE(m.Contains(99));
}

// ============================================================================
// MultiSet
// ============================================================================

TEST(MultiSetBasic, DefaultConstruction)
{
    MultiSet<int> s;
    EXPECT_TRUE(s.Empty());
}

TEST(MultiSetBasic, InsertDuplicates)
{
    MultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(1);
    EXPECT_EQ(s.Size(), 3u);
}

TEST(MultiSetBasic, Count)
{
    MultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    EXPECT_EQ(s.Count(1), 2u);
    EXPECT_EQ(s.Count(2), 1u);
    EXPECT_EQ(s.Count(99), 0u);
}

TEST(MultiSetBasic, EqualRange)
{
    MultiSet<int> s;
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

TEST(MultiSetBasic, EqualRangeNotFound)
{
    MultiSet<int> s = {1, 2, 3};
    auto [first, last] = s.EqualRange(99);
    EXPECT_EQ(first, last);
}

TEST(MultiSetBasic, OrderedIteration)
{
    MultiSet<int> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(2);
    s.Insert(1);
    s.Insert(3);

    gx::Vector<int> values;
    for (int v : s)
        values.push_back(v);
    EXPECT_EQ(values, (gx::Vector<int>{1, 1, 2, 3, 3}));
}

TEST(MultiSetBasic, EraseByKey)
{
    MultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    size_t count = s.Erase(1);
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(MultiSetBasic, InitializerList)
{
    MultiSet<int> s = {1, 1, 2, 3, 3};
    EXPECT_EQ(s.Size(), 5u);
}

TEST(MultiSetBasic, CopyConstructor)
{
    MultiSet<int> s1 = {1, 1, 2};
    MultiSet<int> s2(s1);
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_TRUE(s2.Validate());
}

TEST(MultiSetBasic, MoveConstructor)
{
    MultiSet<int> s1 = {1, 1, 2};
    MultiSet<int> s2(std::move(s1));
    EXPECT_EQ(s2.Size(), 3u);
    EXPECT_TRUE(s1.Empty());
}

TEST(MultiSetBasic, Clear)
{
    MultiSet<int> s = {1, 1, 2};
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(MultiSetBasic, Contains)
{
    MultiSet<int> s = {1, 2, 3};
    EXPECT_TRUE(s.Contains(1));
    EXPECT_FALSE(s.Contains(99));
}

TEST(MultiSetBasic, Validate)
{
    MultiSet<int> s;
    for (int i = 0; i < 50; ++i)
        s.Insert(i % 10);
    EXPECT_TRUE(s.Validate());
}

TEST(MultiSetBasic, CountRange)
{
    MultiSet<int> s;
    for (int i = 1; i <= 5; ++i)
        for (int j = 0; j < i; ++j)
            s.Insert(i);
    // s = {1, 2,2, 3,3,3, 4,4,4,4, 5,5,5,5,5}
    // CountRange(2, 4) = elements in [2, upper_bound(4))
    EXPECT_EQ(s.CountRange(2, 4), 9u); // 2,2, 3,3,3, 4,4,4,4
}

TEST(MultiSetBasic, ManyDuplicates)
{
    MultiSet<int> s;
    for (int i = 0; i < 100; ++i)
        s.Insert(42);
    EXPECT_EQ(s.Size(), 100u);
    EXPECT_EQ(s.Count(42), 100u);
    EXPECT_TRUE(s.Validate());
}

TEST(MultiSetBasic, LowerBound)
{
    MultiSet<int> s = {1, 1, 3, 3, 5};
    auto it = s.LowerBound(2);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 3);
}

TEST(MultiSetBasic, UpperBound)
{
    MultiSet<int> s = {1, 1, 3, 3, 5};
    auto it = s.UpperBound(3);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 5);
}

TEST(MultiSetBasic, Swap)
{
    MultiSet<int> a = {1, 1, 2};
    MultiSet<int> b = {3, 4};
    a.Swap(b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_TRUE(a.Validate());
    EXPECT_TRUE(b.Validate());
}

TEST(MultiSetBasic, ReverseIteration)
{
    MultiSet<int> s = {1, 2, 2, 3};
    gx::Vector<int> values;
    for (auto it = s.rbegin(); it != s.rend(); ++it)
        values.push_back(*it);
    EXPECT_EQ(values, (gx::Vector<int>{3, 2, 2, 1}));
}
