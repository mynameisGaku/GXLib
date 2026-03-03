/// @file test_ContainerAdapters.cpp
/// @brief Stack, Queue, PriorityQueue tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Stack.h"
#include "Container/Queue.h"
#include "Container/PriorityQueue.h"

#include <vector>
#include <string>

using namespace gx::container;

// ============================================================================
// Stack Tests
// ============================================================================

TEST(Stack, DefaultConstruction)
{
    Stack<int> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(Stack, PushAndTop)
{
    Stack<int> s;
    s.Push(10);
    EXPECT_EQ(s.Top(), 10);
    s.Push(20);
    EXPECT_EQ(s.Top(), 20);
    s.Push(30);
    EXPECT_EQ(s.Top(), 30);
}

TEST(Stack, PushMove)
{
    Stack<gx::String> s;
    gx::String val = "hello";
    s.Push(std::move(val));
    EXPECT_EQ(s.Top(), "hello");
}

TEST(Stack, Pop)
{
    Stack<int> s;
    s.Push(1);
    s.Push(2);
    s.Push(3);
    s.Pop();
    EXPECT_EQ(s.Top(), 2);
    s.Pop();
    EXPECT_EQ(s.Top(), 1);
}

TEST(Stack, LIFOOrder)
{
    Stack<int> s;
    for (int i = 0; i < 10; ++i)
        s.Push(i);
    for (int i = 9; i >= 0; --i)
    {
        EXPECT_EQ(s.Top(), i);
        s.Pop();
    }
    EXPECT_TRUE(s.Empty());
}

TEST(Stack, Size)
{
    Stack<int> s;
    EXPECT_EQ(s.Size(), 0u);
    s.Push(1);
    EXPECT_EQ(s.Size(), 1u);
    s.Push(2);
    EXPECT_EQ(s.Size(), 2u);
    s.Pop();
    EXPECT_EQ(s.Size(), 1u);
}

TEST(Stack, Empty)
{
    Stack<int> s;
    EXPECT_TRUE(s.Empty());
    s.Push(42);
    EXPECT_FALSE(s.Empty());
    s.Pop();
    EXPECT_TRUE(s.Empty());
}

TEST(Stack, Emplace)
{
    Stack<std::pair<int, int>> s;
    s.Emplace(1, 2);
    EXPECT_EQ(s.Top().first, 1);
    EXPECT_EQ(s.Top().second, 2);
}

TEST(Stack, Clear)
{
    Stack<int> s;
    s.Push(1);
    s.Push(2);
    s.Push(3);
    s.Clear();
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
}

TEST(Stack, TopMutable)
{
    Stack<int> s;
    s.Push(10);
    s.Top() = 20;
    EXPECT_EQ(s.Top(), 20);
}

TEST(Stack, ConstTop)
{
    Stack<int> s;
    s.Push(42);
    const auto& cs = s;
    EXPECT_EQ(cs.Top(), 42);
}

TEST(Stack, Validate)
{
    Stack<int> s;
    s.Push(1);
    s.Push(2);
    EXPECT_TRUE(s.Validate());
}

TEST(Stack, Equality)
{
    Stack<int> a, b;
    a.Push(1); a.Push(2);
    b.Push(1); b.Push(2);
    EXPECT_EQ(a, b);
    b.Push(3);
    EXPECT_NE(a, b);
}

TEST(Stack, GetContainer)
{
    Stack<int> s;
    s.Push(1);
    s.Push(2);
    auto& container = s.GetContainer();
    EXPECT_EQ(container.Size(), 2u);
}

TEST(Stack, ManyElements)
{
    Stack<int> s;
    for (int i = 0; i < 1000; ++i)
        s.Push(i);
    EXPECT_EQ(s.Size(), 1000u);
    EXPECT_EQ(s.Top(), 999);
}

TEST(Stack, PushPopAlternating)
{
    Stack<int> s;
    s.Push(1);
    s.Push(2);
    s.Pop();
    s.Push(3);
    EXPECT_EQ(s.Top(), 3);
    EXPECT_EQ(s.Size(), 2u);
}

// ============================================================================
// Queue Tests
// ============================================================================

TEST(Queue, DefaultConstruction)
{
    Queue<int> q;
    EXPECT_TRUE(q.Empty());
    EXPECT_EQ(q.Size(), 0u);
}

TEST(Queue, PushAndFront)
{
    Queue<int> q;
    q.Push(10);
    EXPECT_EQ(q.Front(), 10);
    q.Push(20);
    EXPECT_EQ(q.Front(), 10);
    EXPECT_EQ(q.Back(), 20);
}

TEST(Queue, PushMove)
{
    Queue<gx::String> q;
    gx::String val = "world";
    q.Push(std::move(val));
    EXPECT_EQ(q.Front(), "world");
}

TEST(Queue, Pop)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    q.Push(3);
    q.Pop();
    EXPECT_EQ(q.Front(), 2);
    q.Pop();
    EXPECT_EQ(q.Front(), 3);
}

