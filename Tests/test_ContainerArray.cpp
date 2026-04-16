/// @file test_ContainerArray.cpp
/// @brief Container Array and Span tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Array.h"
#include "Container/Span.h"

using namespace gx::container;

// ============================================================================
// Array: Aggregate Initialization
// ============================================================================

TEST(ArrayTest, AggregateInit)
{
    Array<int, 3> a = {10, 20, 30};
    EXPECT_EQ(a[0], 10);
    EXPECT_EQ(a[1], 20);
    EXPECT_EQ(a[2], 30);
}

TEST(ArrayTest, ZeroInit)
{
    Array<int, 4> a = {};
    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(a[i], 0);
}

// ============================================================================
// Array: Element Access
// ============================================================================

TEST(ArrayTest, OperatorBracket)
{
    Array<int, 5> a = {1, 2, 3, 4, 5};
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[4], 5);
    a[2] = 99;
    EXPECT_EQ(a[2], 99);
}

TEST(ArrayTest, ConstOperatorBracket)
{
    const Array<int, 3> a = {10, 20, 30};
    EXPECT_EQ(a[0], 10);
    EXPECT_EQ(a[2], 30);
}

TEST(ArrayTest, Front)
{
    Array<int, 3> a = {100, 200, 300};
    EXPECT_EQ(a.Front(), 100);
    a.Front() = 42;
    EXPECT_EQ(a.Front(), 42);
}

TEST(ArrayTest, Back)
{
    Array<int, 3> a = {100, 200, 300};
    EXPECT_EQ(a.Back(), 300);
    a.Back() = 42;
    EXPECT_EQ(a.Back(), 42);
}

TEST(ArrayTest, ConstFrontBack)
{
    const Array<int, 3> a = {1, 2, 3};
    EXPECT_EQ(a.Front(), 1);
    EXPECT_EQ(a.Back(), 3);
}

TEST(ArrayTest, Data)
{
    Array<int, 3> a = {1, 2, 3};
    int* p = a.Data();
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[2], 3);
}

TEST(ArrayTest, ConstData)
{
    const Array<int, 3> a = {1, 2, 3};
    const int* p = a.Data();
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(p[1], 2);
}

// ============================================================================
// Array: Size / Empty
// ============================================================================

TEST(ArrayTest, Size)
{
    Array<int, 5> a = {};
    EXPECT_EQ(a.Size(), 5u);
    EXPECT_EQ(a.MaxSize(), 5u);
}

TEST(ArrayTest, Empty)
{
    Array<int, 5> a = {};
    EXPECT_FALSE(a.Empty());
}

// ============================================================================
// Array: Fill / Swap
// ============================================================================

TEST(ArrayTest, Fill)
{
    Array<int, 4> a = {};
    a.Fill(42);
    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(a[i], 42);
}

TEST(ArrayTest, Swap)
{
    Array<int, 3> a = {1, 2, 3};
    Array<int, 3> b = {4, 5, 6};
    a.Swap(b);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(a[2], 6);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[2], 3);
}

TEST(ArrayTest, ADLSwap)
{
    Array<int, 3> a = {1, 2, 3};
    Array<int, 3> b = {7, 8, 9};
    swap(a, b);
    EXPECT_EQ(a[0], 7);
    EXPECT_EQ(b[0], 1);
}

// ============================================================================
// Array: Iteration
// ============================================================================

TEST(ArrayTest, RangeFor)
{
    Array<int, 4> a = {10, 20, 30, 40};
    int sum = 0;
    for (int x : a)
        sum += x;
    EXPECT_EQ(sum, 100);
}

TEST(ArrayTest, BeginEnd)
{
    Array<int, 3> a = {1, 2, 3};
    EXPECT_EQ(a.end() - a.begin(), 3);
    EXPECT_EQ(*a.begin(), 1);
    EXPECT_EQ(*(a.end() - 1), 3);
}

TEST(ArrayTest, CBeginCEnd)
{
    Array<int, 3> a = {1, 2, 3};
    auto it = a.cbegin();
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(a.cend() - a.cbegin(), 3);
}

// ============================================================================
// Array: Comparison
// ============================================================================

