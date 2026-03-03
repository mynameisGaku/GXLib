/// @file test_ContainerFlat.cpp
/// @brief VectorMap, VectorSet, VectorMultiMap, VectorMultiSet tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/VectorMap.h"
#include "Container/VectorSet.h"
#include "Container/VectorMultiMap.h"
#include "Container/VectorMultiSet.h"

#include <vector>
#include <string>

using namespace gx::container;

// ============================================================================
// VectorMap Tests
// ============================================================================

TEST(VectorMap, DefaultConstruction)
{
    VectorMap<int, int> m;
    EXPECT_TRUE(m.Empty());
    EXPECT_EQ(m.Size(), 0u);
}

TEST(VectorMap, InsertAndFind)
{
    VectorMap<int, int> m;
    auto [it, ok] = m.Insert(1, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, 10);
    auto found = m.Find(1);
    ASSERT_NE(found, m.end());
    EXPECT_EQ(found->second, 10);
}

TEST(VectorMap, InsertDuplicate)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    auto [it, ok] = m.Insert(1, 20);
    EXPECT_FALSE(ok);
    EXPECT_EQ(it->second, 10);
}

TEST(VectorMap, InsertMove)
{
    VectorMap<int, gx::String> m;
    gx::String val = "hello";
    auto [it, ok] = m.Insert(1, std::move(val));
    EXPECT_TRUE(ok);
    EXPECT_EQ(it->second, "hello");
}

TEST(VectorMap, OperatorIndex)
{
    VectorMap<int, int> m;
    m[1] = 10;
    m[2] = 20;
    m[3] = 30;
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);
    EXPECT_EQ(m[3], 30);
    m[2] = 25;
    EXPECT_EQ(m[2], 25);
}

TEST(VectorMap, OperatorIndexCreatesDefault)
{
    VectorMap<int, int> m;
    int& val = m[42];
    val = 100;
    EXPECT_EQ(m.Size(), 1u);
    EXPECT_EQ(m[42], 100);
}

TEST(VectorMap, FindMissing)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    EXPECT_EQ(m.Find(999), m.end());
}

TEST(VectorMap, Contains)
{
    VectorMap<int, int> m;
    m.Insert(5, 50);
    EXPECT_TRUE(m.Contains(5));
    EXPECT_FALSE(m.Contains(6));
}

TEST(VectorMap, EraseByKey)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(3, 30);
    EXPECT_TRUE(m.Erase(2));
    EXPECT_FALSE(m.Contains(2));
    EXPECT_EQ(m.Size(), 2u);
}

TEST(VectorMap, EraseByKeyNotFound)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    EXPECT_FALSE(m.Erase(999));
}

TEST(VectorMap, EraseByIterator)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(3, 30);
    auto it = m.Find(2);
    auto next = m.Erase(it);
    EXPECT_EQ(m.Size(), 2u);
    EXPECT_EQ(next->first, 3);
}

TEST(VectorMap, SortedOrder)
{
    VectorMap<int, int> m;
    m.Insert(3, 30);
    m.Insert(1, 10);
    m.Insert(5, 50);
    m.Insert(2, 20);
    m.Insert(4, 40);
    auto it = m.begin();
    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_EQ(it->first, i);
        EXPECT_EQ(it->second, i * 10);
        ++it;
    }
}

TEST(VectorMap, LowerBound)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(3, 30);
    m.Insert(5, 50);
    auto lb = m.LowerBound(3);
    EXPECT_EQ(lb->first, 3);
    auto lb2 = m.LowerBound(2);
    EXPECT_EQ(lb2->first, 3);
}

TEST(VectorMap, UpperBound)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(3, 30);
    m.Insert(5, 50);
    auto ub = m.UpperBound(3);
    EXPECT_EQ(ub->first, 5);
    auto ub2 = m.UpperBound(5);
    EXPECT_EQ(ub2, m.end());
}

