/// @file test_ContainerFixed.cpp
/// @brief FixedVector, FixedString, FixedList, FixedHashMap, FixedSet,
///        FixedMap, FixedMultiSet, FixedHashSet tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/FixedVector.h"
#include "Container/FixedString.h"
#include "Container/FixedList.h"
#include "Container/FixedHashMap.h"
#include "Container/FixedSet.h"
#include "Container/FixedMap.h"
#include "Container/FixedMultiSet.h"
#include "Container/FixedHashSet.h"

#include <vector>
#include <string>
#include <algorithm>

using namespace gx::container;

// ============================================================================
// FixedVector Tests
// ============================================================================

TEST(FixedVector, DefaultConstruction)
{
    FixedVector<int, 10> v;
    EXPECT_TRUE(v.Empty());
    EXPECT_EQ(v.Size(), 0u);
    EXPECT_EQ(v.Capacity(), 10u);
    EXPECT_FALSE(v.IsUsingHeap());
}

TEST(FixedVector, ConstructWithCount)
{
    FixedVector<int, 10> v(5);
    EXPECT_EQ(v.Size(), 5u);
    for (size_t i = 0; i < v.Size(); ++i)
        EXPECT_EQ(v[i], 0);
}

TEST(FixedVector, ConstructWithCountAndValue)
{
    FixedVector<int, 10> v(3, 42);
    EXPECT_EQ(v.Size(), 3u);
    for (size_t i = 0; i < v.Size(); ++i)
        EXPECT_EQ(v[i], 42);
}

TEST(FixedVector, ConstructFromInitializerList)
{
    FixedVector<int, 10> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(v.Size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[4], 5);
}

TEST(FixedVector, PushBack)
{
    FixedVector<int, 10> v;
    for (int i = 0; i < 10; ++i)
        v.PushBack(i);
    EXPECT_EQ(v.Size(), 10u);
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(v[i], i);
}

TEST(FixedVector, PushBackMove)
{
    FixedVector<gx::String, 5> v;
    gx::String s = "test";
    v.PushBack(std::move(s));
    EXPECT_EQ(v[0], "test");
}

TEST(FixedVector, PopBack)
{
    FixedVector<int, 10> v = {1, 2, 3};
    v.PopBack();
    EXPECT_EQ(v.Size(), 2u);
    EXPECT_EQ(v.Back(), 2);
}

TEST(FixedVector, EmplaceBack)
{
    FixedVector<std::pair<int, int>, 5> v;
    auto& ref = v.EmplaceBack(1, 2);
    EXPECT_EQ(ref.first, 1);
    EXPECT_EQ(ref.second, 2);
}

TEST(FixedVector, OperatorIndex)
{
    FixedVector<int, 10> v = {10, 20, 30};
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
    v[1] = 25;
    EXPECT_EQ(v[1], 25);
}

TEST(FixedVector, FrontAndBack)
{
    FixedVector<int, 10> v = {1, 2, 3};
    EXPECT_EQ(v.Front(), 1);
    EXPECT_EQ(v.Back(), 3);
}

TEST(FixedVector, Data)
{
    FixedVector<int, 10> v = {1, 2, 3};
    int* data = v.Data();
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[2], 3);
}

