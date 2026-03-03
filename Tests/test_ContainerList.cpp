/// @file test_ContainerList.cpp
/// @brief List, ForwardList, IntrusiveList, IntrusiveForwardList tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/List.h"
#include "Container/ForwardList.h"
#include "Container/IntrusiveList.h"
#include "Container/IntrusiveForwardList.h"

#include <vector>
#include <algorithm>
#include <numeric>

using namespace gx::container;

// ============================================================================
// List Tests
// ============================================================================

TEST(List, DefaultConstruction)
{
    List<int> list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
    EXPECT_TRUE(list.Validate());
}

TEST(List, ConstructWithCount)
{
    List<int> list(5);
    EXPECT_EQ(list.Size(), 5u);
    for (auto& v : list)
        EXPECT_EQ(v, 0);
    EXPECT_TRUE(list.Validate());
}

TEST(List, ConstructWithCountAndValue)
{
    List<int> list(3, 42);
    EXPECT_EQ(list.Size(), 3u);
    for (auto& v : list)
        EXPECT_EQ(v, 42);
}

TEST(List, ConstructFromIteratorRange)
{
    gx::Vector<int> src = {10, 20, 30};
    List<int> list(src.begin(), src.end());
    EXPECT_EQ(list.Size(), 3u);
    auto it = list.begin();
    EXPECT_EQ(*it++, 10);
    EXPECT_EQ(*it++, 20);
    EXPECT_EQ(*it++, 30);
}

TEST(List, ConstructFromInitializerList)
{
    List<int> list = {1, 2, 3, 4, 5};
    EXPECT_EQ(list.Size(), 5u);
    EXPECT_EQ(list.Front(), 1);
    EXPECT_EQ(list.Back(), 5);
}

TEST(List, CopyConstruction)
{
    List<int> original = {1, 2, 3};
    List<int> copy(original);
    EXPECT_EQ(copy.Size(), 3u);
    EXPECT_EQ(copy, original);
    EXPECT_TRUE(copy.Validate());
}

TEST(List, MoveConstruction)
{
    List<int> original = {1, 2, 3};
    List<int> moved(std::move(original));
    EXPECT_EQ(moved.Size(), 3u);
    EXPECT_TRUE(original.Empty());
    EXPECT_TRUE(moved.Validate());
}

TEST(List, CopyAssignment)
{
    List<int> a = {1, 2, 3};
    List<int> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(List, MoveAssignment)
{
    List<int> a = {1, 2, 3};
    List<int> b;
    b = std::move(a);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_TRUE(a.Empty());
}

TEST(List, InitializerListAssignment)
{
    List<int> list;
    list = {10, 20, 30};
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front(), 10);
    EXPECT_EQ(list.Back(), 30);
}

TEST(List, PushBack)
{
    List<int> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Back(), 3);
    EXPECT_TRUE(list.Validate());
}

TEST(List, PushBackMove)
{
    List<gx::String> list;
    gx::String s = "hello";
    list.PushBack(std::move(s));
    EXPECT_EQ(list.Front(), "hello");
}

TEST(List, PushFront)
{
    List<int> list;
    list.PushFront(1);
    list.PushFront(2);
    list.PushFront(3);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front(), 3);
    EXPECT_EQ(list.Back(), 1);
}

TEST(List, PopBack)
{
    List<int> list = {1, 2, 3};
    list.PopBack();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Back(), 2);
}

TEST(List, PopFront)
{
    List<int> list = {1, 2, 3};
    list.PopFront();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front(), 2);
}

TEST(List, EmplaceBack)
{
    List<std::pair<int, int>> list;
    list.EmplaceBack(1, 2);
    EXPECT_EQ(list.Front().first, 1);
    EXPECT_EQ(list.Front().second, 2);
}

TEST(List, EmplaceFront)
{
    List<int> list = {2, 3};
    list.EmplaceFront(1);
    EXPECT_EQ(list.Front(), 1);
}

