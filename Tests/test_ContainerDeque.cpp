/// @file test_ContainerDeque.cpp
/// @brief Container Deque tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Deque.h"

using namespace gx::container;

// ============================================================================
// Construction
// ============================================================================

TEST(DequeTest, DefaultConstruction)
{
    Deque<int> d;
    EXPECT_EQ(d.Size(), 0u);
    EXPECT_TRUE(d.Empty());
}

TEST(DequeTest, ConstructWithCount)
{
    Deque<int> d(5);
    EXPECT_EQ(d.Size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(d[i], 0);
}

TEST(DequeTest, ConstructWithCountAndValue)
{
    Deque<int> d(4, 42);
    EXPECT_EQ(d.Size(), 4u);
    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(d[i], 42);
}

TEST(DequeTest, ConstructFromInitializerList)
{
    Deque<int> d = {10, 20, 30, 40, 50};
    EXPECT_EQ(d.Size(), 5u);
    EXPECT_EQ(d[0], 10);
    EXPECT_EQ(d[4], 50);
}

TEST(DequeTest, ConstructFromIteratorRange)
{
    gx::Vector<int> v = {1, 2, 3, 4};
    Deque<int> d(v.begin(), v.end());
    EXPECT_EQ(d.Size(), 4u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[3], 4);
}

// ============================================================================
// PushBack / PushFront
// ============================================================================

TEST(DequeTest, PushBack)
{
    Deque<int> d;
    d.PushBack(10);
    d.PushBack(20);
    d.PushBack(30);
    EXPECT_EQ(d.Size(), 3u);
    EXPECT_EQ(d[0], 10);
    EXPECT_EQ(d[1], 20);
    EXPECT_EQ(d[2], 30);
}

TEST(DequeTest, PushFront)
{
    Deque<int> d;
    d.PushFront(10);
    d.PushFront(20);
    d.PushFront(30);
    EXPECT_EQ(d.Size(), 3u);
    EXPECT_EQ(d[0], 30);
    EXPECT_EQ(d[1], 20);
    EXPECT_EQ(d[2], 10);
}

TEST(DequeTest, PushFrontAndBack)
{
    Deque<int> d;
    d.PushBack(2);
    d.PushFront(1);
    d.PushBack(3);
    d.PushFront(0);
    EXPECT_EQ(d.Size(), 4u);
    EXPECT_EQ(d[0], 0);
    EXPECT_EQ(d[1], 1);
    EXPECT_EQ(d[2], 2);
    EXPECT_EQ(d[3], 3);
}

TEST(DequeTest, EmplaceBackFront)
{
    Deque<gx::String> d;
    d.EmplaceBack("hello");
    d.EmplaceFront("world");
    EXPECT_EQ(d.Size(), 2u);
    EXPECT_EQ(d[0], "world");
    EXPECT_EQ(d[1], "hello");
}

// ============================================================================
// PopBack / PopFront
// ============================================================================

TEST(DequeTest, PopBack)
{
    Deque<int> d = {1, 2, 3};
    d.PopBack();
    EXPECT_EQ(d.Size(), 2u);
    EXPECT_EQ(d.Back(), 2);
}

TEST(DequeTest, PopFront)
{
    Deque<int> d = {1, 2, 3};
    d.PopFront();
    EXPECT_EQ(d.Size(), 2u);
    EXPECT_EQ(d.Front(), 2);
}

TEST(DequeTest, PopAllFront)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    while (!d.Empty())
        d.PopFront();
    EXPECT_EQ(d.Size(), 0u);
    EXPECT_TRUE(d.Empty());
}

TEST(DequeTest, PopAllBack)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    while (!d.Empty())
        d.PopBack();
    EXPECT_EQ(d.Size(), 0u);
    EXPECT_TRUE(d.Empty());
}

// ============================================================================
// Random Access
// ============================================================================

TEST(DequeTest, OperatorBracket)
{
    Deque<int> d = {10, 20, 30, 40, 50};
    EXPECT_EQ(d[0], 10);
    EXPECT_EQ(d[2], 30);
    EXPECT_EQ(d[4], 50);
    d[2] = 99;
    EXPECT_EQ(d[2], 99);
}

TEST(DequeTest, At)
{
    Deque<int> d = {10, 20, 30};
    EXPECT_EQ(d.At(0), 10);
    EXPECT_EQ(d.At(2), 30);
}

TEST(DequeTest, ConstOperatorBracket)
{
    const Deque<int> d = {1, 2, 3};
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[2], 3);
}

