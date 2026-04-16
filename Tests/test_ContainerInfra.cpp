/// @file test_ContainerInfra.cpp
/// @brief Container infrastructure tests: Allocator, TypeTraits, CompressedPair, Iterator, Hash

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Allocator.h"
#include "Container/TypeTraits.h"
#include "Container/CompressedPair.h"
#include "Container/Iterator.h"
#include "Container/Hash.h"

#include <string>
#include <cstring>
#include <vector>

using namespace gx::container;

// ============================================================================
// Allocator tests
// ============================================================================

TEST(Allocator, DefaultConstructor)
{
    Allocator alloc;
    EXPECT_STREQ(alloc.GetName(), "gx::container::Allocator");
}

TEST(Allocator, NamedConstructor)
{
    Allocator alloc("TestAllocator");
    EXPECT_STREQ(alloc.GetName(), "TestAllocator");
}

TEST(Allocator, SetName)
{
    Allocator alloc;
    alloc.SetName("NewName");
    EXPECT_STREQ(alloc.GetName(), "NewName");
}

TEST(Allocator, AllocateAndDeallocate)
{
    Allocator alloc;
    void* p = alloc.allocate(128);
    EXPECT_NE(p, nullptr);
    alloc.deallocate(p, 128);
}

TEST(Allocator, AllocateZeroBytes)
{
    Allocator alloc;
    // Allocating zero bytes should still return a valid pointer (or nullptr)
    void* p = alloc.allocate(0);
    // Just deallocate whatever was returned
    alloc.deallocate(p, 0);
}

TEST(Allocator, AllocateWithAlignment)
{
    Allocator alloc;
    void* p = alloc.allocate(256, 64, 0, 0);
    EXPECT_NE(p, nullptr);
    alloc.deallocate(p, 256);
}

TEST(Allocator, AllocateLargeBlock)
{
    Allocator alloc;
    void* p = alloc.allocate(1024 * 1024);
    EXPECT_NE(p, nullptr);
    // Write to the memory to verify it's usable
    std::memset(p, 0xAB, 1024 * 1024);
    alloc.deallocate(p, 1024 * 1024);
}

TEST(Allocator, MultipleAllocations)
{
    Allocator alloc;
    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(128);
    void* p3 = alloc.allocate(256);
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p3, nullptr);
    EXPECT_NE(p1, p2);
    EXPECT_NE(p2, p3);
    alloc.deallocate(p3, 256);
    alloc.deallocate(p2, 128);
    alloc.deallocate(p1, 64);
}

TEST(Allocator, CopyConstructor)
{
    Allocator alloc("Original");
    Allocator copy(alloc);
    EXPECT_STREQ(copy.GetName(), "Original");
}

TEST(Allocator, CopyAssignment)
{
    Allocator alloc("Original");
    Allocator other;
    other = alloc;
    EXPECT_STREQ(other.GetName(), "Original");
}

TEST(Allocator, EqualitySameInstance)
{
    Allocator alloc;
    EXPECT_TRUE(alloc == alloc);
    EXPECT_FALSE(alloc != alloc);
}