TEST(List, InsertBeforeBegin)
{
    List<int> list = {2, 3};
    auto it = list.Insert(list.begin(), 1);
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(list.Front(), 1);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(List, InsertInMiddle)
{
    List<int> list = {1, 3};
    auto it = list.begin();
    ++it;
    list.Insert(it, 2);
    auto check = list.begin();
    EXPECT_EQ(*check++, 1);
    EXPECT_EQ(*check++, 2);
    EXPECT_EQ(*check++, 3);
}

TEST(List, InsertAtEnd)
{
    List<int> list = {1, 2};
    list.Insert(list.end(), 3);
    EXPECT_EQ(list.Back(), 3);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(List, EraseSingleElement)
{
    List<int> list = {1, 2, 3};
    auto it = list.begin();
    ++it;
    auto next = list.Erase(it);
    EXPECT_EQ(*next, 3);
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front(), 1);
    EXPECT_EQ(list.Back(), 3);
}

TEST(List, EraseRange)
{
    List<int> list = {1, 2, 3, 4, 5};
    auto first = list.begin();
    ++first;
    auto last = first;
    ++last;
    ++last;
    list.Erase(first, last);
    EXPECT_EQ(list.Size(), 3u);
    auto it = list.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST(List, Emplace)
{
    List<int> list = {1, 3};
    auto it = list.begin();
    ++it;
    list.Emplace(it, 2);
    auto check = list.begin();
    EXPECT_EQ(*check++, 1);
    EXPECT_EQ(*check++, 2);
    EXPECT_EQ(*check++, 3);
}

TEST(List, Clear)
{
    List<int> list = {1, 2, 3};
    list.Clear();
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
    EXPECT_TRUE(list.Validate());
}

TEST(List, FrontAndBack)
{
    List<int> list = {10, 20, 30};
    EXPECT_EQ(list.Front(), 10);
    EXPECT_EQ(list.Back(), 30);
    list.Front() = 100;
    EXPECT_EQ(list.Front(), 100);
}

TEST(List, ConstFrontAndBack)
{
    const List<int> list = {10, 20, 30};
    EXPECT_EQ(list.Front(), 10);
    EXPECT_EQ(list.Back(), 30);
}

TEST(List, IterationForward)
{
    List<int> list = {1, 2, 3, 4, 5};
    gx::Vector<int> result;
    for (auto& v : list)
        result.push_back(v);
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3, 4, 5}));
}

TEST(List, IterationBackward)
{
    List<int> list = {1, 2, 3};
    auto it = list.end();
    --it;
    EXPECT_EQ(*it, 3);
    --it;
    EXPECT_EQ(*it, 2);
    --it;
    EXPECT_EQ(*it, 1);
}

TEST(List, ConstIteration)
{
    const List<int> list = {1, 2, 3};
    int sum = 0;
    for (auto& v : list)
        sum += v;
    EXPECT_EQ(sum, 6);
}

TEST(List, SpliceAll)
{
    List<int> a = {1, 2, 3};
    List<int> b = {4, 5, 6};
    a.Splice(a.end(), b);
    EXPECT_EQ(a.Size(), 6u);
    EXPECT_TRUE(b.Empty());
    auto it = a.begin();
    for (int i = 1; i <= 6; ++i)
        EXPECT_EQ(*it++, i);
}

TEST(List, SpliceSingleElement)
{
    List<int> a = {1, 3};
    List<int> b = {2};
    auto pos = a.begin();
    ++pos;
    a.Splice(pos, b, b.begin());
    EXPECT_EQ(a.Size(), 3u);
    EXPECT_TRUE(b.Empty());
    auto it = a.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
}

TEST(List, SpliceRange)
{
    List<int> a = {1, 6};
    List<int> b = {2, 3, 4, 5};
    auto pos = a.begin();
    ++pos;
    auto bFirst = b.begin();
    auto bLast = b.end();
    a.Splice(pos, b, bFirst, bLast);
    EXPECT_EQ(a.Size(), 6u);
    EXPECT_TRUE(b.Empty());
}

TEST(List, Sort)
{
    List<int> list = {5, 3, 1, 4, 2};
    list.Sort();
    auto it = list.begin();
    for (int i = 1; i <= 5; ++i)
        EXPECT_EQ(*it++, i);
    EXPECT_TRUE(list.Validate());
}

TEST(List, SortAlreadySorted)
{
    List<int> list = {1, 2, 3, 4, 5};
    list.Sort();
    auto it = list.begin();
    for (int i = 1; i <= 5; ++i)
        EXPECT_EQ(*it++, i);
}

TEST(List, SortReversed)
{
    List<int> list = {5, 4, 3, 2, 1};
    list.Sort();
    auto it = list.begin();
    for (int i = 1; i <= 5; ++i)
        EXPECT_EQ(*it++, i);
}