TEST(FixedVector, Iteration)
{
    FixedVector<int, 10> v = {1, 2, 3, 4, 5};
    gx::Vector<int> result;
    for (auto& val : v)
        result.push_back(val);
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(FixedVector, InsertAtBeginning)
{
    FixedVector<int, 10> v = {2, 3, 4};
    v.Insert(v.begin(), 1);
    EXPECT_EQ(v.Size(), 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(FixedVector, InsertInMiddle)
{
    FixedVector<int, 10> v = {1, 3, 4};
    v.Insert(v.begin() + 1, 2);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(FixedVector, InsertAtEnd)
{
    FixedVector<int, 10> v = {1, 2};
    v.Insert(v.end(), 3);
    EXPECT_EQ(v.Back(), 3);
}

TEST(FixedVector, EraseSingle)
{
    FixedVector<int, 10> v = {1, 2, 3, 4};
    v.Erase(v.begin() + 1);
    EXPECT_EQ(v.Size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
}

TEST(FixedVector, EraseRange)
{
    FixedVector<int, 10> v = {1, 2, 3, 4, 5};
    v.Erase(v.begin() + 1, v.begin() + 3);
    EXPECT_EQ(v.Size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[2], 5);
}

TEST(FixedVector, Clear)
{
    FixedVector<int, 10> v = {1, 2, 3};
    v.Clear();
    EXPECT_TRUE(v.Empty());
    EXPECT_EQ(v.Size(), 0u);
}

TEST(FixedVector, Resize)
{
    FixedVector<int, 10> v;
    v.Resize(5);
    EXPECT_EQ(v.Size(), 5u);
    v.Resize(3);
    EXPECT_EQ(v.Size(), 3u);
}

TEST(FixedVector, ResizeWithValue)
{
    FixedVector<int, 10> v;
    v.Resize(5, 42);
    EXPECT_EQ(v.Size(), 5u);
    for (size_t i = 0; i < v.Size(); ++i)
        EXPECT_EQ(v[i], 42);
}

TEST(FixedVector, CopyConstruction)
{
    FixedVector<int, 10> a = {1, 2, 3};
    FixedVector<int, 10> b(a);
    EXPECT_EQ(a, b);
}

TEST(FixedVector, MoveConstruction)
{
    FixedVector<int, 10> a = {1, 2, 3};
    FixedVector<int, 10> b(std::move(a));
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_EQ(a.Size(), 0u);
}

TEST(FixedVector, CopyAssignment)
{
    FixedVector<int, 10> a = {1, 2, 3};
    FixedVector<int, 10> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(FixedVector, MoveAssignment)
{
    FixedVector<int, 10> a = {1, 2, 3};
    FixedVector<int, 10> b;
    b = std::move(a);
    EXPECT_EQ(b.Size(), 3u);
}

TEST(FixedVector, Equality)
{
    FixedVector<int, 10> a = {1, 2, 3};
    FixedVector<int, 10> b = {1, 2, 3};
    FixedVector<int, 10> c = {1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(FixedVector, Validate)
{
    FixedVector<int, 10> v = {1, 2, 3};
    EXPECT_TRUE(v.Validate());
}

TEST(FixedVector, OverflowAllowed)
{
    FixedVector<int, 2, true> v;
    v.PushBack(1);
    v.PushBack(2);
    v.PushBack(3); // overflows to heap
    EXPECT_EQ(v.Size(), 3u);
    EXPECT_TRUE(v.IsUsingHeap());
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[2], 3);
}

TEST(FixedVector, ShrinkToFit)
{
    FixedVector<int, 4, true> v;
    for (int i = 0; i < 10; ++i)
        v.PushBack(i);
    EXPECT_TRUE(v.IsUsingHeap());

    v.Clear();
    v.PushBack(1);
    v.PushBack(2);
    v.ShrinkToFit();
    EXPECT_FALSE(v.IsUsingHeap());
}

TEST(FixedVector, Swap)
{
    FixedVector<int, 10> a = {1, 2, 3};
    FixedVector<int, 10> b = {4, 5};
    a.Swap(b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 3u);
}

TEST(FixedVector, ConstAccess)
{
    const FixedVector<int, 10> v = {10, 20, 30};
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v.Front(), 10);
    EXPECT_EQ(v.Back(), 30);
    EXPECT_EQ(v.Data()[1], 20);
}

// ============================================================================
// FixedString Tests
// ============================================================================

TEST(FixedString, DefaultConstruction)
{
    FixedString<32> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
    EXPECT_STREQ(s.CStr(), "");
}

TEST(FixedString, ConstructFromCStr)
{
    FixedString<32> s("hello");
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.CStr(), "hello");
}

TEST(FixedString, ConstructFromCStrAndCount)
{
    FixedString<32> s("hello world", 5);
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.CStr(), "hello");
}

TEST(FixedString, ConstructFromCountAndChar)
{
    FixedString<32> s(5, 'a');
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.CStr(), "aaaaa");
}

TEST(FixedString, ConstructFromStringView)
{
    std::string_view sv = "test";
    FixedString<32> s(sv);
    EXPECT_EQ(s.Size(), 4u);
    EXPECT_STREQ(s.CStr(), "test");
}

TEST(FixedString, CopyConstruction)
{
    FixedString<32> a("hello");
    FixedString<32> b(a);
    EXPECT_EQ(a, b);
}

TEST(FixedString, MoveConstruction)
{
    FixedString<32> a("hello");
    FixedString<32> b(std::move(a));
    EXPECT_STREQ(b.CStr(), "hello");
    EXPECT_TRUE(a.Empty());
}

TEST(FixedString, CopyAssignment)
{
    FixedString<32> a("hello");
    FixedString<32> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(FixedString, MoveAssignment)
{
    FixedString<32> a("hello");
    FixedString<32> b;
    b = std::move(a);
    EXPECT_STREQ(b.CStr(), "hello");
}

TEST(FixedString, AssignFromCStr)
{
    FixedString<32> s;
    s = "world";
    EXPECT_STREQ(s.CStr(), "world");
}

TEST(FixedString, Append)
{
    FixedString<32> s("hello");
    s.Append(" world");
    EXPECT_STREQ(s.CStr(), "hello world");
}

TEST(FixedString, AppendStringView)
{
    FixedString<32> s("hello");
    std::string_view sv = " world";
    s.Append(sv);
    EXPECT_STREQ(s.CStr(), "hello world");
}

TEST(FixedString, OperatorPlusEqualCStr)
{
    FixedString<32> s("foo");
    s += "bar";
    EXPECT_STREQ(s.CStr(), "foobar");
}

TEST(FixedString, OperatorPlusEqualChar)
{
    FixedString<32> s("ab");
    s += 'c';
    EXPECT_STREQ(s.CStr(), "abc");
}

TEST(FixedString, OperatorPlusEqualStringView)
{
    FixedString<32> s("hello");
    std::string_view sv = "!";
    s += sv;
    EXPECT_STREQ(s.CStr(), "hello!");
}

TEST(FixedString, PushBack)
{
    FixedString<32> s("ab");
    s.PushBack('c');
    EXPECT_EQ(s.Size(), 3u);
    EXPECT_STREQ(s.CStr(), "abc");
}

TEST(FixedString, PopBack)
{
    FixedString<32> s("abc");
    s.PopBack();
    EXPECT_EQ(s.Size(), 2u);
    EXPECT_STREQ(s.CStr(), "ab");
}

TEST(FixedString, Clear)
{
    FixedString<32> s("hello");
    s.Clear();
    EXPECT_TRUE(s.Empty());
    EXPECT_STREQ(s.CStr(), "");
}

TEST(FixedString, SizeAndLength)
{
    FixedString<32> s("test");
    EXPECT_EQ(s.Size(), 4u);
    EXPECT_EQ(s.Length(), 4u);
}

TEST(FixedString, Capacity)
{
    FixedString<32> s;
    EXPECT_EQ(s.Capacity(), 32u);
}

TEST(FixedString, OperatorIndex)
{
    FixedString<32> s("abc");
    EXPECT_EQ(s[0], 'a');
    EXPECT_EQ(s[1], 'b');
    EXPECT_EQ(s[2], 'c');
    s[1] = 'x';
    EXPECT_EQ(s[1], 'x');
}

TEST(FixedString, FrontAndBack)
{
    FixedString<32> s("abc");
    EXPECT_EQ(s.Front(), 'a');
    EXPECT_EQ(s.Back(), 'c');
}

TEST(FixedString, FindChar)
{
    FixedString<32> s("hello world");
    EXPECT_EQ(s.Find('o'), 4u);
    EXPECT_EQ(s.Find('o', 5), 7u);
    EXPECT_EQ(s.Find('z'), FixedString<32>::npos);
}

TEST(FixedString, FindStr)
{
    FixedString<32> s("hello world");
    EXPECT_EQ(s.Find("world"), 6u);
    EXPECT_EQ(s.Find("xyz"), FixedString<32>::npos);
    EXPECT_EQ(s.Find("hello"), 0u);
}

TEST(FixedString, Equality)
{
    FixedString<32> a("hello");
    FixedString<32> b("hello");
    FixedString<32> c("world");
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(FixedString, EqualityWithCStr)
{
    FixedString<32> s("hello");
    EXPECT_TRUE(s == "hello");
    EXPECT_FALSE(s == "world");
}

TEST(FixedString, LessThan)
{
    FixedString<32> a("abc");
    FixedString<32> b("abd");
    FixedString<32> c("abc");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(a < c);
}

TEST(FixedString, ConversionToStringView)
{
    FixedString<32> s("test");
    std::string_view sv = s;
    EXPECT_EQ(sv, "test");
}

TEST(FixedString, Resize)
{
    FixedString<32> s("abc");
    s.Resize(5, 'x');
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.CStr(), "abcxx");
    s.Resize(2);
    EXPECT_EQ(s.Size(), 2u);
    EXPECT_STREQ(s.CStr(), "ab");
}

TEST(FixedString, Validate)
{
    FixedString<32> s("test");
    EXPECT_TRUE(s.Validate());
}

TEST(FixedString, Iteration)
{
    FixedString<32> s("abc");
    gx::String result;
    for (char c : s)
        result += c;
    EXPECT_EQ(result, "abc");
}

TEST(FixedString, OverflowAllowed)
{
    FixedString<4, true> s("abcd");
    s += "efgh";
    EXPECT_EQ(s.Size(), 8u);
    EXPECT_TRUE(s.IsUsingHeap());
    EXPECT_STREQ(s.CStr(), "abcdefgh");
}

TEST(FixedString, NullConstruction)
{
    FixedString<32> s(static_cast<const char*>(nullptr));
    EXPECT_TRUE(s.Empty());
}

// ============================================================================
// FixedList Tests
// ============================================================================

TEST(FixedList, DefaultConstruction)
{
    FixedList<int, 10> list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
}

TEST(FixedList, PushBack)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front(), 1);
    EXPECT_EQ(list.Back(), 3);
}

TEST(FixedList, PushFront)
{
    FixedList<int, 10> list;
    list.PushFront(1);
    list.PushFront(2);
    list.PushFront(3);
    EXPECT_EQ(list.Front(), 3);
    EXPECT_EQ(list.Back(), 1);
}

TEST(FixedList, PopBack)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.PopBack();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Back(), 2);
}