TEST(Allocator, EqualityDifferentInstances)
{
    Allocator a("A");
    Allocator b("B");
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(Allocator, GetDefaultAllocator)
{
    Allocator* def = GetDefaultAllocator();
    EXPECT_NE(def, nullptr);
}

TEST(Allocator, SetDefaultAllocator)
{
    Allocator* original = GetDefaultAllocator();
    Allocator custom("Custom");
    SetDefaultAllocator(&custom);
    EXPECT_EQ(GetDefaultAllocator(), &custom);
    // Restore
    SetDefaultAllocator(original);
    EXPECT_EQ(GetDefaultAllocator(), original);
}

// ============================================================================
// FixedBufferAllocator tests
// ============================================================================

TEST(FixedBufferAllocator, DefaultConstructor)
{
    FixedBufferAllocator alloc;
    EXPECT_STREQ(alloc.GetName(), "FixedBufferAllocator");
}

TEST(FixedBufferAllocator, ConstructWithBuffer)
{
    alignas(16) char buffer[256];
    FixedBufferAllocator alloc(buffer, sizeof(buffer));
    EXPECT_STREQ(alloc.GetName(), "FixedBufferAllocator");
}

TEST(FixedBufferAllocator, AllocateFromBuffer)
{
    alignas(16) char buffer[256];
    FixedBufferAllocator alloc(buffer, sizeof(buffer));
    void* p = alloc.allocate(64);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(alloc.OwnsMemory(p));
}

TEST(FixedBufferAllocator, Init)
{
    alignas(16) char buffer[128];
    FixedBufferAllocator alloc;
    alloc.Init(buffer, sizeof(buffer));
    void* p = alloc.allocate(32);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(alloc.OwnsMemory(p));
}

TEST(FixedBufferAllocator, OwnsMemoryOutsideBuffer)
{
    alignas(16) char buffer[128];
    FixedBufferAllocator alloc(buffer, sizeof(buffer));
    int stack_var = 42;
    EXPECT_FALSE(alloc.OwnsMemory(&stack_var));
}

TEST(FixedBufferAllocator, MultipleSmallAllocations)
{
    alignas(16) char buffer[512];
    FixedBufferAllocator alloc(buffer, sizeof(buffer));
    void* p1 = alloc.allocate(32);
    void* p2 = alloc.allocate(32);
    void* p3 = alloc.allocate(32);
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p3, nullptr);
    EXPECT_TRUE(alloc.OwnsMemory(p1));
    EXPECT_TRUE(alloc.OwnsMemory(p2));
    EXPECT_TRUE(alloc.OwnsMemory(p3));
}

TEST(FixedBufferAllocator, AllocateWithAlignment)
{
    alignas(64) char buffer[1024];
    FixedBufferAllocator alloc(buffer, sizeof(buffer));
    void* p = alloc.allocate(128, 64, 0, 0);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(alloc.OwnsMemory(p));
}

TEST(FixedBufferAllocator, SetNameIsNoOp)
{
    FixedBufferAllocator alloc;
    alloc.SetName("AnyName");
    EXPECT_STREQ(alloc.GetName(), "FixedBufferAllocator");
}

TEST(FixedBufferAllocator, DeallocateIsNoOp)
{
    alignas(16) char buffer[128];
    FixedBufferAllocator alloc(buffer, sizeof(buffer));
    void* p = alloc.allocate(64);
    // deallocate should not crash (bump allocator typically ignores deallocate)
    alloc.deallocate(p, 64);
}

// ============================================================================
// TypeTraits tests
// ============================================================================

TEST(TypeTraits, IsTriviallyRelocatableForInt)
{
    EXPECT_TRUE(is_trivially_relocatable_v<int>);
}

TEST(TypeTraits, IsTriviallyRelocatableForFloat)
{
    EXPECT_TRUE(is_trivially_relocatable_v<float>);
}

TEST(TypeTraits, IsTriviallyRelocatableForDouble)
{
    EXPECT_TRUE(is_trivially_relocatable_v<double>);
}

TEST(TypeTraits, IsTriviallyRelocatableForPointer)
{
    EXPECT_TRUE(is_trivially_relocatable_v<int*>);
}

TEST(TypeTraits, IsTriviallyRelocatableForStdString)
{
    // gx::String is NOT trivially copyable
    EXPECT_FALSE(is_trivially_relocatable_v<gx::String>);
}

struct TrivialStruct { int a; float b; };

TEST(TypeTraits, IsTriviallyRelocatableForTrivialStruct)
{
    EXPECT_TRUE(is_trivially_relocatable_v<TrivialStruct>);
}

struct NonTrivialStruct {
    gx::String data;
    NonTrivialStruct() = default;
    NonTrivialStruct(const NonTrivialStruct&) = default;
    NonTrivialStruct& operator=(const NonTrivialStruct&) = default;
    ~NonTrivialStruct() = default;
};

TEST(TypeTraits, IsTriviallyRelocatableForNonTrivial)
{
    EXPECT_FALSE(is_trivially_relocatable_v<NonTrivialStruct>);
}

