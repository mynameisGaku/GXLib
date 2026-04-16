/// @file test_ContainerRingBuffer.cpp
/// @brief Container RingBuffer tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/RingBuffer.h"

using namespace gx::container;

// ============================================================================
// Construction
// ============================================================================

TEST(RingBufferTest, DefaultConstruction)
{
    RingBuffer<int> rb;
    EXPECT_EQ(rb.Size(), 0u);
    EXPECT_EQ(rb.Capacity(), 0u);
    EXPECT_TRUE(rb.Empty());
}

TEST(RingBufferTest, ConstructWithCapacity)
{
    RingBuffer<int> rb(10);
    EXPECT_EQ(rb.Size(), 0u);
    EXPECT_EQ(rb.Capacity(), 10u);
    EXPECT_TRUE(rb.Empty());
    EXPECT_FALSE(rb.Full());
}

// ============================================================================
// PushBack
// ============================================================================

TEST(RingBufferTest, PushBackSingle)
{
    RingBuffer<int> rb(5);
    rb.PushBack(42);
    EXPECT_EQ(rb.Size(), 1u);
    EXPECT_EQ(rb.Front(), 42);
    EXPECT_EQ(rb.Back(), 42);
}

TEST(RingBufferTest, PushBackMultiple)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.PushBack(30);
    EXPECT_EQ(rb.Size(), 3u);
    EXPECT_EQ(rb.Front(), 10);
    EXPECT_EQ(rb.Back(), 30);
}

TEST(RingBufferTest, PushBackFillToCapacity)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PushBack(3);
    EXPECT_EQ(rb.Size(), 3u);
    EXPECT_TRUE(rb.Full());
    EXPECT_EQ(rb[0], 1);
    EXPECT_EQ(rb[1], 2);
    EXPECT_EQ(rb[2], 3);
}

TEST(RingBufferTest, PushBackMove)
{
    RingBuffer<gx::String> rb(3);
    gx::String s = "hello";
    rb.PushBack(std::move(s));
    EXPECT_EQ(rb.Front(), "hello");
}

TEST(RingBufferTest, EmplaceBack)
{
    RingBuffer<gx::String> rb(3);
    rb.EmplaceBack("test");
    EXPECT_EQ(rb.Front(), "test");
    EXPECT_EQ(rb.Size(), 1u);
}

// ============================================================================
// PopFront
// ============================================================================

TEST(RingBufferTest, PopFrontSingle)
{
    RingBuffer<int> rb(5);
    rb.PushBack(42);
    int val = rb.PopFront();
    EXPECT_EQ(val, 42);
    EXPECT_EQ(rb.Size(), 0u);
    EXPECT_TRUE(rb.Empty());
}

TEST(RingBufferTest, PopFrontMultiple)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.PushBack(30);
    EXPECT_EQ(rb.PopFront(), 10);
    EXPECT_EQ(rb.PopFront(), 20);
    EXPECT_EQ(rb.PopFront(), 30);
    EXPECT_TRUE(rb.Empty());
}

TEST(RingBufferTest, PopFrontReturnsOldest)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PushBack(3);
    EXPECT_EQ(rb.PopFront(), 1);
    EXPECT_EQ(rb.Front(), 2);
}

// ============================================================================
// Front / Back
// ============================================================================

TEST(RingBufferTest, FrontIsOldest)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.PushBack(30);
    EXPECT_EQ(rb.Front(), 10);
}

TEST(RingBufferTest, BackIsNewest)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.PushBack(30);
    EXPECT_EQ(rb.Back(), 30);
}

TEST(RingBufferTest, ConstFrontBack)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    const auto& cref = rb;
    EXPECT_EQ(cref.Front(), 1);
    EXPECT_EQ(cref.Back(), 2);
}

// ============================================================================
// operator[] (oldest-first indexing)
// ============================================================================

TEST(RingBufferTest, IndexAccessOldestFirst)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.PushBack(30);
    EXPECT_EQ(rb[0], 10); // oldest
    EXPECT_EQ(rb[1], 20);
    EXPECT_EQ(rb[2], 30); // newest
}

TEST(RingBufferTest, IndexAccessAfterOverwrite)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PushBack(3);
    rb.PushBack(4); // overwrites 1
    EXPECT_EQ(rb[0], 2); // oldest is now 2
    EXPECT_EQ(rb[1], 3);
    EXPECT_EQ(rb[2], 4); // newest
}

