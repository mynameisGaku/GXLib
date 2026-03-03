/// @file test_ContainerVector.cpp
/// @brief Container Vector tests: construction, assignment, access, capacity, modifiers, comparison

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Vector.h"

#include <string>
#include <vector>
#include <utility>

using namespace gx::container;

// Helper: non-trivially-relocatable type for testing non-memcpy paths
struct NTR {
    gx::String data;
    NTR() = default;
    NTR(const char* s) : data(s) {}
    NTR(const NTR& o) : data(o.data) {}
    NTR(NTR&& o) noexcept : data(std::move(o.data)) {}
    NTR& operator=(const NTR& o) { data = o.data; return *this; }
    NTR& operator=(NTR&& o) noexcept { data = std::move(o.data); return *this; }
    ~NTR() = default;
    bool operator==(const NTR& o) const { return data == o.data; }
    bool operator<(const NTR& o) const { return data < o.data; }
};

// ============================================================================
// Construction
// ============================================================================

TEST(VectorConstruction, Default)
{
    Vector<int> v;
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.Data(), nullptr);
}

TEST(VectorConstruction, WithAllocator)
{
    Allocator alloc("test");
    Vector<int> v(alloc);
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
}

TEST(VectorConstruction, SizeN)
{
    Vector<int> v(5);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_GE(v.capacity(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(v[i], 0);
}

TEST(VectorConstruction, SizeNLarge)
{
    Vector<int> v(1000);
    EXPECT_EQ(v.size(), 1000u);
}

TEST(VectorConstruction, SizeZero)
{
    Vector<int> v(size_t(0));
    EXPECT_EQ(v.size(), 0u);
}

TEST(VectorConstruction, SizeNWithValue)
{
    Vector<int> v(5, 42);
    EXPECT_EQ(v.size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(v[i], 42);
}

TEST(VectorConstruction, FromRange)
{
    int arr[] = {10, 20, 30, 40, 50};
    Vector<int> v(arr, arr + 5);
    EXPECT_EQ(v.size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(v[i], arr[i]);
}

TEST(VectorConstruction, FromEmptyRange)
{
    int arr[] = {1};
    Vector<int> v(arr, arr);
    EXPECT_EQ(v.size(), 0u);
}

TEST(VectorConstruction, InitializerList)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[4], 5);
}

TEST(VectorConstruction, EmptyInitializerList)
{
    Vector<int> v = {};
    EXPECT_EQ(v.size(), 0u);
}

TEST(VectorConstruction, CopyConstructor)
{
    Vector<int> v1 = {1, 2, 3};
    Vector<int> v2(v1);
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[1], 2);
    EXPECT_EQ(v2[2], 3);
    // Original unchanged
    EXPECT_EQ(v1.size(), 3u);
}

TEST(VectorConstruction, CopyConstructorEmpty)
{
    Vector<int> v1;
    Vector<int> v2(v1);
    EXPECT_EQ(v2.size(), 0u);
}

TEST(VectorConstruction, MoveConstructor)
{
    Vector<int> v1 = {1, 2, 3};
    Vector<int> v2(std::move(v1));
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v1.size(), 0u);
    EXPECT_EQ(v1.Data(), nullptr);
}

TEST(VectorConstruction, MoveConstructorIsNoexcept)
{
    static_assert(std::is_nothrow_move_constructible_v<Vector<int>>);
}

TEST(VectorConstruction, NonTrivialType)
{
    Vector<NTR> v(3, NTR("abc"));
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0].data, "abc");
    EXPECT_EQ(v[2].data, "abc");
}

TEST(VectorConstruction, CopyConstructorWithAllocator)
{
    Allocator alloc("custom");
    Vector<int> v1 = {10, 20, 30};
    Vector<int> v2(v1, alloc);
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v2[1], 20);
}

TEST(VectorConstruction, MoveConstructorWithAllocator)
{
    Allocator alloc("custom");
    Vector<int> v1 = {10, 20, 30};
    Vector<int> v2(std::move(v1), alloc);
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v2[0], 10);
}