TEST(FixedList, PopFront)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.PopFront();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front(), 2);
}

TEST(FixedList, Insert)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(3);
    auto it = list.begin();
    ++it;
    list.Insert(it, 2);
    auto check = list.begin();
    EXPECT_EQ(*check++, 1);
    EXPECT_EQ(*check++, 2);
    EXPECT_EQ(*check++, 3);
}

TEST(FixedList, Erase)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    auto it = list.begin();
    ++it;
    list.Erase(it);
    EXPECT_EQ(list.Size(), 2u);
    auto check = list.begin();
    EXPECT_EQ(*check++, 1);
    EXPECT_EQ(*check++, 3);
}

TEST(FixedList, Clear)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.Clear();
    EXPECT_TRUE(list.Empty());
}

TEST(FixedList, MaxSize)
{
    FixedList<int, 5> list;
    EXPECT_EQ(list.MaxSize(), 5u);
}

TEST(FixedList, FillToCapacity)
{
    FixedList<int, 5> list;
    for (int i = 0; i < 5; ++i)
        list.PushBack(i);
    EXPECT_EQ(list.Size(), 5u);
}

TEST(FixedList, Iteration)
{
    FixedList<int, 10> list;
    list.PushBack(10);
    list.PushBack(20);
    list.PushBack(30);
    gx::Vector<int> result;
    for (auto it = list.begin(); it != list.end(); ++it)
        result.push_back(*it);
    EXPECT_EQ(result, (gx::Vector<int>{10, 20, 30}));
}