TEST(VectorMap, EqualRange)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(3, 30);
    m.Insert(5, 50);
    auto [first, last] = m.EqualRange(3);
    EXPECT_EQ(first->first, 3);
    EXPECT_EQ(last->first, 5);
}

TEST(VectorMap, Clear)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Clear();
    EXPECT_TRUE(m.Empty());
    EXPECT_EQ(m.Size(), 0u);
}

TEST(VectorMap, Reserve)
{
    VectorMap<int, int> m;
    m.Reserve(100);
    EXPECT_GE(m.Capacity(), 100u);
}

TEST(VectorMap, CopyConstruction)
{
    VectorMap<int, int> a;
    a.Insert(1, 10);
    a.Insert(2, 20);
    VectorMap<int, int> b(a);
    EXPECT_EQ(a, b);
}

TEST(VectorMap, MoveConstruction)
{
    VectorMap<int, int> a;
    a.Insert(1, 10);
    a.Insert(2, 20);
    VectorMap<int, int> b(std::move(a));
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_TRUE(a.Empty());
}

TEST(VectorMap, CopyAssignment)
{
    VectorMap<int, int> a;
    a.Insert(1, 10);
    VectorMap<int, int> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(VectorMap, MoveAssignment)
{
    VectorMap<int, int> a;
    a.Insert(1, 10);
    VectorMap<int, int> b;
    b = std::move(a);
    EXPECT_EQ(b.Size(), 1u);
    EXPECT_TRUE(a.Empty());
}

TEST(VectorMap, Validate)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(3, 30);
    EXPECT_TRUE(m.Validate());
}

TEST(VectorMap, Equality)
{
    VectorMap<int, int> a;
    a.Insert(1, 10);
    a.Insert(2, 20);
    VectorMap<int, int> b;
    b.Insert(1, 10);
    b.Insert(2, 20);
    EXPECT_EQ(a, b);
    b[2] = 25;
    EXPECT_NE(a, b);
}

TEST(VectorMap, Swap)
{
    VectorMap<int, int> a;
    a.Insert(1, 10);
    VectorMap<int, int> b;
    b.Insert(2, 20);
    b.Insert(3, 30);
    a.Swap(b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 1u);
}