TEST(ArrayTest, EqualityTrue)
{
    Array<int, 3> a = {1, 2, 3};
    Array<int, 3> b = {1, 2, 3};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(ArrayTest, EqualityFalse)
{
    Array<int, 3> a = {1, 2, 3};
    Array<int, 3> b = {1, 2, 4};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(ArrayTest, LessThan)
{
    Array<int, 3> a = {1, 2, 3};
    Array<int, 3> b = {1, 2, 4};
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(ArrayTest, GreaterThanEqualLessEqual)
{
    Array<int, 3> a = {1, 2, 3};
    Array<int, 3> b = {1, 2, 4};
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
}

// ============================================================================
// Array: Validate
// ============================================================================

TEST(ArrayTest, Validate)
{
    Array<int, 3> a = {1, 2, 3};
    EXPECT_TRUE(a.Validate());
}

// ============================================================================
// Array: Constexpr usage
// ============================================================================

TEST(ArrayTest, ConstexprSize)
{
    constexpr Array<int, 3> a = {1, 2, 3};
    static_assert(a.Size() == 3);
    static_assert(!a.Empty());
    EXPECT_EQ(a[0], 1);
}

// ============================================================================
// Span: Construction
// ============================================================================

TEST(SpanTest, DefaultConstruction)
{
    Span<int> s;
    EXPECT_EQ(s.Data(), nullptr);
    EXPECT_EQ(s.Size(), 0u);
    EXPECT_TRUE(s.Empty());
}

TEST(SpanTest, FromPointerAndCount)
{
    int arr[] = {10, 20, 30, 40};
    Span<int> s(arr, 4);
    EXPECT_EQ(s.Size(), 4u);
    EXPECT_EQ(s[0], 10);
    EXPECT_EQ(s[3], 40);
}

TEST(SpanTest, FromPointerRange)
{
    int arr[] = {10, 20, 30, 40};
    Span<int> s(arr, arr + 4);
    EXPECT_EQ(s.Size(), 4u);
    EXPECT_EQ(s[0], 10);
}

TEST(SpanTest, FromCArray)
{
    int arr[] = {1, 2, 3, 4, 5};
    Span s(arr);
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_EQ(s[4], 5);
}

TEST(SpanTest, FromVector)
{
    gx::Vector<int> v = {10, 20, 30};
    Span<int> s(v);
    EXPECT_EQ(s.Size(), 3u);
    EXPECT_EQ(s[0], 10);
}

// ============================================================================
// Span: Element Access
// ============================================================================

TEST(SpanTest, DataSizeSizeBytes)
{
    int arr[] = {1, 2, 3};
    Span<int> s(arr, 3);
    EXPECT_EQ(s.Data(), arr);
    EXPECT_EQ(s.Size(), 3u);
    EXPECT_EQ(s.SizeBytes(), 3u * sizeof(int));
}

TEST(SpanTest, OperatorBracket)
{
    int arr[] = {10, 20, 30};
    Span<int> s(arr, 3);
    EXPECT_EQ(s[0], 10);
    EXPECT_EQ(s[1], 20);
    EXPECT_EQ(s[2], 30);
    s[1] = 99;
    EXPECT_EQ(arr[1], 99);
}

TEST(SpanTest, FrontBack)
{
    int arr[] = {5, 10, 15};
    Span<int> s(arr, 3);
    EXPECT_EQ(s.Front(), 5);
    EXPECT_EQ(s.Back(), 15);
}

// ============================================================================
// Span: Subviews
// ============================================================================

TEST(SpanTest, First)
{
    int arr[] = {1, 2, 3, 4, 5};
    Span<int> s(arr, 5);
    auto sub = s.First(3);
    EXPECT_EQ(sub.Size(), 3u);
    EXPECT_EQ(sub[0], 1);
    EXPECT_EQ(sub[2], 3);
}

TEST(SpanTest, Last)
{
    int arr[] = {1, 2, 3, 4, 5};
    Span<int> s(arr, 5);
    auto sub = s.Last(2);
    EXPECT_EQ(sub.Size(), 2u);
    EXPECT_EQ(sub[0], 4);
    EXPECT_EQ(sub[1], 5);
}

TEST(SpanTest, Subspan)
{
    int arr[] = {10, 20, 30, 40, 50};
    Span<int> s(arr, 5);
    auto sub = s.Subspan(1, 3);
    EXPECT_EQ(sub.Size(), 3u);
    EXPECT_EQ(sub[0], 20);
    EXPECT_EQ(sub[2], 40);
}

TEST(SpanTest, SubspanDefaultCount)
{
    int arr[] = {10, 20, 30, 40, 50};
    Span<int> s(arr, 5);
    auto sub = s.Subspan(2);
    EXPECT_EQ(sub.Size(), 3u);
    EXPECT_EQ(sub[0], 30);
    EXPECT_EQ(sub[2], 50);
}

// ============================================================================
// Span: Iteration
// ============================================================================

TEST(SpanTest, RangeFor)
{
    int arr[] = {1, 2, 3, 4};
    Span<int> s(arr, 4);
    int sum = 0;
    for (int x : s)
        sum += x;
    EXPECT_EQ(sum, 10);
}

TEST(SpanTest, CBeginCEnd)
{
    int arr[] = {10, 20, 30};
    Span<int> s(arr, 3);
    EXPECT_EQ(s.cend() - s.cbegin(), 3);
    EXPECT_EQ(*s.cbegin(), 10);
}

// ============================================================================
// Span: Empty
// ============================================================================

TEST(SpanTest, EmptySpan)
{
    Span<int> s;
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(s.Size(), 0u);
    EXPECT_EQ(s.SizeBytes(), 0u);
}

// ============================================================================
// Span: const element type
// ============================================================================

TEST(SpanTest, ConstElementType)
{
    const int arr[] = {1, 2, 3};
    Span<const int> s(arr, 3);
    EXPECT_EQ(s[0], 1);
    EXPECT_EQ(s.Front(), 1);
    EXPECT_EQ(s.Back(), 3);
}