TEST(List, SortCustomComparator)
{
    List<int> list = {1, 2, 3, 4, 5};
    list.Sort([](int a, int b) { return a > b; });
    auto it = list.begin();
    for (int i = 5; i >= 1; --i)
        EXPECT_EQ(*it++, i);
}

TEST(List, SortEmpty)
{
    List<int> list;
    list.Sort();
    EXPECT_TRUE(list.Empty());
}

TEST(List, SortSingleElement)
{
    List<int> list = {42};
    list.Sort();
    EXPECT_EQ(list.Front(), 42);
}

TEST(List, Merge)
{
    List<int> a = {1, 3, 5};
    List<int> b = {2, 4, 6};
    a.Merge(b);
    EXPECT_EQ(a.Size(), 6u);
    EXPECT_TRUE(b.Empty());
    auto it = a.begin();
    for (int i = 1; i <= 6; ++i)
        EXPECT_EQ(*it++, i);
}

TEST(List, MergeEmpty)
{
    List<int> a = {1, 2, 3};
    List<int> b;
    a.Merge(b);
    EXPECT_EQ(a.Size(), 3u);
}

TEST(List, MergeIntoEmpty)
{
    List<int> a;
    List<int> b = {1, 2, 3};
    a.Merge(b);
    EXPECT_EQ(a.Size(), 3u);
    EXPECT_TRUE(b.Empty());
}

TEST(List, Reverse)
{
    List<int> list = {1, 2, 3, 4, 5};
    list.Reverse();
    auto it = list.begin();
    for (int i = 5; i >= 1; --i)
        EXPECT_EQ(*it++, i);
}

TEST(List, ReverseEmpty)
{
    List<int> list;
    list.Reverse();
    EXPECT_TRUE(list.Empty());
}

TEST(List, ReverseSingle)
{
    List<int> list = {42};
    list.Reverse();
    EXPECT_EQ(list.Front(), 42);
}

TEST(List, Unique)
{
    List<int> list = {1, 1, 2, 2, 3, 3, 3};
    list.Unique();
    EXPECT_EQ(list.Size(), 3u);
    auto it = list.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
}

TEST(List, UniqueNoDuplicates)
{
    List<int> list = {1, 2, 3};
    list.Unique();
    EXPECT_EQ(list.Size(), 3u);
}

TEST(List, UniqueWithPredicate)
{
    List<int> list = {1, 2, 3, 4, 5};
    // Unique removes consecutive elements where pred is true (like std::list::unique)
    // pred(1,2)=true→remove 2, pred(1,3)=false, pred(3,4)=true→remove 4, pred(3,5)=false
    list.Unique([](int a, int b) { return b - a == 1; });
    EXPECT_EQ(list.Size(), 3u); // {1, 3, 5}
    EXPECT_EQ(list.Front(), 1);
    EXPECT_EQ(list.Back(), 5);
}

TEST(List, Remove)
{
    List<int> list = {1, 2, 3, 2, 4, 2};
    auto removed = list.Remove(2);
    EXPECT_EQ(removed, 3u);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(List, RemoveNotFound)
{
    List<int> list = {1, 2, 3};
    auto removed = list.Remove(99);
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(List, RemoveIf)
{
    List<int> list = {1, 2, 3, 4, 5, 6};
    auto removed = list.RemoveIf([](int v) { return v % 2 == 0; });
    EXPECT_EQ(removed, 3u);
    EXPECT_EQ(list.Size(), 3u);
    auto it = list.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 5);
}

TEST(List, Validate)
{
    List<int> list = {1, 2, 3, 4, 5};
    EXPECT_TRUE(list.Validate());
}

TEST(List, Equality)
{
    List<int> a = {1, 2, 3};
    List<int> b = {1, 2, 3};
    List<int> c = {1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(List, EqualityDifferentSizes)
{
    List<int> a = {1, 2};
    List<int> b = {1, 2, 3};
    EXPECT_NE(a, b);
}

TEST(List, Swap)
{
    List<int> a = {1, 2, 3};
    List<int> b = {4, 5};
    swap(a, b);
    EXPECT_EQ(a.Size(), 2u);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_EQ(a.Front(), 4);
    EXPECT_EQ(b.Front(), 1);
}

TEST(List, LargeListSort)
{
    List<int> list;
    for (int i = 100; i > 0; --i)
        list.PushBack(i);
    list.Sort();
    int expected = 1;
    for (auto& v : list)
        EXPECT_EQ(v, expected++);
    EXPECT_TRUE(list.Validate());
}

// ============================================================================
// ForwardList Tests
// ============================================================================

TEST(ForwardList, DefaultConstruction)
{
    ForwardList<int> list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
    EXPECT_TRUE(list.Validate());
}

TEST(ForwardList, ConstructFromInitializerList)
{
    ForwardList<int> list = {1, 2, 3};
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front(), 1);
}

TEST(ForwardList, PushFront)
{
    ForwardList<int> list;
    list.PushFront(3);
    list.PushFront(2);
    list.PushFront(1);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front(), 1);
}

TEST(ForwardList, PopFront)
{
    ForwardList<int> list = {1, 2, 3};
    list.PopFront();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front(), 2);
}