TEST(TypeTraits, UninitializedRelocateTrivial)
{
    int src[5] = {1, 2, 3, 4, 5};
    int dest[5] = {};
    gx::container::uninitialized_relocate(dest, src, 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(dest[i], i + 1);
}

TEST(TypeTraits, UninitializedRelocateNonTrivial)
{
    // Use a non-trivially-relocatable type
    gx::String src[3];
    src[0] = "hello";
    src[1] = "world";
    src[2] = "test";

    alignas(gx::String) char buffer[sizeof(gx::String) * 3];
    gx::String* dest = reinterpret_cast<gx::String*>(buffer);

    gx::container::uninitialized_relocate(dest, src, 3);
    EXPECT_EQ(dest[0], "hello");
    EXPECT_EQ(dest[1], "world");
    EXPECT_EQ(dest[2], "test");

    // Clean up (they were move-constructed into dest, src was destroyed)
    for (int i = 0; i < 3; ++i)
        dest[i].~String();
}

TEST(TypeTraits, UninitializedCopyTrivial)
{
    int src[4] = {10, 20, 30, 40};
    int dest[4] = {};
    gx::container::uninitialized_copy(dest, src, 4);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(dest[i], src[i]);
}

TEST(TypeTraits, UninitializedCopyNonTrivial)
{
    gx::String src[2] = {"abc", "def"};
    alignas(gx::String) char buffer[sizeof(gx::String) * 2];
    gx::String* dest = reinterpret_cast<gx::String*>(buffer);

    gx::container::uninitialized_copy(dest, src, 2);
    EXPECT_EQ(dest[0], "abc");
    EXPECT_EQ(dest[1], "def");

    for (int i = 0; i < 2; ++i)
        dest[i].~String();
}

TEST(TypeTraits, UninitializedFill)
{
    int arr[5];
    gx::container::uninitialized_fill(arr, 5, 42);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(arr[i], 42);
}

TEST(TypeTraits, UninitializedFillNonTrivial)
{
    alignas(gx::String) char buffer[sizeof(gx::String) * 3];
    gx::String* arr = reinterpret_cast<gx::String*>(buffer);
    gx::String val("fill");

    gx::container::uninitialized_fill(arr, 3, val);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ(arr[i], "fill");

    for (int i = 0; i < 3; ++i)
        arr[i].~String();
}

TEST(TypeTraits, DestroyRangeTrivial)
{
    int arr[5] = {1, 2, 3, 4, 5};
    // For trivially destructible types, this is a no-op
    gx::container::destroy_range(arr, 5);
    // Should not crash; values unchanged since it's a no-op
    EXPECT_EQ(arr[0], 1);
}

static int g_destructorCount = 0;

struct DestructorCounter {
    ~DestructorCounter() { ++g_destructorCount; }
};

TEST(TypeTraits, DestroyRangeNonTrivial)
{
    alignas(DestructorCounter) char buffer[sizeof(DestructorCounter) * 3];
    DestructorCounter* arr = reinterpret_cast<DestructorCounter*>(buffer);

    for (int i = 0; i < 3; ++i)
        new (&arr[i]) DestructorCounter();

    g_destructorCount = 0;
    gx::container::destroy_range(arr, 3);
    EXPECT_EQ(g_destructorCount, 3);
}

TEST(TypeTraits, UninitializedDefaultConstruct)
{
    int arr[4];
    std::memset(arr, 0xFF, sizeof(arr));
    gx::container::uninitialized_default_construct(arr, 4);
    // For trivially default constructible types, memset to 0
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(arr[i], 0);
}

TEST(TypeTraits, UninitializedMoveTrivial)
{
    int src[3] = {100, 200, 300};
    int dest[3] = {};
    gx::container::uninitialized_move(dest, src, 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ(dest[i], src[i]);
}

TEST(TypeTraits, UninitializedRelocateZeroCount)
{
    int src[1] = {42};
    int dest[1] = {0};
    gx::container::uninitialized_relocate(dest, src, 0);
    EXPECT_EQ(dest[0], 0); // unchanged
}

// ============================================================================
// CompressedPair tests
// ============================================================================

struct EmptyClass {};
struct AnotherEmpty {};

TEST(CompressedPair, EBOApplied)
{
    // When first type is empty, sizeof(CompressedPair) should equal sizeof(second)
    CompressedPair<EmptyClass, int> pair;
    EXPECT_EQ(sizeof(pair), sizeof(int));
}

TEST(CompressedPair, EBONotAppliedForNonEmpty)
{
    CompressedPair<int, int> pair;
    // Both are ints, so size should be at least 2*sizeof(int)
    EXPECT_GE(sizeof(pair), 2 * sizeof(int));
}

TEST(CompressedPair, DefaultConstruction)
{
    CompressedPair<EmptyClass, int> pair;
    EXPECT_EQ(pair.Second(), 0);
}

TEST(CompressedPair, ConstructWithValues)
{
    CompressedPair<EmptyClass, int> pair(EmptyClass{}, 42);
    EXPECT_EQ(pair.Second(), 42);
}

TEST(CompressedPair, ConstructWithSecondOnly)
{
    CompressedPair<EmptyClass, int> pair(42);
    EXPECT_EQ(pair.Second(), 42);
}

TEST(CompressedPair, AccessFirst)
{
    CompressedPair<EmptyClass, int> pair;
    // First() should return a reference to the EmptyClass base
    [[maybe_unused]] const EmptyClass& ref = pair.First();
}

TEST(CompressedPair, AccessSecond)
{
    CompressedPair<EmptyClass, int> pair(99);
    EXPECT_EQ(pair.Second(), 99);
    pair.Second() = 100;
    EXPECT_EQ(pair.Second(), 100);
}

TEST(CompressedPair, NonEmptyPairConstruction)
{
    CompressedPair<int, float> pair(10, 3.14f);
    EXPECT_EQ(pair.First(), 10);
    EXPECT_FLOAT_EQ(pair.Second(), 3.14f);
}

TEST(CompressedPair, NonEmptyPairAccessors)
{
    CompressedPair<int, double> pair(5, 2.71828);
    pair.First() = 7;
    pair.Second() = 1.414;
    EXPECT_EQ(pair.First(), 7);
    EXPECT_DOUBLE_EQ(pair.Second(), 1.414);
}

TEST(CompressedPair, NonEmptyDefaultConstruction)
{
    CompressedPair<int, int> pair;
    EXPECT_EQ(pair.First(), 0);
    EXPECT_EQ(pair.Second(), 0);
}

TEST(CompressedPair, ConstAccess)
{
    const CompressedPair<EmptyClass, int> pair(42);
    EXPECT_EQ(pair.Second(), 42);
    [[maybe_unused]] const EmptyClass& ref = pair.First();
}

TEST(CompressedPair, ConstNonEmptyAccess)
{
    const CompressedPair<int, float> pair(5, 1.0f);
    EXPECT_EQ(pair.First(), 5);
    EXPECT_FLOAT_EQ(pair.Second(), 1.0f);
}

TEST(CompressedPair, EBOWithAllocator)
{
    // Allocator is typically an empty-enough class for EBO
    // CompressedPair<Allocator, int*> should compress Allocator to near-zero
    CompressedPair<Allocator, int*> pair;
    EXPECT_EQ(pair.Second(), nullptr);
}

// ============================================================================
// ContiguousIterator tests
// ============================================================================

TEST(ContiguousIterator, DefaultConstruction)
{
    ContiguousIterator<int> it;
    EXPECT_EQ(it.GetPtr(), nullptr);
}

TEST(ContiguousIterator, ConstructFromPointer)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> it(arr);
    EXPECT_EQ(it.GetPtr(), arr);
}

TEST(ContiguousIterator, Dereference)
{
    int arr[3] = {10, 20, 30};
    ContiguousIterator<int> it(arr);
    EXPECT_EQ(*it, 10);
}

TEST(ContiguousIterator, ArrowOperator)
{
    struct Foo { int x; };
    Foo arr[2] = {{1}, {2}};
    ContiguousIterator<Foo> it(arr);
    EXPECT_EQ(it->x, 1);
}

TEST(ContiguousIterator, SubscriptOperator)
{
    int arr[5] = {10, 20, 30, 40, 50};
    ContiguousIterator<int> it(arr);
    EXPECT_EQ(it[0], 10);
    EXPECT_EQ(it[2], 30);
    EXPECT_EQ(it[4], 50);
}

TEST(ContiguousIterator, PreIncrement)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> it(arr);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST(ContiguousIterator, PostIncrement)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> it(arr);
    auto old = it++;
    EXPECT_EQ(*old, 1);
    EXPECT_EQ(*it, 2);
}