TEST(VectorMap, Emplace)
{
    VectorMap<int, int> m;
    auto [it, ok] = m.Emplace(1, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(it->second, 10);
    auto [it2, ok2] = m.Emplace(1, 20);
    EXPECT_FALSE(ok2);
}

TEST(VectorMap, ConstructFromInitializerList)
{
    VectorMap<int, int> m = {{3, 30}, {1, 10}, {2, 20}};
    EXPECT_EQ(m.Size(), 3u);
    EXPECT_TRUE(m.Validate());
    auto it = m.begin();
    EXPECT_EQ(it->first, 1);
}

TEST(VectorMap, ManyElements)
{
    VectorMap<int, int> m;
    for (int i = 100; i > 0; --i)
        m.Insert(i, i * 10);
    EXPECT_EQ(m.Size(), 100u);
    EXPECT_TRUE(m.Validate());
    for (int i = 1; i <= 100; ++i)
    {
        auto it = m.Find(i);
        ASSERT_NE(it, m.end());
        EXPECT_EQ(it->second, i * 10);
    }
}

TEST(VectorMap, ConstFind)
{
    VectorMap<int, int> m;
    m.Insert(1, 10);
    const auto& cm = m;
    auto found = cm.Find(1);
    ASSERT_NE(found, cm.end());
    EXPECT_EQ(found->second, 10);
}

// ============================================================================
// VectorSet Tests
// ============================================================================

TEST(VectorSet, DefaultConstruction)
{
    VectorSet<int> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(VectorSet, Insert)
{
    VectorSet<int> s;
    auto [it, ok] = s.Insert(5);
    EXPECT_TRUE(ok);
    EXPECT_EQ(*it, 5);
}

TEST(VectorSet, InsertDuplicate)
{
    VectorSet<int> s;
    s.Insert(5);
    auto [it, ok] = s.Insert(5);
    EXPECT_FALSE(ok);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(VectorSet, InsertMove)
{
    VectorSet<int> s;
    int val = 42;
    auto [it, ok] = s.Insert(std::move(val));
    EXPECT_TRUE(ok);
    EXPECT_EQ(*it, 42);
}

TEST(VectorSet, Find)
{
    VectorSet<int> s;
    s.Insert(1);
    s.Insert(3);
    s.Insert(5);
    EXPECT_NE(s.Find(3), s.end());
    EXPECT_EQ(s.Find(2), s.end());
}

TEST(VectorSet, Contains)
{
    VectorSet<int> s;
    s.Insert(42);
    EXPECT_TRUE(s.Contains(42));
    EXPECT_FALSE(s.Contains(43));
}

TEST(VectorSet, EraseByKey)
{
    VectorSet<int> s;
    s.Insert(1);
    s.Insert(2);
    s.Insert(3);
    EXPECT_TRUE(s.Erase(2));
    EXPECT_FALSE(s.Contains(2));
    EXPECT_EQ(s.Size(), 2u);
}

TEST(VectorSet, EraseByIterator)
{
    VectorSet<int> s;
    s.Insert(1);
    s.Insert(2);
    s.Insert(3);
    auto it = s.Find(2);
    auto next = s.Erase(it);
    EXPECT_EQ(*next, 3);
    EXPECT_EQ(s.Size(), 2u);
}

TEST(VectorSet, SortedIteration)
{
    VectorSet<int> s;
    s.Insert(5);
    s.Insert(1);
    s.Insert(3);
    s.Insert(2);
    s.Insert(4);
    gx::Vector<int> result(s.begin(), s.end());
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(VectorSet, LowerBound)
{
    VectorSet<int> s;
    s.Insert(1);
    s.Insert(3);
    s.Insert(5);
    EXPECT_EQ(*s.LowerBound(3), 3);
    EXPECT_EQ(*s.LowerBound(2), 3);
}

TEST(VectorSet, UpperBound)
{
    VectorSet<int> s;
    s.Insert(1);
    s.Insert(3);
    s.Insert(5);
    EXPECT_EQ(*s.UpperBound(3), 5);
    EXPECT_EQ(s.UpperBound(5), s.end());
}

TEST(VectorSet, Clear)
{
    VectorSet<int> s;
    s.Insert(1);
    s.Insert(2);
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(VectorSet, CopyConstruction)
{
    VectorSet<int> a;
    a.Insert(1);
    a.Insert(2);
    a.Insert(3);
    VectorSet<int> b(a);
    EXPECT_EQ(a, b);
}

TEST(VectorSet, MoveConstruction)
{
    VectorSet<int> a;
    a.Insert(1);
    a.Insert(2);
    VectorSet<int> b(std::move(a));
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_TRUE(a.Empty());
}

TEST(VectorSet, Equality)
{
    VectorSet<int> a;
    a.Insert(1);
    a.Insert(2);
    VectorSet<int> b;
    b.Insert(1);
    b.Insert(2);
    EXPECT_EQ(a, b);
}

TEST(VectorSet, Validate)
{
    VectorSet<int> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(2);
    EXPECT_TRUE(s.Validate());
}

TEST(VectorSet, Swap)
{
    VectorSet<int> a;
    a.Insert(1);
    VectorSet<int> b;
    b.Insert(2);
    b.Insert(3);
    a.Swap(b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 1u);
}

TEST(VectorSet, Reserve)
{
    VectorSet<int> s;
    s.Reserve(50);
    EXPECT_GE(s.Capacity(), 50u);
}

TEST(VectorSet, ConstructFromInitializerList)
{
    VectorSet<int> s = {5, 3, 1, 4, 2};
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_TRUE(s.Validate());
}

TEST(VectorSet, ManyElements)
{
    VectorSet<int> s;
    for (int i = 100; i > 0; --i)
        s.Insert(i);
    EXPECT_EQ(s.Size(), 100u);
    EXPECT_TRUE(s.Validate());
}

// ============================================================================
// VectorMultiMap Tests
// ============================================================================

TEST(VectorMultiMap, DefaultConstruction)
{
    VectorMultiMap<int, int> m;
    EXPECT_TRUE(m.Empty());
    EXPECT_EQ(m.Size(), 0u);
}

TEST(VectorMultiMap, InsertDuplicateKeys)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(1, 20);
    m.Insert(1, 30);
    EXPECT_EQ(m.Size(), 3u);
    EXPECT_EQ(m.Count(1), 3u);
}

TEST(VectorMultiMap, FindFirstOccurrence)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(1, 30);
    auto it = m.Find(1);
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->first, 1);
}

TEST(VectorMultiMap, FindMissing)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    EXPECT_EQ(m.Find(999), m.end());
}

TEST(VectorMultiMap, Contains)
{
    VectorMultiMap<int, int> m;
    m.Insert(5, 50);
    EXPECT_TRUE(m.Contains(5));
    EXPECT_FALSE(m.Contains(6));
}

TEST(VectorMultiMap, Count)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(1, 30);
    m.Insert(1, 40);
    m.Insert(3, 50);
    EXPECT_EQ(m.Count(1), 3u);
    EXPECT_EQ(m.Count(2), 1u);
    EXPECT_EQ(m.Count(3), 1u);
    EXPECT_EQ(m.Count(99), 0u);
}