// ============================================================================
// Front / Back
// ============================================================================

TEST(DequeTest, FrontBack)
{
    Deque<int> d = {1, 2, 3};
    EXPECT_EQ(d.Front(), 1);
    EXPECT_EQ(d.Back(), 3);
    d.Front() = 99;
    d.Back() = 77;
    EXPECT_EQ(d[0], 99);
    EXPECT_EQ(d[2], 77);
}

TEST(DequeTest, ConstFrontBack)
{
    const Deque<int> d = {10, 20, 30};
    EXPECT_EQ(d.Front(), 10);
    EXPECT_EQ(d.Back(), 30);
}

// ============================================================================
// Size / Empty
// ============================================================================

TEST(DequeTest, SizeEmpty)
{
    Deque<int> d;
    EXPECT_EQ(d.Size(), 0u);
    EXPECT_TRUE(d.Empty());
    d.PushBack(1);
    EXPECT_EQ(d.Size(), 1u);
    EXPECT_FALSE(d.Empty());
}

// ============================================================================
// Insert
// ============================================================================

TEST(DequeTest, InsertAtBegin)
{
    Deque<int> d = {2, 3, 4};
    d.Insert(d.cbegin(), 1);
    EXPECT_EQ(d.Size(), 4u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 2);
}

TEST(DequeTest, InsertAtEnd)
{
    Deque<int> d = {1, 2, 3};
    d.Insert(d.cend(), 4);
    EXPECT_EQ(d.Size(), 4u);
    EXPECT_EQ(d[3], 4);
}

TEST(DequeTest, InsertInMiddle)
{
    Deque<int> d = {1, 2, 4, 5};
    auto it = d.cbegin() + 2;
    d.Insert(it, 3);
    EXPECT_EQ(d.Size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(d[i], static_cast<int>(i + 1));
}

TEST(DequeTest, InsertMove)
{
    Deque<gx::String> d;
    d.PushBack("a");
    d.PushBack("c");
    gx::String val = "b";
    d.Insert(d.cbegin() + 1, std::move(val));
    EXPECT_EQ(d[1], "b");
}

// ============================================================================
// Erase
// ============================================================================

TEST(DequeTest, EraseBegin)
{
    Deque<int> d = {1, 2, 3, 4};
    d.Erase(d.cbegin());
    EXPECT_EQ(d.Size(), 3u);
    EXPECT_EQ(d[0], 2);
}

TEST(DequeTest, EraseLast)
{
    Deque<int> d = {1, 2, 3, 4};
    d.Erase(d.cbegin() + 3);
    EXPECT_EQ(d.Size(), 3u);
    EXPECT_EQ(d.Back(), 3);
}

TEST(DequeTest, EraseMiddle)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    d.Erase(d.cbegin() + 2);
    EXPECT_EQ(d.Size(), 4u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 2);
    EXPECT_EQ(d[2], 4);
    EXPECT_EQ(d[3], 5);
}

TEST(DequeTest, EraseRange)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    d.Erase(d.cbegin() + 1, d.cbegin() + 4);
    EXPECT_EQ(d.Size(), 2u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 5);
}

// ============================================================================
// Clear
// ============================================================================

TEST(DequeTest, Clear)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    d.Clear();
    EXPECT_EQ(d.Size(), 0u);
    EXPECT_TRUE(d.Empty());
}

TEST(DequeTest, ClearAndReuse)
{
    Deque<int> d = {1, 2, 3};
    d.Clear();
    d.PushBack(10);
    d.PushFront(5);
    EXPECT_EQ(d.Size(), 2u);
    EXPECT_EQ(d[0], 5);
    EXPECT_EQ(d[1], 10);
}

// ============================================================================
// Iteration
// ============================================================================

TEST(DequeTest, RangeFor)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int x : d)
        sum += x;
    EXPECT_EQ(sum, 15);
}

TEST(DequeTest, IteratorArithmetic)
{
    Deque<int> d = {10, 20, 30, 40, 50};
    auto it = d.begin();
    EXPECT_EQ(*it, 10);
    EXPECT_EQ(*(it + 2), 30);
    EXPECT_EQ(d.end() - d.begin(), 5);
}

TEST(DequeTest, ConstIteration)
{
    const Deque<int> d = {1, 2, 3};
    int sum = 0;
    for (auto it = d.cbegin(); it != d.cend(); ++it)
        sum += *it;
    EXPECT_EQ(sum, 6);
}

// ============================================================================
// Copy / Move
// ============================================================================