TEST(RingBufferTest, ConstIndexAccess)
{
    RingBuffer<int> rb(3);
    rb.PushBack(10);
    rb.PushBack(20);
    const auto& cref = rb;
    EXPECT_EQ(cref[0], 10);
    EXPECT_EQ(cref[1], 20);
}

// ============================================================================
// Size / Capacity / Empty / Full
// ============================================================================

TEST(RingBufferTest, SizeTracking)
{
    RingBuffer<int> rb(5);
    EXPECT_EQ(rb.Size(), 0u);
    rb.PushBack(1);
    EXPECT_EQ(rb.Size(), 1u);
    rb.PushBack(2);
    EXPECT_EQ(rb.Size(), 2u);
    rb.PopFront();
    EXPECT_EQ(rb.Size(), 1u);
}

TEST(RingBufferTest, CapacityFixed)
{
    RingBuffer<int> rb(10);
    EXPECT_EQ(rb.Capacity(), 10u);
    rb.PushBack(1);
    EXPECT_EQ(rb.Capacity(), 10u); // capacity does not change
}

TEST(RingBufferTest, EmptyAndFull)
{
    RingBuffer<int> rb(2);
    EXPECT_TRUE(rb.Empty());
    EXPECT_FALSE(rb.Full());
    rb.PushBack(1);
    EXPECT_FALSE(rb.Empty());
    EXPECT_FALSE(rb.Full());
    rb.PushBack(2);
    EXPECT_FALSE(rb.Empty());
    EXPECT_TRUE(rb.Full());
}

TEST(RingBufferTest, SizeDoesNotExceedCapacity)
{
    RingBuffer<int> rb(3);
    for (int i = 0; i < 10; ++i)
        rb.PushBack(i);
    EXPECT_EQ(rb.Size(), 3u);
    EXPECT_TRUE(rb.Full());
}

// ============================================================================
// Overwrite behavior (push when full overwrites oldest)
// ============================================================================

TEST(RingBufferTest, OverwriteOldest)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PushBack(3);
    EXPECT_TRUE(rb.Full());

    rb.PushBack(4); // overwrites 1
    EXPECT_EQ(rb.Size(), 3u);
    EXPECT_EQ(rb[0], 2);
    EXPECT_EQ(rb[1], 3);
    EXPECT_EQ(rb[2], 4);
}

TEST(RingBufferTest, OverwriteMultipleTimes)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PushBack(3);
    rb.PushBack(4); // overwrites 1
    rb.PushBack(5); // overwrites 2
    rb.PushBack(6); // overwrites 3
    EXPECT_EQ(rb[0], 4);
    EXPECT_EQ(rb[1], 5);
    EXPECT_EQ(rb[2], 6);
}

TEST(RingBufferTest, OverwriteFullCycle)
{
    RingBuffer<int> rb(3);
    // Push 9 elements, only last 3 should remain
    for (int i = 1; i <= 9; ++i)
        rb.PushBack(i);
    EXPECT_EQ(rb[0], 7);
    EXPECT_EQ(rb[1], 8);
    EXPECT_EQ(rb[2], 9);
}

TEST(RingBufferTest, OverwritePreservesOrder)
{
    RingBuffer<int> rb(4);
    for (int i = 0; i < 20; ++i)
        rb.PushBack(i);
    // Last 4 elements: 16, 17, 18, 19
    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(rb[i], static_cast<int>(16 + i));
}

// ============================================================================
// Clear
// ============================================================================

TEST(RingBufferTest, Clear)
{
    RingBuffer<int> rb(5);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PushBack(3);
    rb.Clear();
    EXPECT_EQ(rb.Size(), 0u);
    EXPECT_TRUE(rb.Empty());
    EXPECT_EQ(rb.Capacity(), 5u); // capacity preserved
}

TEST(RingBufferTest, ClearAndReuse)
{
    RingBuffer<int> rb(3);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.Clear();
    rb.PushBack(30);
    EXPECT_EQ(rb.Size(), 1u);
    EXPECT_EQ(rb.Front(), 30);
}

// ============================================================================
// Many push/pop cycles
// ============================================================================