TEST(Queue, FIFOOrder)
{
    Queue<int> q;
    for (int i = 0; i < 10; ++i)
        q.Push(i);
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(q.Front(), i);
        q.Pop();
    }
    EXPECT_TRUE(q.Empty());
}

TEST(Queue, FrontAndBack)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    q.Push(3);
    EXPECT_EQ(q.Front(), 1);
    EXPECT_EQ(q.Back(), 3);
}

TEST(Queue, Size)
{
    Queue<int> q;
    EXPECT_EQ(q.Size(), 0u);
    q.Push(1);
    EXPECT_EQ(q.Size(), 1u);
    q.Push(2);
    EXPECT_EQ(q.Size(), 2u);
    q.Pop();
    EXPECT_EQ(q.Size(), 1u);
}

TEST(Queue, Empty)
{
    Queue<int> q;
    EXPECT_TRUE(q.Empty());
    q.Push(42);
    EXPECT_FALSE(q.Empty());
    q.Pop();
    EXPECT_TRUE(q.Empty());
}

TEST(Queue, Emplace)
{
    Queue<std::pair<int, int>> q;
    q.Emplace(3, 4);
    EXPECT_EQ(q.Front().first, 3);
    EXPECT_EQ(q.Front().second, 4);
}

TEST(Queue, Clear)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    q.Push(3);
    q.Clear();
    EXPECT_TRUE(q.Empty());
}

TEST(Queue, FrontMutable)
{
    Queue<int> q;
    q.Push(10);
    q.Front() = 20;
    EXPECT_EQ(q.Front(), 20);
}

TEST(Queue, BackMutable)
{
    Queue<int> q;
    q.Push(10);
    q.Push(20);
    q.Back() = 30;
    EXPECT_EQ(q.Back(), 30);
}

TEST(Queue, ConstFrontAndBack)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    const auto& cq = q;
    EXPECT_EQ(cq.Front(), 1);
    EXPECT_EQ(cq.Back(), 2);
}

TEST(Queue, Validate)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    EXPECT_TRUE(q.Validate());
}

TEST(Queue, Equality)
{
    Queue<int> a, b;
    a.Push(1); a.Push(2); a.Push(3);
    b.Push(1); b.Push(2); b.Push(3);
    EXPECT_EQ(a, b);
    a.Pop();
    EXPECT_NE(a, b);
}

TEST(Queue, GetContainer)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    auto& container = q.GetContainer();
    EXPECT_EQ(container.Size(), 2u);
}

TEST(Queue, ManyElements)
{
    Queue<int> q;
    for (int i = 0; i < 1000; ++i)
        q.Push(i);
    EXPECT_EQ(q.Size(), 1000u);
    EXPECT_EQ(q.Front(), 0);
    EXPECT_EQ(q.Back(), 999);
}

TEST(Queue, PushPopAlternating)
{
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    q.Pop();
    q.Push(3);
    EXPECT_EQ(q.Front(), 2);
    EXPECT_EQ(q.Back(), 3);
    EXPECT_EQ(q.Size(), 2u);
}

TEST(Queue, WrapAround)
{
    Queue<int> q;
    for (int i = 0; i < 100; ++i)
    {
        q.Push(i);
        if (i > 10)
            q.Pop();
    }
    EXPECT_EQ(q.Size(), 11u);
}