TEST(ForwardList, EmplaceFront)
{
    ForwardList<int> list;
    auto& ref = list.EmplaceFront(42);
    EXPECT_EQ(ref, 42);
    EXPECT_EQ(list.Front(), 42);
}

TEST(ForwardList, InsertAfter)
{
    ForwardList<int> list = {1, 3};
    auto it = list.begin();
    list.InsertAfter(it, 2);
    auto check = list.begin();
    EXPECT_EQ(*check++, 1);
    EXPECT_EQ(*check++, 2);
    EXPECT_EQ(*check++, 3);
}

TEST(ForwardList, InsertAfterBeforeBegin)
{
    ForwardList<int> list = {2, 3};
    list.InsertAfter(list.BeforeBegin(), 1);
    EXPECT_EQ(list.Front(), 1);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(ForwardList, EraseAfter)
{
    ForwardList<int> list = {1, 2, 3};
    list.EraseAfter(list.begin());
    EXPECT_EQ(list.Size(), 2u);
    auto it = list.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 3);
}

TEST(ForwardList, EraseAfterRange)
{
    ForwardList<int> list = {1, 2, 3, 4, 5};
    auto first = list.begin();
    auto last = first;
    ++last; ++last; ++last;
    list.EraseAfter(first, last);
    EXPECT_EQ(list.Size(), 3u);
    auto it = list.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST(ForwardList, Sort)
{
    ForwardList<int> list = {5, 3, 1, 4, 2};
    list.Sort();
    auto it = list.begin();
    for (int i = 1; i <= 5; ++i)
        EXPECT_EQ(*it++, i);
}

TEST(ForwardList, SortCustomComparator)
{
    ForwardList<int> list = {1, 2, 3, 4, 5};
    list.Sort([](int a, int b) { return a > b; });
    auto it = list.begin();
    for (int i = 5; i >= 1; --i)
        EXPECT_EQ(*it++, i);
}

TEST(ForwardList, Reverse)
{
    ForwardList<int> list = {1, 2, 3, 4, 5};
    list.Reverse();
    auto it = list.begin();
    for (int i = 5; i >= 1; --i)
        EXPECT_EQ(*it++, i);
}

TEST(ForwardList, Merge)
{
    ForwardList<int> a = {1, 3, 5};
    ForwardList<int> b = {2, 4, 6};
    a.Merge(b);
    EXPECT_EQ(a.Size(), 6u);
    EXPECT_TRUE(b.Empty());
    auto it = a.begin();
    for (int i = 1; i <= 6; ++i)
        EXPECT_EQ(*it++, i);
}

TEST(ForwardList, Unique)
{
    ForwardList<int> list = {1, 1, 2, 2, 3};
    list.Unique();
    EXPECT_EQ(list.Size(), 3u);
}

TEST(ForwardList, Remove)
{
    ForwardList<int> list = {1, 2, 3, 2, 4};
    auto removed = list.Remove(2);
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(ForwardList, RemoveIf)
{
    ForwardList<int> list = {1, 2, 3, 4, 5};
    auto removed = list.RemoveIf([](int v) { return v > 3; });
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(list.Size(), 3u);
}

TEST(ForwardList, Clear)
{
    ForwardList<int> list = {1, 2, 3};
    list.Clear();
    EXPECT_TRUE(list.Empty());
}

TEST(ForwardList, CopyConstruction)
{
    ForwardList<int> a = {1, 2, 3};
    ForwardList<int> b(a);
    EXPECT_EQ(a, b);
}

TEST(ForwardList, MoveConstruction)
{
    ForwardList<int> a = {1, 2, 3};
    ForwardList<int> b(std::move(a));
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_TRUE(a.Empty());
}

TEST(ForwardList, Equality)
{
    ForwardList<int> a = {1, 2, 3};
    ForwardList<int> b = {1, 2, 3};
    ForwardList<int> c = {1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(ForwardList, Validate)
{
    ForwardList<int> list = {1, 2, 3};
    EXPECT_TRUE(list.Validate());
}

// ============================================================================
// IntrusiveList Tests
// ============================================================================

struct ILItem
{
    int value = 0;
    IntrusiveListNode node{};

    ILItem() = default;
    explicit ILItem(int v) : value(v) {}

    bool operator<(const ILItem& other) const { return value < other.value; }
};

using IList = IntrusiveList<ILItem, &ILItem::node>;

TEST(IntrusiveList, DefaultConstruction)
{
    IList list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
    EXPECT_TRUE(list.Validate());
}

TEST(IntrusiveList, PushBack)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front().value, 1);
    EXPECT_EQ(list.Back().value, 3);
}

TEST(IntrusiveList, PushFront)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushFront(a);
    list.PushFront(b);
    list.PushFront(c);
    EXPECT_EQ(list.Front().value, 3);
    EXPECT_EQ(list.Back().value, 1);
}

TEST(IntrusiveList, PopBack)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    list.PopBack();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Back().value, 2);
}