TEST(FixedList, CopyConstruction)
{
    FixedList<int, 10> a;
    a.PushBack(1);
    a.PushBack(2);
    a.PushBack(3);
    FixedList<int, 10> b(a);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_EQ(b.Front(), 1);
    EXPECT_EQ(b.Back(), 3);
}

TEST(FixedList, Validate)
{
    FixedList<int, 10> list;
    list.PushBack(1);
    list.PushBack(2);
    EXPECT_TRUE(list.Validate());
}

TEST(FixedList, ReuseFreedNodes)
{
    FixedList<int, 3> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.PopBack();
    list.PushBack(4);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Back(), 4);
}

// ============================================================================
// FixedHashMap Tests
// ============================================================================

TEST(FixedHashMap, DefaultConstruction)
{
    FixedHashMap<int, int, 16> map;
    EXPECT_TRUE(map.Empty());
    EXPECT_EQ(map.Size(), 0u);
}

TEST(FixedHashMap, InsertAndFind)
{
    FixedHashMap<int, int, 16> map;
    auto [it, ok] = map.Insert(1, 10);
    EXPECT_TRUE(ok);
    auto* val = map.Find(1);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 10);
}

TEST(FixedHashMap, InsertDuplicate)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    auto [it, ok] = map.Insert(1, 20);
    EXPECT_FALSE(ok);
    EXPECT_EQ(*map.Find(1), 10);
}