// ============================================================================
// Assignment
// ============================================================================

TEST(VectorAssignment, CopyAssignment)
{
    Vector<int> v1 = {1, 2, 3};
    Vector<int> v2;
    v2 = v1;
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v2[0], 1);
}

TEST(VectorAssignment, CopyAssignmentSelf)
{
    Vector<int> v = {1, 2, 3};
    v = v;
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
}

TEST(VectorAssignment, MoveAssignment)
{
    Vector<int> v1 = {1, 2, 3};
    Vector<int> v2;
    v2 = std::move(v1);
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v1.size(), 0u);
}

TEST(VectorAssignment, MoveAssignmentIsNoexcept)
{
    static_assert(std::is_nothrow_move_assignable_v<Vector<int>>);
}

TEST(VectorAssignment, InitializerListAssignment)
{
    Vector<int> v;
    v = {10, 20, 30};
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 10);
}

TEST(VectorAssignment, AssignNValue)
{
    Vector<int> v;
    v.assign(5, 99);
    EXPECT_EQ(v.size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(v[i], 99);
}

TEST(VectorAssignment, AssignRange)
{
    int arr[] = {7, 8, 9};
    Vector<int> v;
    v.assign(arr, arr + 3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 7);
}

TEST(VectorAssignment, AssignInitializerList)
{
    Vector<int> v;
    v.assign({100, 200});
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 100);
    EXPECT_EQ(v[1], 200);
}

TEST(VectorAssignment, AssignOverwritesExisting)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    v.assign(2, 42);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 42);
    EXPECT_EQ(v[1], 42);
}

// ============================================================================
// Element access
// ============================================================================

TEST(VectorAccess, OperatorBracket)
{
    Vector<int> v = {10, 20, 30};
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
}

TEST(VectorAccess, OperatorBracketConst)
{
    const Vector<int> v = {5, 10, 15};
    EXPECT_EQ(v[0], 5);
    EXPECT_EQ(v[2], 15);
}

TEST(VectorAccess, OperatorBracketModify)
{
    Vector<int> v = {1, 2, 3};
    v[1] = 99;
    EXPECT_EQ(v[1], 99);
}

TEST(VectorAccess, Front)
{
    Vector<int> v = {42, 1, 2};
    EXPECT_EQ(v.Front(), 42);
}

TEST(VectorAccess, FrontConst)
{
    const Vector<int> v = {42, 1, 2};
    EXPECT_EQ(v.Front(), 42);
}

TEST(VectorAccess, Back)
{
    Vector<int> v = {1, 2, 99};
    EXPECT_EQ(v.Back(), 99);
}

TEST(VectorAccess, BackConst)
{
    const Vector<int> v = {1, 2, 99};
    EXPECT_EQ(v.Back(), 99);
}

TEST(VectorAccess, Data)
{
    Vector<int> v = {1, 2, 3};
    int* p = v.Data();
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[2], 3);
}

TEST(VectorAccess, DataConst)
{
    const Vector<int> v = {1, 2, 3};
    const int* p = v.Data();
    EXPECT_EQ(p[1], 2);
}

TEST(VectorAccess, DataNullWhenEmpty)
{
    Vector<int> v;
    EXPECT_EQ(v.Data(), nullptr);
}

// ============================================================================
// Iterators
// ============================================================================

TEST(VectorIterators, BeginEnd)
{
    Vector<int> v = {1, 2, 3};
    int sum = 0;
    for (auto it = v.begin(); it != v.end(); ++it)
        sum += *it;
    EXPECT_EQ(sum, 6);
}

TEST(VectorIterators, CBeginCEnd)
{
    const Vector<int> v = {1, 2, 3};
    int sum = 0;
    for (auto it = v.cbegin(); it != v.cend(); ++it)
        sum += *it;
    EXPECT_EQ(sum, 6);
}