TEST(IntrusiveList, PopFront)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    list.PopFront();
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front().value, 2);
}

TEST(IntrusiveList, Remove)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    list.Remove(b);
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front().value, 1);
    EXPECT_EQ(list.Back().value, 3);
}

TEST(IntrusiveList, Insert)
{
    IList list;
    ILItem a(1), b(3), c(2);
    list.PushBack(a);
    list.PushBack(b);
    auto it = list.begin();
    ++it;
    list.Insert(it, c);
    auto check = list.begin();
    EXPECT_EQ((*check++).value, 1);
    EXPECT_EQ((*check++).value, 2);
    EXPECT_EQ((*check++).value, 3);
}

TEST(IntrusiveList, Erase)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    auto it = list.begin();
    ++it;
    auto next = list.Erase(it);
    EXPECT_EQ((*next).value, 3);
    EXPECT_EQ(list.Size(), 2u);
}

TEST(IntrusiveList, Iteration)
{
    IList list;
    ILItem items[5] = {ILItem(10), ILItem(20), ILItem(30), ILItem(40), ILItem(50)};
    for (auto& item : items)
        list.PushBack(item);

    gx::Vector<int> result;
    for (auto& item : list)
        result.push_back(item.value);
    EXPECT_EQ(result, (gx::Vector<int>{10, 20, 30, 40, 50}));
}

TEST(IntrusiveList, Sort)
{
    IList list;
    ILItem items[5] = {ILItem(5), ILItem(3), ILItem(1), ILItem(4), ILItem(2)};
    for (auto& item : items)
        list.PushBack(item);

    list.Sort();
    int expected = 1;
    for (auto& item : list)
        EXPECT_EQ(item.value, expected++);
}

TEST(IntrusiveList, Clear)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    list.Clear();
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
    // Nodes should be reset
    EXPECT_EQ(a.node.prev, nullptr);
    EXPECT_EQ(a.node.next, nullptr);
}

TEST(IntrusiveList, Reverse)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    list.Reverse();
    EXPECT_EQ(list.Front().value, 3);
    EXPECT_EQ(list.Back().value, 1);
}

TEST(IntrusiveList, Splice)
{
    IList listA, listB;
    ILItem a(1), b(2), c(3), d(4);
    listA.PushBack(a);
    listA.PushBack(b);
    listB.PushBack(c);
    listB.PushBack(d);
    listA.Splice(listA.end(), listB);
    EXPECT_EQ(listA.Size(), 4u);
    EXPECT_TRUE(listB.Empty());
}

TEST(IntrusiveList, MoveConstruction)
{
    IList list;
    ILItem a(1), b(2);
    list.PushBack(a);
    list.PushBack(b);
    IList moved(std::move(list));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_TRUE(list.Empty());
}