// ============================================================================
// PriorityQueue Tests
// ============================================================================

TEST(PriorityQueue, DefaultConstruction)
{
    PriorityQueue<int> pq;
    EXPECT_TRUE(pq.Empty());
    EXPECT_EQ(pq.Size(), 0u);
}

TEST(PriorityQueue, PushAndTop)
{
    PriorityQueue<int> pq;
    pq.Push(5);
    EXPECT_EQ(pq.Top(), 5);
    pq.Push(10);
    EXPECT_EQ(pq.Top(), 10);
    pq.Push(3);
    EXPECT_EQ(pq.Top(), 10);
}

TEST(PriorityQueue, PushMove)
{
    PriorityQueue<int> pq;
    int val = 42;
    pq.Push(std::move(val));
    EXPECT_EQ(pq.Top(), 42);
}

TEST(PriorityQueue, Pop)
{
    PriorityQueue<int> pq;
    pq.Push(5);
    pq.Push(10);
    pq.Push(3);
    pq.Pop();
    EXPECT_EQ(pq.Top(), 5);
    pq.Pop();
    EXPECT_EQ(pq.Top(), 3);
}

TEST(PriorityQueue, MaxHeapOrder)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(5);
    pq.Push(3);
    pq.Push(7);
    pq.Push(2);
    pq.Push(9);
    pq.Push(4);

    gx::Vector<int> result;
    while (!pq.Empty())
    {
        result.push_back(pq.Top());
        pq.Pop();
    }
    EXPECT_EQ(result, (gx::Vector<int>{9, 7, 5, 4, 3, 2, 1}));
}

struct GreaterInt
{
    bool operator()(int a, int b) const noexcept { return a > b; }
};

TEST(PriorityQueue, CustomComparatorMinHeap)
{
    // GreaterInt means min-heap
    PriorityQueue<int, Deque<int>, GreaterInt> pq;
    pq.Push(5);
    pq.Push(1);
    pq.Push(3);
    pq.Push(7);
    pq.Push(2);

    gx::Vector<int> result;
    while (!pq.Empty())
    {
        result.push_back(pq.Top());
        pq.Pop();
    }
    EXPECT_EQ(result, (gx::Vector<int>{1, 2, 3, 5, 7}));
}

TEST(PriorityQueue, Size)
{
    PriorityQueue<int> pq;
    EXPECT_EQ(pq.Size(), 0u);
    pq.Push(1);
    EXPECT_EQ(pq.Size(), 1u);
    pq.Push(2);
    EXPECT_EQ(pq.Size(), 2u);
    pq.Pop();
    EXPECT_EQ(pq.Size(), 1u);
}

TEST(PriorityQueue, Empty)
{
    PriorityQueue<int> pq;
    EXPECT_TRUE(pq.Empty());
    pq.Push(42);
    EXPECT_FALSE(pq.Empty());
    pq.Pop();
    EXPECT_TRUE(pq.Empty());
}

TEST(PriorityQueue, Emplace)
{
    PriorityQueue<int> pq;
    pq.Emplace(42);
    EXPECT_EQ(pq.Top(), 42);
}

TEST(PriorityQueue, Clear)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(2);
    pq.Push(3);
    pq.Clear();
    EXPECT_TRUE(pq.Empty());
}