TEST(VectorIterators, RangeForLoop)
{
    Vector<int> v = {10, 20, 30};
    int sum = 0;
    for (int x : v)
        sum += x;
    EXPECT_EQ(sum, 60);
}

// ============================================================================
// Capacity
// ============================================================================

TEST(VectorCapacity, Size)
{
    Vector<int> v = {1, 2, 3, 4};
    EXPECT_EQ(v.size(), 4u);
}

TEST(VectorCapacity, Capacity)
{
    Vector<int> v = {1, 2, 3};
    EXPECT_GE(v.capacity(), 3u);
}

TEST(VectorCapacity, Empty)
{
    Vector<int> v;
    EXPECT_TRUE(v.empty());
    v.push_back(1);
    EXPECT_FALSE(v.empty());
}

TEST(VectorCapacity, Reserve)
{
    Vector<int> v;
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100u);
    EXPECT_EQ(v.size(), 0u);
}

TEST(VectorCapacity, ReserveSmallerDoesNotShrink)
{
    Vector<int> v;
    v.reserve(100);
    size_t cap = v.capacity();
    v.reserve(50);
    EXPECT_EQ(v.capacity(), cap);
}

TEST(VectorCapacity, SetCapacity)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    v.set_capacity(10);
    EXPECT_EQ(v.capacity(), 10u);
    EXPECT_EQ(v.size(), 5u);
}

TEST(VectorCapacity, SetCapacityTruncates)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    v.set_capacity(3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v.capacity(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[2], 3);
}

TEST(VectorCapacity, SetCapacityZero)
{
    Vector<int> v = {1, 2, 3};
    v.set_capacity(0);
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.Data(), nullptr);
}

TEST(VectorCapacity, ShrinkToFit)
{
    Vector<int> v;
    v.reserve(100);
    v.push_back(1);
    v.push_back(2);
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), 2u);
    EXPECT_EQ(v.size(), 2u);
}

TEST(VectorCapacity, ShrinkToFitEmpty)
{
    Vector<int> v;
    v.reserve(50);
    v.shrink_to_fit();
    // When size is 0, set_capacity(0) makes everything null
    EXPECT_EQ(v.size(), 0u);
}

// ============================================================================
// Modifiers - push/emplace/pop
// ============================================================================

TEST(VectorModifiers, PushBackCopy)
{
    Vector<int> v;
    int val = 42;
    v.push_back(val);
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 42);
}

TEST(VectorModifiers, PushBackMove)
{
    Vector<NTR> v;
    NTR val("test");
    v.push_back(std::move(val));
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0].data, "test");
}

TEST(VectorModifiers, PushBackDefault)
{
    Vector<int> v;
    v.push_back();
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 0);
}

TEST(VectorModifiers, PushBackGrowth)
{
    Vector<int> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(i);
    EXPECT_EQ(v.size(), 100u);
    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(v[i], i);
}

TEST(VectorModifiers, EmplaceBack)
{
    Vector<NTR> v;
    auto& ref = v.emplace_back("hello");
    EXPECT_EQ(ref.data, "hello");
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0].data, "hello");
}

TEST(VectorModifiers, EmplaceBackReturnsReference)
{
    Vector<int> v;
    int& ref = v.emplace_back(42);
    EXPECT_EQ(ref, 42);
    EXPECT_EQ(&ref, &v[0]);
}

TEST(VectorModifiers, EmplaceBackMultiple)
{
    Vector<int> v;
    for (int i = 0; i < 50; ++i)
        v.emplace_back(i * 2);
    EXPECT_EQ(v.size(), 50u);
    EXPECT_EQ(v[25], 50);
}

TEST(VectorModifiers, PopBack)
{
    Vector<int> v = {1, 2, 3};
    v.pop_back();
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.Back(), 2);
}

TEST(VectorModifiers, PopBackNonTrivial)
{
    Vector<NTR> v;
    v.push_back(NTR("a"));
    v.push_back(NTR("b"));
    v.pop_back();
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0].data, "a");
}