TEST(VectorMultiMap, EqualRange)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(1, 30);
    auto [first, last] = m.EqualRange(1);
    size_t count = 0;
    for (auto it = first; it != last; ++it)
    {
        EXPECT_EQ(it->first, 1);
        ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST(VectorMultiMap, EraseByKey)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(1, 20);
    m.Insert(2, 30);
    auto removed = m.Erase(1);
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(m.Size(), 1u);
    EXPECT_FALSE(m.Contains(1));
}

TEST(VectorMultiMap, EraseNotFound)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    auto removed = m.Erase(999);
    EXPECT_EQ(removed, 0u);
}

TEST(VectorMultiMap, SortedOrder)
{
    VectorMultiMap<int, int> m;
    m.Insert(3, 30);
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(1, 15);
    auto it = m.begin();
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 3);
}

TEST(VectorMultiMap, LowerBound)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(3, 30);
    m.Insert(3, 35);
    auto lb = m.LowerBound(3);
    EXPECT_EQ(lb->first, 3);
}

TEST(VectorMultiMap, UpperBound)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(3, 30);
    m.Insert(3, 35);
    m.Insert(5, 50);
    auto ub = m.UpperBound(3);
    EXPECT_EQ(ub->first, 5);
}

TEST(VectorMultiMap, Clear)
{
    VectorMultiMap<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Clear();
    EXPECT_TRUE(m.Empty());
}

TEST(VectorMultiMap, CopyConstruction)
{
    VectorMultiMap<int, int> a;
    a.Insert(1, 10);
    a.Insert(1, 20);
    VectorMultiMap<int, int> b(a);
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_EQ(b.Count(1), 2u);
}

TEST(VectorMultiMap, MoveConstruction)
{
    VectorMultiMap<int, int> a;
    a.Insert(1, 10);
    VectorMultiMap<int, int> b(std::move(a));
    EXPECT_EQ(b.Size(), 1u);
    EXPECT_TRUE(a.Empty());
}

TEST(VectorMultiMap, Validate)
{
    VectorMultiMap<int, int> m;
    m.Insert(3, 30);
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(1, 15);
    EXPECT_TRUE(m.Validate());
}

TEST(VectorMultiMap, Reserve)
{
    VectorMultiMap<int, int> m;
    m.Reserve(100);
    EXPECT_GE(m.Capacity(), 100u);
}