TEST(FixedHashMap, FindMissing)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    auto* val = map.Find(999);
    EXPECT_EQ(val, nullptr);
}

TEST(FixedHashMap, OperatorIndex)
{
    FixedHashMap<int, int, 16> map;
    map[1] = 10;
    map[2] = 20;
    EXPECT_EQ(map[1], 10);
    EXPECT_EQ(map[2], 20);
    map[1] = 100;
    EXPECT_EQ(map[1], 100);
}

TEST(FixedHashMap, Erase)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    map.Insert(2, 20);
    map.Insert(3, 30);
    EXPECT_TRUE(map.Erase(2));
    EXPECT_FALSE(map.Contains(2));
    EXPECT_EQ(map.Size(), 2u);
}

TEST(FixedHashMap, EraseNotFound)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    EXPECT_FALSE(map.Erase(999));
}

TEST(FixedHashMap, Contains)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(5, 50);
    EXPECT_TRUE(map.Contains(5));
    EXPECT_FALSE(map.Contains(6));
}

TEST(FixedHashMap, Clear)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    map.Insert(2, 20);
    map.Clear();
    EXPECT_TRUE(map.Empty());
}

TEST(FixedHashMap, MaxSize)
{
    FixedHashMap<int, int, 8> map;
    EXPECT_EQ(map.MaxSize(), 8u);
}

TEST(FixedHashMap, ManyElements)
{
    FixedHashMap<int, int, 32> map;
    for (int i = 0; i < 20; ++i)
        map.Insert(i, i * 10);
    EXPECT_EQ(map.Size(), 20u);
    for (int i = 0; i < 20; ++i)
    {
        auto* val = map.Find(i);
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, i * 10);
    }
}

TEST(FixedHashMap, Iteration)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    map.Insert(2, 20);
    map.Insert(3, 30);
    size_t count = 0;
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        ++count;
        EXPECT_TRUE(it.GetKey() >= 1 && it.GetKey() <= 3);
    }
    EXPECT_EQ(count, 3u);
}

TEST(FixedHashMap, Validate)
{
    FixedHashMap<int, int, 16> map;
    map.Insert(1, 10);
    EXPECT_TRUE(map.Validate());
}

TEST(FixedHashMap, CopyConstruction)
{
    FixedHashMap<int, int, 16> a;
    a.Insert(1, 10);
    a.Insert(2, 20);
    FixedHashMap<int, int, 16> b(a);
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_EQ(*b.Find(1), 10);
    EXPECT_EQ(*b.Find(2), 20);
}

// ============================================================================
// FixedSet Tests
// ============================================================================