TEST(RingBufferTest, ManyCycles)
{
    RingBuffer<int> rb(5);
    for (int cycle = 0; cycle < 100; ++cycle)
    {
        rb.PushBack(cycle);
    }
    // After 100 pushes into capacity-5 buffer, last 5 should remain
    EXPECT_EQ(rb.Size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(rb[i], static_cast<int>(95 + i));
}

TEST(RingBufferTest, AlternatingPushPop)
{
    RingBuffer<int> rb(5);
    for (int i = 0; i < 50; ++i)
    {
        rb.PushBack(i);
        if (rb.Size() > 3)
            rb.PopFront();
    }
    // Should have at most 3 elements at end
    EXPECT_LE(rb.Size(), 3u);
    EXPECT_TRUE(rb.Validate());
}

TEST(RingBufferTest, PushPopPushSequence)
{
    RingBuffer<int> rb(3);
    rb.PushBack(1);
    rb.PushBack(2);
    rb.PopFront(); // remove 1
    rb.PushBack(3);
    rb.PushBack(4);
    // Buffer contains: 2, 3, 4
    EXPECT_EQ(rb.Size(), 3u);
    EXPECT_EQ(rb[0], 2);
    EXPECT_EQ(rb[1], 3);
    EXPECT_EQ(rb[2], 4);
}

// ============================================================================
// Copy / Move
// ============================================================================

TEST(RingBufferTest, CopyConstruction)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    rb.PushBack(30);
    RingBuffer<int> copy(rb);
    EXPECT_EQ(copy.Size(), 3u);
    EXPECT_EQ(copy[0], 10);
    EXPECT_EQ(copy[1], 20);
    EXPECT_EQ(copy[2], 30);
}

TEST(RingBufferTest, CopyAssignment)
{
    RingBuffer<int> rb(5);
    rb.PushBack(1);
    rb.PushBack(2);
    RingBuffer<int> other(3);
    other = rb;
    EXPECT_EQ(other.Size(), 2u);
    EXPECT_EQ(other[0], 1);
    EXPECT_EQ(other[1], 2);
}

TEST(RingBufferTest, MoveConstruction)
{
    RingBuffer<int> rb(5);
    rb.PushBack(10);
    rb.PushBack(20);
    RingBuffer<int> moved(std::move(rb));
    EXPECT_EQ(moved.Size(), 2u);
    EXPECT_EQ(moved[0], 10);
    EXPECT_EQ(moved[1], 20);
    EXPECT_EQ(rb.Size(), 0u);
    EXPECT_EQ(rb.Capacity(), 0u);
}

TEST(RingBufferTest, MoveAssignment)
{
    RingBuffer<int> rb(5);
    rb.PushBack(1);
    rb.PushBack(2);
    RingBuffer<int> other(3);
    other = std::move(rb);
    EXPECT_EQ(other.Size(), 2u);
    EXPECT_EQ(other[0], 1);
    EXPECT_EQ(rb.Size(), 0u);
}

// ============================================================================
// Validate
// ============================================================================

TEST(RingBufferTest, ValidateEmpty)
{
    RingBuffer<int> rb;
    EXPECT_TRUE(rb.Validate());
}

TEST(RingBufferTest, ValidateWithElements)
{
    RingBuffer<int> rb(5);
    rb.PushBack(1);
    rb.PushBack(2);
    EXPECT_TRUE(rb.Validate());
}

TEST(RingBufferTest, ValidateAfterOverwrite)
{
    RingBuffer<int> rb(3);
    for (int i = 0; i < 10; ++i)
        rb.PushBack(i);
    EXPECT_TRUE(rb.Validate());
}

// ============================================================================
// Non-trivial types
// ============================================================================

TEST(RingBufferTest, StringOverwrite)
{
    RingBuffer<gx::String> rb(2);
    rb.PushBack("alpha");
    rb.PushBack("beta");
    rb.PushBack("gamma"); // overwrites "alpha"
    EXPECT_EQ(rb[0], "beta");
    EXPECT_EQ(rb[1], "gamma");
}

TEST(RingBufferTest, StringPopFront)
{
    RingBuffer<gx::String> rb(3);
    rb.PushBack("one");
    rb.PushBack("two");
    rb.PushBack("three");
    gx::String val = rb.PopFront();
    EXPECT_EQ(val, "one");
    EXPECT_EQ(rb.Size(), 2u);
    EXPECT_EQ(rb.Front(), "two");
}