TEST(DequeTest, CopyConstruction)
{
    Deque<int> d = {1, 2, 3, 4};
    Deque<int> copy(d);
    EXPECT_EQ(copy.Size(), 4u);
    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(copy[i], d[i]);
}

TEST(DequeTest, CopyAssignment)
{
    Deque<int> d = {1, 2, 3};
    Deque<int> other;
    other = d;
    EXPECT_EQ(other.Size(), 3u);
    EXPECT_EQ(other[0], 1);
    EXPECT_EQ(other[2], 3);
}

TEST(DequeTest, MoveConstruction)
{
    Deque<int> d = {1, 2, 3, 4};
    Deque<int> moved(std::move(d));
    EXPECT_EQ(moved.Size(), 4u);
    EXPECT_EQ(moved[0], 1);
    EXPECT_EQ(moved[3], 4);
    EXPECT_EQ(d.Size(), 0u);
}

TEST(DequeTest, MoveAssignment)
{
    Deque<int> d = {1, 2, 3};
    Deque<int> other;
    other = std::move(d);
    EXPECT_EQ(other.Size(), 3u);
    EXPECT_EQ(d.Size(), 0u);
}

TEST(DequeTest, InitializerListAssignment)
{
    Deque<int> d;
    d = {10, 20, 30};
    EXPECT_EQ(d.Size(), 3u);
    EXPECT_EQ(d[0], 10);
}

// ============================================================================
// Many elements (cross chunk boundaries)
// ============================================================================

TEST(DequeTest, ManyElementsPushBack)
{
    Deque<int> d;
    const int count = 1000;
    for (int i = 0; i < count; ++i)
        d.PushBack(i);
    EXPECT_EQ(d.Size(), static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        EXPECT_EQ(d[static_cast<size_t>(i)], i);
    EXPECT_TRUE(d.Validate());
}

TEST(DequeTest, ManyElementsPushFront)
{
    Deque<int> d;
    const int count = 1000;
    for (int i = 0; i < count; ++i)
        d.PushFront(i);
    EXPECT_EQ(d.Size(), static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        EXPECT_EQ(d[static_cast<size_t>(i)], count - 1 - i);
    EXPECT_TRUE(d.Validate());
}

TEST(DequeTest, ManyElementsMixed)
{
    Deque<int> d;
    // Alternate push front and back
    for (int i = 0; i < 500; ++i)
    {
        if (i % 2 == 0)
            d.PushBack(i);
        else
            d.PushFront(i);
    }
    EXPECT_EQ(d.Size(), 500u);
    EXPECT_TRUE(d.Validate());
}

TEST(DequeTest, ManyElementsPopMixed)
{
    Deque<int> d;
    for (int i = 0; i < 500; ++i)
        d.PushBack(i);
    for (int i = 0; i < 200; ++i)
        d.PopFront();
    for (int i = 0; i < 200; ++i)
        d.PopBack();
    EXPECT_EQ(d.Size(), 100u);
    EXPECT_EQ(d.Front(), 200);
    EXPECT_EQ(d.Back(), 299);
}

// ============================================================================
// Comparison
// ============================================================================

TEST(DequeTest, Equality)
{
    Deque<int> a = {1, 2, 3};
    Deque<int> b = {1, 2, 3};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(DequeTest, Inequality)
{
    Deque<int> a = {1, 2, 3};
    Deque<int> b = {1, 2, 4};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(DequeTest, InequalityDifferentSize)
{
    Deque<int> a = {1, 2};
    Deque<int> b = {1, 2, 3};
    EXPECT_FALSE(a == b);
}

// ============================================================================
// Resize
// ============================================================================

TEST(DequeTest, ResizeGrow)
{
    Deque<int> d = {1, 2, 3};
    d.Resize(5);
    EXPECT_EQ(d.Size(), 5u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[2], 3);
    EXPECT_EQ(d[3], 0);
    EXPECT_EQ(d[4], 0);
}

TEST(DequeTest, ResizeShrink)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    d.Resize(2);
    EXPECT_EQ(d.Size(), 2u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 2);
}

TEST(DequeTest, ResizeWithValue)
{
    Deque<int> d = {1, 2};
    d.Resize(5, 99);
    EXPECT_EQ(d.Size(), 5u);
    EXPECT_EQ(d[2], 99);
    EXPECT_EQ(d[4], 99);
}

// ============================================================================
// Validate
// ============================================================================

TEST(DequeTest, Validate)
{
    Deque<int> d = {1, 2, 3, 4, 5};
    EXPECT_TRUE(d.Validate());
}