TEST(FixedSet, DefaultConstruction)
{
    FixedSet<int, 10> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(FixedSet, Insert)
{
    FixedSet<int, 10> s;
    auto [it, ok] = s.Insert(5);
    EXPECT_TRUE(ok);
    EXPECT_EQ(*it, 5);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(FixedSet, InsertDuplicate)
{
    FixedSet<int, 10> s;
    s.Insert(5);
    auto [it, ok] = s.Insert(5);
    EXPECT_FALSE(ok);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(FixedSet, Find)
{
    FixedSet<int, 10> s;
    s.Insert(1);
    s.Insert(3);
    s.Insert(5);
    EXPECT_NE(s.Find(3), s.end());
    EXPECT_EQ(s.Find(2), s.end());
}

TEST(FixedSet, Contains)
{
    FixedSet<int, 10> s;
    s.Insert(42);
    EXPECT_TRUE(s.Contains(42));
    EXPECT_FALSE(s.Contains(43));
}

TEST(FixedSet, Erase)
{
    FixedSet<int, 10> s;
    s.Insert(1);
    s.Insert(2);
    s.Insert(3);
    EXPECT_TRUE(s.Erase(2));
    EXPECT_FALSE(s.Contains(2));
    EXPECT_EQ(s.Size(), 2u);
}

TEST(FixedSet, EraseNotFound)
{
    FixedSet<int, 10> s;
    s.Insert(1);
    EXPECT_FALSE(s.Erase(999));
}

TEST(FixedSet, SortedIteration)
{
    FixedSet<int, 10> s;
    s.Insert(5);
    s.Insert(1);
    s.Insert(3);
    s.Insert(2);
    s.Insert(4);

    gx::Vector<int> result(s.begin(), s.end());
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(FixedSet, Clear)
{
    FixedSet<int, 10> s;
    s.Insert(1);
    s.Insert(2);
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(FixedSet, MaxSize)
{
    FixedSet<int, 8> s;
    EXPECT_EQ(s.MaxSize(), 8u);
}

TEST(FixedSet, Validate)
{
    FixedSet<int, 10> s;
    s.Insert(1);
    s.Insert(2);
    s.Insert(3);
    EXPECT_TRUE(s.Validate());
}

TEST(FixedSet, CopyConstruction)
{
    FixedSet<int, 10> a;
    a.Insert(1);
    a.Insert(2);
    a.Insert(3);
    FixedSet<int, 10> b(a);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_TRUE(b.Contains(1));
    EXPECT_TRUE(b.Contains(2));
    EXPECT_TRUE(b.Contains(3));
}

// ============================================================================
// FixedMap Tests
// ============================================================================

TEST(FixedMap, DefaultConstruction)
{
    FixedMap<int, int, 10> m;
    EXPECT_TRUE(m.Empty());
    EXPECT_EQ(m.Size(), 0u);
}

TEST(FixedMap, InsertAndFind)
{
    FixedMap<int, int, 10> m;
    auto [it, ok] = m.Insert(1, 10);
    EXPECT_TRUE(ok);
    auto found = m.Find(1);
    ASSERT_NE(found, m.end());
    EXPECT_EQ(found->second, 10);
}

TEST(FixedMap, InsertDuplicate)
{
    FixedMap<int, int, 10> m;
    m.Insert(1, 10);
    auto [it, ok] = m.Insert(1, 20);
    EXPECT_FALSE(ok);
    EXPECT_EQ(m.Find(1)->second, 10);
}

TEST(FixedMap, OperatorIndex)
{
    FixedMap<int, int, 10> m;
    m[1] = 10;
    m[2] = 20;
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);
}

TEST(FixedMap, Erase)
{
    FixedMap<int, int, 10> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    EXPECT_TRUE(m.Erase(1));
    EXPECT_EQ(m.Size(), 1u);
    EXPECT_FALSE(m.Contains(1));
}

TEST(FixedMap, SortedOrder)
{
    FixedMap<int, int, 10> m;
    m.Insert(3, 30);
    m.Insert(1, 10);
    m.Insert(2, 20);
    auto it = m.begin();
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 3);
}

TEST(FixedMap, Clear)
{
    FixedMap<int, int, 10> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Clear();
    EXPECT_TRUE(m.Empty());
}

TEST(FixedMap, Validate)
{
    FixedMap<int, int, 10> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    EXPECT_TRUE(m.Validate());
}

// ============================================================================
// FixedMultiSet Tests
// ============================================================================

TEST(FixedMultiSet, DefaultConstruction)
{
    FixedMultiSet<int, 10> s;
    EXPECT_TRUE(s.Empty());
}

TEST(FixedMultiSet, InsertDuplicates)
{
    FixedMultiSet<int, 10> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(3);
    s.Insert(2);
    s.Insert(1);
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_EQ(s.Count(1), 2u);
    EXPECT_EQ(s.Count(3), 2u);
    EXPECT_EQ(s.Count(2), 1u);
}

TEST(FixedMultiSet, SortedOrder)
{
    FixedMultiSet<int, 10> s;
    s.Insert(3);
    s.Insert(1);
    s.Insert(2);
    s.Insert(1);
    gx::Vector<int> result(s.begin(), s.end());
    EXPECT_EQ(result, (gx::Vector<int>{1, 1, 2, 3}));
}

TEST(FixedMultiSet, Erase)
{
    FixedMultiSet<int, 10> s;
    s.Insert(1);
    s.Insert(1);
    s.Insert(2);
    auto removed = s.Erase(1);
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(FixedMultiSet, Find)
{
    FixedMultiSet<int, 10> s;
    s.Insert(1);
    s.Insert(2);
    EXPECT_NE(s.Find(1), s.end());
    EXPECT_EQ(s.Find(99), s.end());
}

TEST(FixedMultiSet, Clear)
{
    FixedMultiSet<int, 10> s;
    s.Insert(1);
    s.Insert(2);
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

// ============================================================================
// FixedHashSet Tests
// ============================================================================

TEST(FixedHashSet, DefaultConstruction)
{
    FixedHashSet<int, 16> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(FixedHashSet, Insert)
{
    FixedHashSet<int, 16> s;
    auto [it, ok] = s.Insert(42);
    EXPECT_TRUE(ok);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(FixedHashSet, InsertDuplicate)
{
    FixedHashSet<int, 16> s;
    s.Insert(42);
    auto [it, ok] = s.Insert(42);
    EXPECT_FALSE(ok);
    EXPECT_EQ(s.Size(), 1u);
}

TEST(FixedHashSet, Contains)
{
    FixedHashSet<int, 16> s;
    s.Insert(1);
    s.Insert(2);
    EXPECT_TRUE(s.Contains(1));
    EXPECT_TRUE(s.Contains(2));
    EXPECT_FALSE(s.Contains(3));
}

TEST(FixedHashSet, Find)
{
    FixedHashSet<int, 16> s;
    s.Insert(10);
    auto it = s.Find(10);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 10);
    EXPECT_EQ(s.Find(99), s.end());
}

TEST(FixedHashSet, Erase)
{
    FixedHashSet<int, 16> s;
    s.Insert(1);
    s.Insert(2);
    s.Insert(3);
    EXPECT_TRUE(s.Erase(2));
    EXPECT_FALSE(s.Contains(2));
    EXPECT_EQ(s.Size(), 2u);
}

TEST(FixedHashSet, Clear)
{
    FixedHashSet<int, 16> s;
    s.Insert(1);
    s.Insert(2);
    s.Clear();
    EXPECT_TRUE(s.Empty());
}

TEST(FixedHashSet, Validate)
{
    FixedHashSet<int, 16> s;
    s.Insert(1);
    s.Insert(2);
    EXPECT_TRUE(s.Validate());
}

TEST(FixedHashSet, ManyElements)
{
    FixedHashSet<int, 32> s;
    for (int i = 0; i < 20; ++i)
        s.Insert(i);
    EXPECT_EQ(s.Size(), 20u);
    for (int i = 0; i < 20; ++i)
        EXPECT_TRUE(s.Contains(i));
}