TEST(VectorMultiMap, ManyDuplicates)
{
    VectorMultiMap<int, int> m;
    for (int i = 0; i < 50; ++i)
        m.Insert(1, i);
    EXPECT_EQ(m.Size(), 50u);
    EXPECT_EQ(m.Count(1), 50u);
    EXPECT_TRUE(m.Validate());
}

// ============================================================================
// VectorMultiSet Tests
// ============================================================================

TEST(VectorMultiSet, DefaultConstruction)
{
    VectorMultiSet<int> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(VectorMultiSet, InsertDuplicates)
{
    VectorMultiSet<int> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(3);
    s.Insert(2);
    s.Insert(1);
    EXPECT_EQ(s.Size(), 5u);
}

TEST(VectorMultiSet, Count)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    s.Insert(2);
    EXPECT_EQ(s.Count(1), 3u);
    EXPECT_EQ(s.Count(2), 2u);
    EXPECT_EQ(s.Count(3), 0u);
}

TEST(VectorMultiSet, Find)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    s.Insert(2);
    EXPECT_NE(s.Find(1), s.end());
    EXPECT_EQ(s.Find(99), s.end());
}

TEST(VectorMultiSet, Contains)
{
    VectorMultiSet<int> s;
    s.Insert(42);
    EXPECT_TRUE(s.Contains(42));
    EXPECT_FALSE(s.Contains(43));
}

TEST(VectorMultiSet, SortedOrder)
{
    VectorMultiSet<int> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(2);
    s.Insert(1);
    gx::Vector<int> result(s.begin(), s.end());
    EXPECT_EQ(result, (gx::Vector<int>{1, 1, 2, 3}));
}

TEST(VectorMultiSet, Erase)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    s.Insert(3);
    auto removed = s.Erase(1);
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(s.Size(), 2u);
    EXPECT_FALSE(s.Contains(1));
}

TEST(VectorMultiSet, EraseNotFound)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    auto removed = s.Erase(999);
    EXPECT_EQ(removed, 0u);
}

TEST(VectorMultiSet, LowerBound)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    s.Insert(3);
    s.Insert(3);
    s.Insert(5);
    EXPECT_EQ(*s.LowerBound(3), 3);
    EXPECT_EQ(*s.LowerBound(2), 3);
}

TEST(VectorMultiSet, UpperBound)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    s.Insert(3);
    s.Insert(3);
    s.Insert(5);
    EXPECT_EQ(*s.UpperBound(3), 5);
    EXPECT_EQ(s.UpperBound(5), s.end());
}

TEST(VectorMultiSet, Clear)
{
    VectorMultiSet<int> s;
    s.Insert(1);
    s.Insert(2);
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(VectorMultiSet, CopyConstruction)
{
    VectorMultiSet<int> a;
    a.Insert(1);
    a.Insert(1);
    a.Insert(2);
    VectorMultiSet<int> b(a);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_EQ(b.Count(1), 2u);
}

TEST(VectorMultiSet, MoveConstruction)
{
    VectorMultiSet<int> a;
    a.Insert(1);
    a.Insert(2);
    VectorMultiSet<int> b(std::move(a));
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_TRUE(a.Empty());
}

TEST(VectorMultiSet, Validate)
{
    VectorMultiSet<int> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(2);
    s.Insert(1);
    EXPECT_TRUE(s.Validate());
}

TEST(VectorMultiSet, Reserve)
{
    VectorMultiSet<int> s;
    s.Reserve(50);
    EXPECT_GE(s.Capacity(), 50u);
}

TEST(VectorMultiSet, ManyDuplicates)
{
    VectorMultiSet<int> s;
    for (int i = 0; i < 100; ++i)
        s.Insert(42);
    EXPECT_EQ(s.Size(), 100u);
    EXPECT_EQ(s.Count(42), 100u);
}