TEST(ContiguousIterator, PreDecrement)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> it(arr + 2);
    --it;
    EXPECT_EQ(*it, 2);
}

TEST(ContiguousIterator, PostDecrement)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> it(arr + 2);
    auto old = it--;
    EXPECT_EQ(*old, 3);
    EXPECT_EQ(*it, 2);
}

TEST(ContiguousIterator, PlusEquals)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> it(arr);
    it += 3;
    EXPECT_EQ(*it, 4);
}

TEST(ContiguousIterator, MinusEquals)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> it(arr + 4);
    it -= 2;
    EXPECT_EQ(*it, 3);
}

TEST(ContiguousIterator, IteratorPlusN)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> it(arr);
    auto it2 = it + 3;
    EXPECT_EQ(*it2, 4);
}

TEST(ContiguousIterator, NPlusIterator)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> it(arr);
    auto it2 = 2 + it;
    EXPECT_EQ(*it2, 3);
}

TEST(ContiguousIterator, IteratorMinusN)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> it(arr + 4);
    auto it2 = it - 2;
    EXPECT_EQ(*it2, 3);
}

TEST(ContiguousIterator, IteratorDifference)
{
    int arr[5] = {1, 2, 3, 4, 5};
    ContiguousIterator<int> a(arr);
    ContiguousIterator<int> b(arr + 4);
    EXPECT_EQ(b - a, 4);
    EXPECT_EQ(a - b, -4);
}