TEST(IntrusiveList, Validate)
{
    IList list;
    ILItem a(1), b(2), c(3);
    list.PushBack(a);
    list.PushBack(b);
    list.PushBack(c);
    EXPECT_TRUE(list.Validate());
}

// ============================================================================
// IntrusiveForwardList Tests
// ============================================================================

struct IFLItem
{
    int value = 0;
    IntrusiveForwardListNode node{};

    IFLItem() = default;
    explicit IFLItem(int v) : value(v) {}
};

using IFList = IntrusiveForwardList<IFLItem, &IFLItem::node>;

TEST(IntrusiveForwardList, DefaultConstruction)
{
    IFList list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.Size(), 0u);
}

TEST(IntrusiveForwardList, PushFront)
{
    IFList list;
    IFLItem a(1), b(2), c(3);
    list.PushFront(a);
    list.PushFront(b);
    list.PushFront(c);
    EXPECT_EQ(list.Size(), 3u);
    EXPECT_EQ(list.Front().value, 3);
}

TEST(IntrusiveForwardList, PopFront)
{
    IFList list;
    IFLItem a(1), b(2);
    list.PushFront(b);
    list.PushFront(a);
    list.PopFront();
    EXPECT_EQ(list.Size(), 1u);
    EXPECT_EQ(list.Front().value, 2);
}

TEST(IntrusiveForwardList, InsertAfter)
{
    IFList list;
    IFLItem a(1), b(3), c(2);
    list.PushFront(b);
    list.PushFront(a);
    auto it = list.begin();
    list.InsertAfter(it, c);
    auto check = list.begin();
    EXPECT_EQ((*check++).value, 1);
    EXPECT_EQ((*check++).value, 2);
    EXPECT_EQ((*check++).value, 3);
}

TEST(IntrusiveForwardList, EraseAfter)
{
    IFList list;
    IFLItem a(1), b(2), c(3);
    list.PushFront(c);
    list.PushFront(b);
    list.PushFront(a);
    list.EraseAfter(list.begin());
    EXPECT_EQ(list.Size(), 2u);
    auto it = list.begin();
    EXPECT_EQ((*it++).value, 1);
    EXPECT_EQ((*it++).value, 3);
}

TEST(IntrusiveForwardList, Remove)
{
    IFList list;
    IFLItem a(1), b(2), c(3);
    list.PushFront(c);
    list.PushFront(b);
    list.PushFront(a);
    list.Remove(b);
    EXPECT_EQ(list.Size(), 2u);
    EXPECT_EQ(list.Front().value, 1);
}

TEST(IntrusiveForwardList, Reverse)
{
    IFList list;
    IFLItem a(1), b(2), c(3);
    list.PushFront(c);
    list.PushFront(b);
    list.PushFront(a);
    list.Reverse();
    EXPECT_EQ(list.Front().value, 3);
}

TEST(IntrusiveForwardList, RemoveIf)
{
    IFList list;
    IFLItem a(1), b(2), c(3), d(4);
    list.PushFront(d);
    list.PushFront(c);
    list.PushFront(b);
    list.PushFront(a);
    auto removed = list.RemoveIf([](const IFLItem& item) { return item.value % 2 == 0; });
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(list.Size(), 2u);
}

TEST(IntrusiveForwardList, Clear)
{
    IFList list;
    IFLItem a(1), b(2), c(3);
    list.PushFront(c);
    list.PushFront(b);
    list.PushFront(a);
    list.Clear();
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(a.node.next, nullptr);
}

TEST(IntrusiveForwardList, Validate)
{
    IFList list;
    IFLItem a(1), b(2);
    list.PushFront(b);
    list.PushFront(a);
    EXPECT_TRUE(list.Validate());
}

TEST(IntrusiveForwardList, MoveConstruction)
{
    IFList list;
    IFLItem a(1), b(2);
    list.PushFront(b);
    list.PushFront(a);
    IFList moved(std::move(list));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_TRUE(list.Empty());
}

TEST(IntrusiveForwardList, Iteration)
{
    IFList list;
    IFLItem items[3] = {IFLItem(1), IFLItem(2), IFLItem(3)};
    list.PushFront(items[2]);
    list.PushFront(items[1]);
    list.PushFront(items[0]);

    gx::Vector<int> result;
    for (auto& item : list)
        result.push_back(item.value);
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3}));
}