TEST(VectorModifiers, PopBackToEmpty)
{
    Vector<int> v = {42};
    v.pop_back();
    EXPECT_TRUE(v.empty());
}

// ============================================================================
// Modifiers - insert
// ============================================================================

TEST(VectorModifiers, InsertAtBeginning)
{
    Vector<int> v = {2, 3, 4};
    v.insert(v.begin(), 1);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(VectorModifiers, InsertAtEnd)
{
    Vector<int> v = {1, 2, 3};
    v.insert(v.end(), 4);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[3], 4);
}

TEST(VectorModifiers, InsertInMiddle)
{
    Vector<int> v = {1, 3, 4};
    v.insert(v.begin() + 1, 2);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
}

TEST(VectorModifiers, InsertMoveValue)
{
    Vector<NTR> v;
    v.push_back(NTR("a"));
    v.push_back(NTR("c"));
    NTR b("b");
    v.insert(v.begin() + 1, std::move(b));
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0].data, "a");
    EXPECT_EQ(v[1].data, "b");
    EXPECT_EQ(v[2].data, "c");
}

TEST(VectorModifiers, InsertReturnsIterator)
{
    Vector<int> v = {1, 3};
    auto it = v.insert(v.begin() + 1, 2);
    EXPECT_EQ(*it, 2);
}

TEST(VectorModifiers, InsertTriggerGrow)
{
    Vector<int> v;
    v.reserve(3);
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    // Capacity is full, insert should trigger reallocation
    v.insert(v.begin(), 0);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 0);
}

// ============================================================================
// Modifiers - erase
// ============================================================================

TEST(VectorModifiers, EraseSingle)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    auto it = v.erase(v.begin() + 2);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(*it, 4);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 4);
    EXPECT_EQ(v[3], 5);
}

TEST(VectorModifiers, EraseFirst)
{
    Vector<int> v = {1, 2, 3};
    v.erase(v.begin());
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 2);
}

TEST(VectorModifiers, EraseLast)
{
    Vector<int> v = {1, 2, 3};
    v.erase(v.end() - 1);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.Back(), 2);
}

TEST(VectorModifiers, EraseRange)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    v.erase(v.begin() + 1, v.begin() + 4);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 5);
}

TEST(VectorModifiers, EraseRangeEmpty)
{
    Vector<int> v = {1, 2, 3};
    auto it = v.erase(v.begin() + 1, v.begin() + 1);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(*it, 2);
}

TEST(VectorModifiers, EraseRangeFull)
{
    Vector<int> v = {1, 2, 3};
    v.erase(v.begin(), v.end());
    EXPECT_EQ(v.size(), 0u);
}

TEST(VectorModifiers, EraseNonTrivial)
{
    Vector<NTR> v;
    v.push_back(NTR("a"));
    v.push_back(NTR("b"));
    v.push_back(NTR("c"));
    v.erase(v.begin() + 1);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].data, "a");
    EXPECT_EQ(v[1].data, "c");
}

// ============================================================================
// Modifiers - resize / clear
// ============================================================================

TEST(VectorModifiers, ResizeGrow)
{
    Vector<int> v = {1, 2, 3};
    v.resize(6);
    EXPECT_EQ(v.size(), 6u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[3], 0); // default initialized
}

TEST(VectorModifiers, ResizeShrink)
{
    Vector<int> v = {1, 2, 3, 4, 5};
    v.resize(2);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(VectorModifiers, ResizeToZero)
{
    Vector<int> v = {1, 2, 3};
    v.resize(0);
    EXPECT_TRUE(v.empty());
}

TEST(VectorModifiers, ResizeWithValue)
{
    Vector<int> v = {1, 2};
    v.resize(5, 42);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 42);
    EXPECT_EQ(v[4], 42);
}

TEST(VectorModifiers, ResizeSameSize)
{
    Vector<int> v = {1, 2, 3};
    v.resize(3);
    EXPECT_EQ(v.size(), 3u);
}