TEST(ContiguousIterator, EqualityComparison)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> a(arr);
    ContiguousIterator<int> b(arr);
    ContiguousIterator<int> c(arr + 1);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(ContiguousIterator, InequalityComparison)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> a(arr);
    ContiguousIterator<int> b(arr + 1);
    EXPECT_TRUE(a != b);
}

TEST(ContiguousIterator, LessThan)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> a(arr);
    ContiguousIterator<int> b(arr + 1);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(ContiguousIterator, GreaterThan)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> a(arr);
    ContiguousIterator<int> b(arr + 2);
    EXPECT_TRUE(b > a);
    EXPECT_FALSE(a > b);
}

TEST(ContiguousIterator, LessOrEqual)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> a(arr);
    ContiguousIterator<int> b(arr);
    EXPECT_TRUE(a <= b);
}

TEST(ContiguousIterator, GreaterOrEqual)
{
    int arr[3] = {1, 2, 3};
    ContiguousIterator<int> a(arr + 2);
    ContiguousIterator<int> b(arr + 1);
    EXPECT_TRUE(a >= b);
}

TEST(ContiguousIterator, IteratorCategory)
{
    static_assert(std::is_same_v<ContiguousIterator<int>::iterator_category, std::random_access_iterator_tag>);
}

// ============================================================================
// IteratorValidity tests
// ============================================================================

TEST(IteratorValidity, Values)
{
    EXPECT_EQ(static_cast<int>(IteratorValidity::Valid), 0);
    EXPECT_EQ(static_cast<int>(IteratorValidity::NotOwned), 1);
    EXPECT_EQ(static_cast<int>(IteratorValidity::Invalidated), 2);
}

// ============================================================================
// Hash tests
// ============================================================================

TEST(Hash, BoolHash)
{
    Hash<bool> h;
    EXPECT_EQ(h(false), 0u);
    EXPECT_EQ(h(true), 1u);
}

TEST(Hash, CharHash)
{
    Hash<char> h;
    EXPECT_EQ(h('A'), static_cast<size_t>('A'));
}

