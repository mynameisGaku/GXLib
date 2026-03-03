/// @file test_ContainerAlgorithm.cpp
/// @brief Container Algorithm (sort, search, heap, utility) tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Algorithm.h"

#include <vector>
#include <numeric>
#include <random>

using namespace gx::container;

// ============================================================================
// Min / Max / Clamp
// ============================================================================

TEST(AlgorithmMinMaxClamp, MinReturnsSmaller)
{
    EXPECT_EQ(Min(3, 7), 3);
    EXPECT_EQ(Min(7, 3), 3);
    EXPECT_EQ(Min(-1, 0), -1);
}

TEST(AlgorithmMinMaxClamp, MinEqualValues)
{
    EXPECT_EQ(Min(5, 5), 5);
}

TEST(AlgorithmMinMaxClamp, MaxReturnsLarger)
{
    EXPECT_EQ(Max(3, 7), 7);
    EXPECT_EQ(Max(7, 3), 7);
    EXPECT_EQ(Max(-1, 0), 0);
}

TEST(AlgorithmMinMaxClamp, MaxEqualValues)
{
    EXPECT_EQ(Max(5, 5), 5);
}

TEST(AlgorithmMinMaxClamp, ClampWithinRange)
{
    EXPECT_EQ(Clamp(5, 0, 10), 5);
}

TEST(AlgorithmMinMaxClamp, ClampBelowRange)
{
    EXPECT_EQ(Clamp(-5, 0, 10), 0);
}

TEST(AlgorithmMinMaxClamp, ClampAboveRange)
{
    EXPECT_EQ(Clamp(15, 0, 10), 10);
}

TEST(AlgorithmMinMaxClamp, ClampAtBoundaries)
{
    EXPECT_EQ(Clamp(0, 0, 10), 0);
    EXPECT_EQ(Clamp(10, 0, 10), 10);
}

// ============================================================================
// Median
// ============================================================================

TEST(AlgorithmMedian, AscendingOrder)
{
    EXPECT_EQ(Median(1, 2, 3), 2);
}

TEST(AlgorithmMedian, DescendingOrder)
{
    EXPECT_EQ(Median(3, 2, 1), 2);
}

TEST(AlgorithmMedian, MiddleIsSmallest)
{
    EXPECT_EQ(Median(2, 1, 3), 2);
}

TEST(AlgorithmMedian, AllEqual)
{
    EXPECT_EQ(Median(5, 5, 5), 5);
}

TEST(AlgorithmMedian, TwoEqual)
{
    EXPECT_EQ(Median(1, 1, 3), 1);
    EXPECT_EQ(Median(3, 1, 3), 3);
}

TEST(AlgorithmMedian, CustomComparator)
{
    auto greater = [](int a, int b) { return a > b; };
    // With reversed comparator, median of {1,2,3} interpreted as "reverse" should still give 2
    EXPECT_EQ(Median(1, 2, 3, greater), 2);
}

// ============================================================================
// Swap
// ============================================================================

TEST(AlgorithmSwap, BasicSwap)
{
    int a = 10, b = 20;
    Swap(a, b);
    EXPECT_EQ(a, 20);
    EXPECT_EQ(b, 10);
}

TEST(AlgorithmSwap, SwapStrings)
{
    gx::String a = "hello", b = "world";
    Swap(a, b);
    EXPECT_EQ(a, "world");
    EXPECT_EQ(b, "hello");
}

// ============================================================================
// InsertionSort
// ============================================================================