TEST(VectorModifiers, Clear)
{
    Vector<int> v = {1, 2, 3};
    size_t cap = v.capacity();
    v.clear();
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.capacity(), cap); // capacity preserved
}

TEST(VectorModifiers, ClearEmpty)
{
    Vector<int> v;
    v.clear(); // Should not crash
    EXPECT_TRUE(v.empty());
}

TEST(VectorModifiers, ClearNonTrivial)
{
    Vector<NTR> v;
    v.push_back(NTR("a"));
    v.push_back(NTR("b"));
    v.clear();
    EXPECT_TRUE(v.empty());
}

TEST(VectorModifiers, ResetLoseMemory)
{
    Vector<int> v = {1, 2, 3};
    v.reset_lose_memory();
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.Data(), nullptr);
    // Note: memory is leaked by design (arena allocator pattern)
}

// ============================================================================
// Comparison
// ============================================================================

TEST(VectorComparison, EqualSame)
{
    Vector<int> a = {1, 2, 3};
    Vector<int> b = {1, 2, 3};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(VectorComparison, EqualEmpty)
{
    Vector<int> a;
    Vector<int> b;
    EXPECT_TRUE(a == b);
}

TEST(VectorComparison, NotEqualSize)
{
    Vector<int> a = {1, 2, 3};
    Vector<int> b = {1, 2};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(VectorComparison, NotEqualContent)
{
    Vector<int> a = {1, 2, 3};
    Vector<int> b = {1, 2, 4};
    EXPECT_FALSE(a == b);
}

TEST(VectorComparison, LessThan)
{
    Vector<int> a = {1, 2, 3};
    Vector<int> b = {1, 2, 4};
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(VectorComparison, LessThanShorter)
{
    Vector<int> a = {1, 2};
    Vector<int> b = {1, 2, 3};
    EXPECT_TRUE(a < b);
}

TEST(VectorComparison, GreaterThan)
{
    Vector<int> a = {1, 3};
    Vector<int> b = {1, 2};
    EXPECT_TRUE(a > b);
}

TEST(VectorComparison, LessOrEqual)
{
    Vector<int> a = {1, 2};
    EXPECT_TRUE(a <= a);
}

TEST(VectorComparison, GreaterOrEqual)
{
    Vector<int> a = {1, 2};
    EXPECT_TRUE(a >= a);
}

// ============================================================================
// Validate
// ============================================================================

TEST(VectorValidation, ValidateEmpty)
{
    Vector<int> v;
    EXPECT_TRUE(v.Validate());
}

TEST(VectorValidation, ValidateAfterPushBack)
{
    Vector<int> v;
    for (int i = 0; i < 10; ++i)
        v.push_back(i);
    EXPECT_TRUE(v.Validate());
}

TEST(VectorValidation, ValidateAfterReserve)
{
    Vector<int> v;
    v.reserve(100);
    EXPECT_TRUE(v.Validate());
}

TEST(VectorValidation, ValidateAfterShrink)
{
    Vector<int> v = {1, 2, 3};
    v.shrink_to_fit();
    EXPECT_TRUE(v.Validate());
}

TEST(VectorValidation, ValidateAfterClear)
{
    Vector<int> v = {1, 2, 3};
    v.clear();
    EXPECT_TRUE(v.Validate());
}

// ============================================================================
// ValidateIterator
// ============================================================================

TEST(VectorValidation, ValidateIteratorValid)
{
    Vector<int> v = {1, 2, 3};
    EXPECT_EQ(v.ValidateIterator(v.begin()), IteratorValidity::Valid);
    EXPECT_EQ(v.ValidateIterator(v.end()), IteratorValidity::Valid);
    EXPECT_EQ(v.ValidateIterator(v.begin() + 1), IteratorValidity::Valid);
}

TEST(VectorValidation, ValidateIteratorNotOwned)
{
    Vector<int> v1 = {1, 2, 3};
    Vector<int> v2 = {4, 5, 6};
    EXPECT_EQ(v1.ValidateIterator(v2.begin()), IteratorValidity::NotOwned);
}

// ============================================================================
// Move semantics verification
// ============================================================================

TEST(VectorMoveSemantics, MoveConstructorStealsMemory)
{
    Vector<int> v1 = {1, 2, 3};
    int* data = v1.Data();
    Vector<int> v2(std::move(v1));
    EXPECT_EQ(v2.Data(), data);
    EXPECT_EQ(v1.Data(), nullptr);
}

TEST(VectorMoveSemantics, MoveAssignmentStealsMemory)
{
    Vector<int> v1 = {1, 2, 3};
    int* data = v1.Data();
    Vector<int> v2;
    v2 = std::move(v1);
    EXPECT_EQ(v2.Data(), data);
}

// ============================================================================
// Swap
// ============================================================================

TEST(VectorSwap, ADLSwap)
{
    Vector<int> a = {1, 2, 3};
    Vector<int> b = {4, 5};
    swap(a, b);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(b[0], 1);
}

// ============================================================================
// Allocator access
// ============================================================================

TEST(VectorAllocator, GetAllocator)
{
    Vector<int> v;
    [[maybe_unused]] auto& alloc = v.GetAllocator();
}

TEST(VectorAllocator, GetAllocatorConst)
{
    const Vector<int> v = {1, 2, 3};
    [[maybe_unused]] const auto& alloc = v.GetAllocator();
}

// ============================================================================
// Trivially relocatable optimization
// ============================================================================

TEST(VectorTrivialRelocate, TrivialTypeOptimized)
{
    EXPECT_TRUE(is_trivially_relocatable_v<int>);
    // Verify Vector of trivially-relocatable works correctly after grow
    Vector<int> v;
    for (int i = 0; i < 1000; ++i)
        v.push_back(i);
    EXPECT_EQ(v.size(), 1000u);
    for (int i = 0; i < 1000; ++i)
        EXPECT_EQ(v[i], i);
}

TEST(VectorTrivialRelocate, NonTrivialTypeWorks)
{
    EXPECT_FALSE(is_trivially_relocatable_v<NTR>);
    Vector<NTR> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(NTR(std::to_string(i).c_str()));
    EXPECT_EQ(v.size(), 100u);
    EXPECT_EQ(v[50].data, "50");
}

// ============================================================================
// Growth behavior
// ============================================================================

TEST(VectorGrowth, DoubleGrowthPolicy)
{
    Vector<int> v;
    v.push_back(1);
    // First allocation should be at least 8 (minimum capacity from GetNewCapacity)
    EXPECT_GE(v.capacity(), 8u);

    size_t cap = v.capacity();
    // Fill to capacity
    while (v.size() < cap)
        v.push_back(0);
    v.push_back(0); // triggers grow
    // New capacity should be at least 2x old
    EXPECT_GE(v.capacity(), cap * 2);
}

// ============================================================================
// Complex operations
// ============================================================================

TEST(VectorComplex, InsertEraseSequence)
{
    Vector<int> v;
    for (int i = 0; i < 10; ++i)
        v.push_back(i);

    v.erase(v.begin() + 3);
    v.insert(v.begin() + 5, 99);
    EXPECT_TRUE(v.Validate());
    EXPECT_EQ(v.size(), 10u);
}

TEST(VectorComplex, ClearAndReuse)
{
    Vector<int> v;
    for (int i = 0; i < 50; ++i)
        v.push_back(i);
    v.clear();
    for (int i = 100; i < 150; ++i)
        v.push_back(i);
    EXPECT_EQ(v.size(), 50u);
    EXPECT_EQ(v[0], 100);
}

TEST(VectorComplex, NestedVectors)
{
    Vector<Vector<int>> outer;
    for (int i = 0; i < 5; ++i)
    {
        Vector<int> inner;
        for (int j = 0; j < 3; ++j)
            inner.push_back(i * 10 + j);
        outer.push_back(std::move(inner));
    }
    EXPECT_EQ(outer.size(), 5u);
    EXPECT_EQ(outer[2][1], 21);
}