TEST(Hash, Int8Hash)
{
    Hash<int8_t> h;
    EXPECT_EQ(h(42), static_cast<size_t>(42));
}

TEST(Hash, Uint8Hash)
{
    Hash<uint8_t> h;
    EXPECT_EQ(h(255), static_cast<size_t>(255));
}

TEST(Hash, Int16Hash)
{
    Hash<int16_t> h;
    EXPECT_EQ(h(1000), static_cast<size_t>(1000));
}

TEST(Hash, Uint16Hash)
{
    Hash<uint16_t> h;
    EXPECT_EQ(h(65535), static_cast<size_t>(65535));
}

TEST(Hash, Int32Hash)
{
    Hash<int32_t> h;
    size_t result = h(42);
    EXPECT_EQ(result, MurmurHash3Mix(static_cast<size_t>(42)));
}

TEST(Hash, Uint32Hash)
{
    Hash<uint32_t> h;
    size_t result = h(100u);
    EXPECT_EQ(result, MurmurHash3Mix(static_cast<size_t>(100u)));
}

TEST(Hash, Int64Hash)
{
    Hash<int64_t> h;
    size_t result = h(int64_t(12345));
    EXPECT_EQ(result, MurmurHash3Mix(static_cast<size_t>(12345)));
}

TEST(Hash, Uint64Hash)
{
    Hash<uint64_t> h;
    size_t result = h(uint64_t(99999));
    EXPECT_EQ(result, MurmurHash3Mix(static_cast<size_t>(99999)));
}

TEST(Hash, FloatHash)
{
    Hash<float> h;
    size_t r1 = h(1.0f);
    size_t r2 = h(2.0f);
    EXPECT_NE(r1, r2);
}

TEST(Hash, FloatHashSameValue)
{
    Hash<float> h;
    EXPECT_EQ(h(3.14f), h(3.14f));
}

TEST(Hash, DoubleHash)
{
    Hash<double> h;
    size_t r1 = h(1.0);
    size_t r2 = h(2.0);
    EXPECT_NE(r1, r2);
}

TEST(Hash, DoubleHashSameValue)
{
    Hash<double> h;
    EXPECT_EQ(h(2.71828), h(2.71828));
}

TEST(Hash, PointerHash)
{
    Hash<int*> h;
    int a = 1, b = 2;
    size_t ha = h(&a);
    size_t hb = h(&b);
    EXPECT_NE(ha, hb);
}

TEST(Hash, NullPointerHash)
{
    Hash<int*> h;
    EXPECT_EQ(h(nullptr), MurmurHash3Mix(0));
}

TEST(Hash, StdStringHash)
{
    Hash<gx::String> h;
    EXPECT_EQ(h(gx::String("hello")), h(gx::String("hello")));
    EXPECT_NE(h(gx::String("hello")), h(gx::String("world")));
}

TEST(Hash, StdWStringHash)
{
    Hash<gx::WString> h;
    EXPECT_EQ(h(gx::WString(L"hello")), h(gx::WString(L"hello")));
    EXPECT_NE(h(gx::WString(L"hello")), h(gx::WString(L"world")));
}

TEST(Hash, StringViewHash)
{
    Hash<std::string_view> h;
    EXPECT_EQ(h(std::string_view("test")), h(std::string_view("test")));
    EXPECT_NE(h(std::string_view("abc")), h(std::string_view("xyz")));
}

TEST(Hash, WStringViewHash)
{
    Hash<std::wstring_view> h;
    EXPECT_EQ(h(std::wstring_view(L"test")), h(std::wstring_view(L"test")));
}

TEST(Hash, ConstCharPtrHash)
{
    Hash<const char*> h;
    EXPECT_EQ(h("hello"), h("hello"));
    EXPECT_NE(h("hello"), h("world"));
}

TEST(Hash, ConstCharPtrNullHash)
{
    Hash<const char*> h;
    EXPECT_EQ(h(nullptr), 0u);
}

TEST(Hash, ConstWCharPtrHash)
{
    Hash<const wchar_t*> h;
    EXPECT_EQ(h(L"hello"), h(L"hello"));
    EXPECT_NE(h(L"hello"), h(L"world"));
}