TEST(AlgorithmInsertionSort, EmptyRange)
{
    gx::Vector<int> v;
    InsertionSort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(AlgorithmInsertionSort, SingleElement)
{
    gx::Vector<int> v = {42};
    InsertionSort(v.begin(), v.end());
    EXPECT_EQ(v[0], 42);
}

TEST(AlgorithmInsertionSort, SmallArray)
{
    gx::Vector<int> v = {5, 3, 8, 1, 2, 9, 4, 7, 6};
    InsertionSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmInsertionSort, AlreadySorted)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5};
    InsertionSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmInsertionSort, ReverseSorted)
{
    gx::Vector<int> v = {5, 4, 3, 2, 1};
    InsertionSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmInsertionSort, CustomComparator)
{
    gx::Vector<int> v = {1, 5, 3, 2, 4};
    InsertionSort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    // Should be descending
    for (size_t i = 1; i < v.size(); ++i)
        EXPECT_GE(v[i - 1], v[i]);
}

// ============================================================================
// ShellSort
// ============================================================================

TEST(AlgorithmShellSort, EmptyRange)
{
    gx::Vector<int> v;
    ShellSort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(AlgorithmShellSort, MediumArray)
{
    gx::Vector<int> v(100);
    std::iota(v.begin(), v.end(), 0);
    std::mt19937 rng(12345);
    std::shuffle(v.begin(), v.end(), rng);
    ShellSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmShellSort, AlreadySorted)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
    ShellSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmShellSort, ReverseSorted)
{
    gx::Vector<int> v = {8, 7, 6, 5, 4, 3, 2, 1};
    ShellSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

// ============================================================================
// QuickSort
// ============================================================================

TEST(AlgorithmQuickSort, EmptyRange)
{
    gx::Vector<int> v;
    QuickSort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(AlgorithmQuickSort, SingleElement)
{
    gx::Vector<int> v = {1};
    QuickSort(v.begin(), v.end());
    EXPECT_EQ(v[0], 1);
}

TEST(AlgorithmQuickSort, RandomData)
{
    gx::Vector<int> v(500);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    for (auto& x : v) x = dist(rng);
    QuickSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmQuickSort, SortedData)
{
    gx::Vector<int> v(200);
    std::iota(v.begin(), v.end(), 0);
    QuickSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmQuickSort, ReverseData)
{
    gx::Vector<int> v(200);
    std::iota(v.begin(), v.end(), 0);
    std::reverse(v.begin(), v.end());
    QuickSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmQuickSort, AllDuplicates)
{
    gx::Vector<int> v(100, 42);
    QuickSort(v.begin(), v.end());
    for (auto x : v) EXPECT_EQ(x, 42);
}

TEST(AlgorithmQuickSort, CustomComparator)
{
    gx::Vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
    QuickSort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    for (size_t i = 1; i < v.size(); ++i)
        EXPECT_GE(v[i - 1], v[i]);
}

// ============================================================================
// MergeSortBuffer (stability check)
// ============================================================================

TEST(AlgorithmMergeSort, EmptyRange)
{
    gx::Vector<int> v;
    MergeSortBuffer(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(AlgorithmMergeSort, BasicSort)
{
    gx::Vector<int> v = {5, 2, 8, 1, 9, 3};
    MergeSortBuffer(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmMergeSort, StabilityCheck)
{
    // Pairs where first is the sort key, second is original index
    struct Item { int key; int origIndex; };
    gx::Vector<Item> items = {
        {3, 0}, {1, 1}, {3, 2}, {2, 3}, {1, 4}, {2, 5}
    };
    MergeSortBuffer(items.begin(), items.end(),
        [](const Item& a, const Item& b) { return a.key < b.key; });

    // Verify sorted by key
    for (size_t i = 1; i < items.size(); ++i)
        EXPECT_LE(items[i - 1].key, items[i].key);

    // Verify stability: equal keys should preserve original order
    for (size_t i = 1; i < items.size(); ++i)
    {
        if (items[i - 1].key == items[i].key)
            EXPECT_LT(items[i - 1].origIndex, items[i].origIndex);
    }
}

TEST(AlgorithmMergeSort, LargeRandom)
{
    gx::Vector<int> v(300);
    std::mt19937 rng(999);
    std::uniform_int_distribution<int> dist(0, 10000);
    for (auto& x : v) x = dist(rng);
    MergeSortBuffer(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

// ============================================================================
// HeapSort
// ============================================================================

TEST(AlgorithmHeapSort, EmptyRange)
{
    gx::Vector<int> v;
    HeapSort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(AlgorithmHeapSort, BasicSort)
{
    gx::Vector<int> v = {9, 3, 7, 1, 5, 8, 2, 4, 6};
    HeapSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmHeapSort, AlreadySorted)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5};
    HeapSort(v.begin(), v.end());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

// ============================================================================
// RadixSort (unsigned)
// ============================================================================

TEST(AlgorithmRadixSort, EmptyRange)
{
    uint32_t* ptr = nullptr;
    RadixSort(ptr, ptr); // no-op
}

TEST(AlgorithmRadixSort, SingleElement)
{
    uint32_t v[] = {42};
    RadixSort(v, v + 1);
    EXPECT_EQ(v[0], 42u);
}

TEST(AlgorithmRadixSort, UnsignedInts)
{
    gx::Vector<uint32_t> v = {100, 3, 255, 0, 1024, 50, 999};
    RadixSort(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmRadixSort, Uint8)
{
    gx::Vector<uint8_t> v = {200, 3, 255, 0, 128, 50};
    RadixSort(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmRadixSort, Uint64)
{
    gx::Vector<uint64_t> v = {UINT64_MAX, 0, 1, UINT64_MAX - 1, 12345678901234ULL};
    RadixSort(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmRadixSort, LargeUnsigned)
{
    gx::Vector<uint32_t> v(500);
    std::mt19937 rng(77);
    std::uniform_int_distribution<uint32_t> dist(0, 1000000);
    for (auto& x : v) x = dist(rng);
    RadixSort(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

// ============================================================================
// RadixSortSigned
// ============================================================================

TEST(AlgorithmRadixSortSigned, BasicSigned)
{
    gx::Vector<int32_t> v = {-5, 3, -1, 0, 7, -100, 50};
    RadixSortSigned(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmRadixSortSigned, AllNegative)
{
    gx::Vector<int32_t> v = {-1, -3, -5, -2, -4};
    RadixSortSigned(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmRadixSortSigned, MixedWithExtremes)
{
    gx::Vector<int32_t> v = {INT32_MAX, INT32_MIN, 0, -1, 1};
    RadixSortSigned(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmRadixSortSigned, LargeSigned)
{
    gx::Vector<int32_t> v(500);
    std::mt19937 rng(88);
    std::uniform_int_distribution<int32_t> dist(-500000, 500000);
    for (auto& x : v) x = dist(rng);
    RadixSortSigned(v.data(), v.data() + v.size());
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

// ============================================================================
// Heap operations: MakeHeap, PushHeap, PopHeap
// ============================================================================

static auto less = [](int a, int b) { return a < b; };

static bool IsMaxHeap(const gx::Vector<int>& v, size_t n)
{
    for (size_t i = 1; i < n; ++i)
    {
        size_t parent = (i - 1) / 2;
        if (v[parent] < v[i]) return false;
    }
    return true;
}

TEST(AlgorithmHeap, MakeHeap)
{
    gx::Vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
    MakeHeap(v.begin(), v.size(), less);
    EXPECT_TRUE(IsMaxHeap(v, v.size()));
}

TEST(AlgorithmHeap, PushHeap)
{
    gx::Vector<int> v = {9, 6, 5, 1, 3, 4, 2, 1};
    MakeHeap(v.begin(), v.size(), less);
    v.push_back(10);
    PushHeap(v.begin(), v.size(), less);
    EXPECT_TRUE(IsMaxHeap(v, v.size()));
    EXPECT_EQ(v[0], 10);
}

TEST(AlgorithmHeap, PopHeap)
{
    gx::Vector<int> v = {9, 6, 5, 1, 3, 4, 2, 1};
    MakeHeap(v.begin(), v.size(), less);
    PopHeap(v.begin(), v.size(), less);
    // After pop, the max should be at back
    EXPECT_EQ(v.back(), 9);
    // The remaining [0, n-1) should be a valid heap
    EXPECT_TRUE(IsMaxHeap(v, v.size() - 1));
}

TEST(AlgorithmHeap, ChangeHeapIncrease)
{
    gx::Vector<int> v = {9, 6, 5, 1, 3, 4, 2, 1};
    MakeHeap(v.begin(), v.size(), less);
    v[5] = 20; // increase element at index 5
    ChangeHeap(v.begin(), v.size(), 5, less);
    EXPECT_TRUE(IsMaxHeap(v, v.size()));
}

TEST(AlgorithmHeap, ChangeHeapDecrease)
{
    gx::Vector<int> v = {9, 6, 5, 1, 3, 4, 2, 1};
    MakeHeap(v.begin(), v.size(), less);
    v[0] = 0; // decrease root
    ChangeHeap(v.begin(), v.size(), 0, less);
    EXPECT_TRUE(IsMaxHeap(v, v.size()));
}

TEST(AlgorithmHeap, RemoveHeap)
{
    gx::Vector<int> v = {9, 6, 5, 1, 3, 4, 2, 1};
    MakeHeap(v.begin(), v.size(), less);
    RemoveHeap(v.begin(), v.size(), 2, less);
    // Element removed to position 7, heap is valid in [0,7)
    EXPECT_TRUE(IsMaxHeap(v, v.size() - 1));
}

// ============================================================================
// BinarySearchI
// ============================================================================

TEST(AlgorithmBinarySearch, FoundMiddle)
{
    gx::Vector<int> v = {1, 3, 5, 7, 9, 11, 13};
    auto it = BinarySearchI(v.begin(), v.end(), 7);
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 7);
}

TEST(AlgorithmBinarySearch, FoundFirst)
{
    gx::Vector<int> v = {1, 3, 5, 7, 9};
    auto it = BinarySearchI(v.begin(), v.end(), 1);
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 1);
}

TEST(AlgorithmBinarySearch, FoundLast)
{
    gx::Vector<int> v = {1, 3, 5, 7, 9};
    auto it = BinarySearchI(v.begin(), v.end(), 9);
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 9);
}

TEST(AlgorithmBinarySearch, NotFound)
{
    gx::Vector<int> v = {1, 3, 5, 7, 9};
    auto it = BinarySearchI(v.begin(), v.end(), 4);
    EXPECT_EQ(it, v.end());
}

TEST(AlgorithmBinarySearch, EmptyRange)
{
    gx::Vector<int> v;
    auto it = BinarySearchI(v.begin(), v.end(), 1);
    EXPECT_EQ(it, v.end());
}

// ============================================================================
// LowerBound / UpperBound
// ============================================================================

TEST(AlgorithmBounds, LowerBoundExact)
{
    gx::Vector<int> v = {1, 3, 3, 5, 7};
    auto it = LowerBound(v.begin(), v.end(), 3);
    EXPECT_EQ(it - v.begin(), 1); // first 3
}

TEST(AlgorithmBounds, LowerBoundNotPresent)
{
    gx::Vector<int> v = {1, 3, 5, 7};
    auto it = LowerBound(v.begin(), v.end(), 4);
    EXPECT_EQ(it - v.begin(), 2); // points to 5
}

TEST(AlgorithmBounds, UpperBoundExact)
{
    gx::Vector<int> v = {1, 3, 3, 5, 7};
    auto it = UpperBound(v.begin(), v.end(), 3);
    EXPECT_EQ(it - v.begin(), 3); // past last 3
}

TEST(AlgorithmBounds, UpperBoundNotPresent)
{
    gx::Vector<int> v = {1, 3, 5, 7};
    auto it = UpperBound(v.begin(), v.end(), 4);
    EXPECT_EQ(it - v.begin(), 2); // points to 5
}

TEST(AlgorithmBounds, LowerBoundBeginning)
{
    gx::Vector<int> v = {5, 10, 15};
    auto it = LowerBound(v.begin(), v.end(), 0);
    EXPECT_EQ(it, v.begin());
}

TEST(AlgorithmBounds, UpperBoundEnd)
{
    gx::Vector<int> v = {5, 10, 15};
    auto it = UpperBound(v.begin(), v.end(), 20);
    EXPECT_EQ(it, v.end());
}

// ============================================================================
// Find / FindIf
// ============================================================================

TEST(AlgorithmFind, FindPresent)
{
    gx::Vector<int> v = {10, 20, 30, 40, 50};
    auto it = Find(v.begin(), v.end(), 30);
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 30);
}

TEST(AlgorithmFind, FindAbsent)
{
    gx::Vector<int> v = {10, 20, 30};
    auto it = Find(v.begin(), v.end(), 99);
    EXPECT_EQ(it, v.end());
}

TEST(AlgorithmFind, FindIfOdd)
{
    gx::Vector<int> v = {2, 4, 5, 6, 8};
    auto it = FindIf(v.begin(), v.end(), [](int x) { return x % 2 != 0; });
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 5);
}

TEST(AlgorithmFind, FindIfNoneMatch)
{
    gx::Vector<int> v = {2, 4, 6, 8};
    auto it = FindIf(v.begin(), v.end(), [](int x) { return x > 100; });
    EXPECT_EQ(it, v.end());
}

// ============================================================================
// Count / CountIf
// ============================================================================

TEST(AlgorithmCount, CountValue)
{
    gx::Vector<int> v = {1, 2, 3, 2, 1, 2};
    EXPECT_EQ(Count(v.begin(), v.end(), 2), 3u);
}

TEST(AlgorithmCount, CountValueZero)
{
    gx::Vector<int> v = {1, 2, 3};
    EXPECT_EQ(Count(v.begin(), v.end(), 99), 0u);
}

TEST(AlgorithmCount, CountIfEven)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(CountIf(v.begin(), v.end(), [](int x) { return x % 2 == 0; }), 3u);
}

// ============================================================================
// ForEach
// ============================================================================

TEST(AlgorithmForEach, SumElements)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5};
    int sum = 0;
    ForEach(v.begin(), v.end(), [&sum](int x) { sum += x; });
    EXPECT_EQ(sum, 15);
}

// ============================================================================
// Accumulate
// ============================================================================

TEST(AlgorithmAccumulate, SumDefault)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(Accumulate(v.begin(), v.end(), 0), 15);
}

TEST(AlgorithmAccumulate, ProductCustomOp)
{
    gx::Vector<int> v = {1, 2, 3, 4};
    int result = Accumulate(v.begin(), v.end(), 1, [](int a, int b) { return a * b; });
    EXPECT_EQ(result, 24);
}

TEST(AlgorithmAccumulate, EmptyRange)
{
    gx::Vector<int> v;
    EXPECT_EQ(Accumulate(v.begin(), v.end(), 42), 42);
}

// ============================================================================
// Partition
// ============================================================================

TEST(AlgorithmPartition, SplitEvenOdd)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
    auto mid = Partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    // All even elements before mid
    for (auto it = v.begin(); it != mid; ++it)
        EXPECT_EQ(*it % 2, 0);
    // All odd elements from mid onward
    for (auto it = mid; it != v.end(); ++it)
        EXPECT_NE(*it % 2, 0);
}

TEST(AlgorithmPartition, AllTrue)
{
    gx::Vector<int> v = {2, 4, 6};
    auto mid = Partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.end());
}

TEST(AlgorithmPartition, AllFalse)
{
    gx::Vector<int> v = {1, 3, 5};
    auto mid = Partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.begin());
}

// ============================================================================
// Rotate
// ============================================================================

TEST(AlgorithmRotate, RotateLeft)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5};
    Rotate(v.begin(), v.begin() + 2, v.end());
    gx::Vector<int> expected = {3, 4, 5, 1, 2};
    EXPECT_EQ(v, expected);
}

TEST(AlgorithmRotate, RotateAtBegin)
{
    gx::Vector<int> v = {1, 2, 3};
    Rotate(v.begin(), v.begin(), v.end());
    gx::Vector<int> expected = {1, 2, 3};
    EXPECT_EQ(v, expected);
}

TEST(AlgorithmRotate, RotateAtEnd)
{
    gx::Vector<int> v = {1, 2, 3};
    Rotate(v.begin(), v.end(), v.end());
    gx::Vector<int> expected = {1, 2, 3};
    EXPECT_EQ(v, expected);
}

// ============================================================================
// IsSorted
// ============================================================================

TEST(AlgorithmIsSorted, SortedRange)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmIsSorted, UnsortedRange)
{
    gx::Vector<int> v = {1, 3, 2, 4};
    EXPECT_FALSE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmIsSorted, EmptyRange)
{
    gx::Vector<int> v;
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmIsSorted, SingleElement)
{
    gx::Vector<int> v = {42};
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

TEST(AlgorithmIsSorted, EqualElements)
{
    gx::Vector<int> v = {5, 5, 5, 5};
    EXPECT_TRUE(IsSorted(v.begin(), v.end()));
}

// ============================================================================
// Remove / RemoveIf
// ============================================================================

TEST(AlgorithmRemove, RemoveValue)
{
    gx::Vector<int> v = {1, 2, 3, 2, 4, 2, 5};
    auto newEnd = Remove(v.begin(), v.end(), 2);
    size_t newSize = static_cast<size_t>(newEnd - v.begin());
    EXPECT_EQ(newSize, 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
    EXPECT_EQ(v[3], 5);
}

TEST(AlgorithmRemove, RemoveNone)
{
    gx::Vector<int> v = {1, 2, 3};
    auto newEnd = Remove(v.begin(), v.end(), 99);
    EXPECT_EQ(newEnd, v.end());
}

TEST(AlgorithmRemove, RemoveIfOdd)
{
    gx::Vector<int> v = {1, 2, 3, 4, 5, 6};
    auto newEnd = RemoveIf(v.begin(), v.end(), [](int x) { return x % 2 != 0; });
    size_t newSize = static_cast<size_t>(newEnd - v.begin());
    EXPECT_EQ(newSize, 3u);
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[2], 6);
}

// ============================================================================
// Unique
// ============================================================================

TEST(AlgorithmUnique, RemoveDuplicates)
{
    gx::Vector<int> v = {1, 1, 2, 2, 2, 3, 3, 4};
    auto newEnd = Unique(v.begin(), v.end());
    size_t newSize = static_cast<size_t>(newEnd - v.begin());
    EXPECT_EQ(newSize, 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
}

TEST(AlgorithmUnique, NoDuplicates)
{
    gx::Vector<int> v = {1, 2, 3, 4};
    auto newEnd = Unique(v.begin(), v.end());
    EXPECT_EQ(newEnd, v.end());
}

TEST(AlgorithmUnique, EmptyRange)
{
    gx::Vector<int> v;
    auto newEnd = Unique(v.begin(), v.end());
    EXPECT_EQ(newEnd, v.end());
}

TEST(AlgorithmUnique, AllSame)
{
    gx::Vector<int> v = {5, 5, 5, 5, 5};
    auto newEnd = Unique(v.begin(), v.end());
    size_t newSize = static_cast<size_t>(newEnd - v.begin());
    EXPECT_EQ(newSize, 1u);
    EXPECT_EQ(v[0], 5);
}

TEST(AlgorithmUnique, CustomPredicate)
{
    gx::Vector<int> v = {1, 2, 2, 3, 3, 3, 4};
    auto newEnd = Unique(v.begin(), v.end(), [](int a, int b) { return a == b; });
    size_t newSize = static_cast<size_t>(newEnd - v.begin());
    EXPECT_EQ(newSize, 4u);
}