TEST(PriorityQueue, Change)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(5);
    pq.Push(3);
    // Modify element at index 2 (which is 3) to 10
    auto& container = pq.GetContainer();
    // Find the element with value 3
    for (size_t i = 0; i < container.Size(); ++i)
    {
        if (container[i] == 3)
        {
            container[i] = 10;
            pq.Change(i);
            break;
        }
    }
    EXPECT_EQ(pq.Top(), 10);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, ChangeDecrease)
{
    PriorityQueue<int> pq;
    pq.Push(10);
    pq.Push(5);
    pq.Push(8);
    // Change top (index 0) to 1
    auto& container = pq.GetContainer();
    container[0] = 1;
    pq.Change(0);
    EXPECT_EQ(pq.Top(), 8);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, RemoveArbitrary)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(5);
    pq.Push(3);
    pq.Push(7);
    pq.Push(2);
    // Remove element at index 0 (the max, 7)
    pq.Remove(0);
    EXPECT_EQ(pq.Top(), 5);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, RemoveLast)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(5);
    pq.Push(3);
    size_t last = pq.Size() - 1;
    pq.Remove(last);
    EXPECT_EQ(pq.Size(), 2u);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, RemoveMiddle)
{
    PriorityQueue<int> pq;
    for (int i = 1; i <= 10; ++i)
        pq.Push(i);
    pq.Remove(3);
    EXPECT_EQ(pq.Size(), 9u);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, Validate)
{
    PriorityQueue<int> pq;
    pq.Push(10);
    pq.Push(5);
    pq.Push(8);
    pq.Push(1);
    pq.Push(3);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, ManyElements)
{
    PriorityQueue<int> pq;
    for (int i = 0; i < 1000; ++i)
        pq.Push(i);
    EXPECT_EQ(pq.Size(), 1000u);
    EXPECT_EQ(pq.Top(), 999);
    EXPECT_TRUE(pq.Validate());

    int prev = pq.Top();
    while (!pq.Empty())
    {
        EXPECT_GE(prev, pq.Top());
        prev = pq.Top();
        pq.Pop();
    }
}

TEST(PriorityQueue, DuplicateElements)
{
    PriorityQueue<int> pq;
    pq.Push(5);
    pq.Push(5);
    pq.Push(5);
    pq.Push(3);
    pq.Push(3);
    EXPECT_EQ(pq.Top(), 5);
    pq.Pop();
    EXPECT_EQ(pq.Top(), 5);
    pq.Pop();
    EXPECT_EQ(pq.Top(), 5);
    pq.Pop();
    EXPECT_EQ(pq.Top(), 3);
}

TEST(PriorityQueue, ConstructFromIteratorRange)
{
    gx::Vector<int> src = {3, 1, 4, 1, 5, 9, 2, 6};
    PriorityQueue<int> pq(src.begin(), src.end());
    EXPECT_EQ(pq.Top(), 9);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, GetContainer)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(2);
    pq.Push(3);
    const auto& container = pq.GetContainer();
    EXPECT_EQ(container.Size(), 3u);
}

TEST(PriorityQueue, SingleElement)
{
    PriorityQueue<int> pq;
    pq.Push(42);
    EXPECT_EQ(pq.Top(), 42);
    pq.Pop();
    EXPECT_TRUE(pq.Empty());
}

TEST(PriorityQueue, RemoveAllFromTop)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(2);
    pq.Push(3);
    pq.Remove(0);
    pq.Remove(0);
    pq.Remove(0);
    EXPECT_TRUE(pq.Empty());
}

TEST(PriorityQueue, PushPopAlternating)
{
    PriorityQueue<int> pq;
    pq.Push(5);
    pq.Push(3);
    pq.Pop();
    pq.Push(7);
    EXPECT_EQ(pq.Top(), 7);
    pq.Pop();
    EXPECT_EQ(pq.Top(), 3);
}

TEST(PriorityQueue, ChangeIncrease)
{
    PriorityQueue<int> pq;
    pq.Push(1);
    pq.Push(2);
    pq.Push(3);
    // Change the smallest (somewhere in heap) to 10
    auto& c = pq.GetContainer();
    for (size_t i = 0; i < c.Size(); ++i)
    {
        if (c[i] == 1)
        {
            c[i] = 10;
            pq.Change(i);
            break;
        }
    }
    EXPECT_EQ(pq.Top(), 10);
    EXPECT_TRUE(pq.Validate());
}

TEST(PriorityQueue, StressTest)
{
    PriorityQueue<int> pq;
    // Push and pop many times
    for (int i = 0; i < 500; ++i)
        pq.Push(i);
    for (int i = 0; i < 250; ++i)
        pq.Pop();
    for (int i = 500; i < 750; ++i)
        pq.Push(i);
    EXPECT_TRUE(pq.Validate());

    int prev = pq.Top();
    while (!pq.Empty())
    {
        EXPECT_GE(prev, pq.Top());
        prev = pq.Top();
        pq.Pop();
    }
}