TEST(Hash, ConstWCharPtrNullHash)
{
    Hash<const wchar_t*> h;
    EXPECT_EQ(h(nullptr), 0u);
}

// ============================================================================
// FNV1a tests
// ============================================================================

TEST(FNV1a, EmptyInput)
{
    size_t h = FNV1a("", 0);
    // FNV offset basis
    if constexpr (sizeof(size_t) == 8)
        EXPECT_EQ(h, 14695981039346656037ULL);
    else
        EXPECT_EQ(h, 2166136261U);
}

TEST(FNV1a, ConsistentResults)
{
    const char* data = "test string";
    size_t h1 = FNV1a(data, std::strlen(data));
    size_t h2 = FNV1a(data, std::strlen(data));
    EXPECT_EQ(h1, h2);
}

TEST(FNV1a, DifferentInputsDifferentHashes)
{
    size_t h1 = FNV1a("abc", 3);
    size_t h2 = FNV1a("def", 3);
    EXPECT_NE(h1, h2);
}

// ============================================================================
// MurmurHash3Mix tests
// ============================================================================

TEST(MurmurHash3Mix, ZeroInput)
{
    size_t h = MurmurHash3Mix(0);
    // MurmurHash3 finalizer: 0 maps to 0 (XOR/multiply chain preserves 0)
    EXPECT_EQ(h, 0u);
}

TEST(MurmurHash3Mix, ConsistentResults)
{
    EXPECT_EQ(MurmurHash3Mix(42), MurmurHash3Mix(42));
}

TEST(MurmurHash3Mix, DifferentInputs)
{
    EXPECT_NE(MurmurHash3Mix(1), MurmurHash3Mix(2));
}

// ============================================================================
// HashCombine tests
// ============================================================================

TEST(HashCombine, CombineWithZero)
{
    size_t seed = 0;
    size_t result = HashCombine(seed, 42);
    EXPECT_NE(result, 0u);
}

TEST(HashCombine, OrderMatters)
{
    size_t r1 = HashCombine(0, 1);
    size_t r2 = HashCombine(1, 0);
    EXPECT_NE(r1, r2);
}

TEST(HashCombine, ConsistentResults)
{
    EXPECT_EQ(HashCombine(10, 20), HashCombine(10, 20));
}

TEST(HashCombine, ChainedCombine)
{
    size_t h = 0;
    h = HashCombine(h, 1);
    h = HashCombine(h, 2);
    h = HashCombine(h, 3);
    EXPECT_NE(h, 0u);
}

// ============================================================================
// EqualTo tests
// ============================================================================

TEST(EqualTo, TypedEqual)
{
    EqualTo<int> eq;
    EXPECT_TRUE(eq(5, 5));
    EXPECT_FALSE(eq(5, 6));
}

TEST(EqualTo, VoidEqual)
{
    EqualTo<> eq;
    EXPECT_TRUE(eq(5, 5));
    EXPECT_FALSE(eq(5, 6));
    EXPECT_TRUE(eq(gx::String("a"), gx::String("a")));
}

// ============================================================================
// Less tests
// ============================================================================

TEST(Less, TypedLess)
{
    Less<int> cmp;
    EXPECT_TRUE(cmp(1, 2));
    EXPECT_FALSE(cmp(2, 1));
    EXPECT_FALSE(cmp(1, 1));
}

TEST(Less, VoidLess)
{
    Less<> cmp;
    EXPECT_TRUE(cmp(1, 2));
    EXPECT_FALSE(cmp(2, 1));
}

// ============================================================================
// PairExtractKey / IdentityExtractKey tests
// ============================================================================

TEST(PairExtractKey, ExtractsFirst)
{
    std::pair<int, gx::String> p(42, "hello");
    PairExtractKey<std::pair<int, gx::String>> extract;
    EXPECT_EQ(extract(p), 42);
}

TEST(IdentityExtractKey, ReturnsValue)
{
    IdentityExtractKey<int> extract;
    EXPECT_EQ(extract(42), 42);
}
