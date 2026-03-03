/// @file test_ContainerBitset.cpp
/// @brief Container Bitset and BitVector tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/Bitset.h"
#include "Container/BitVector.h"

using namespace gx::container;

// ============================================================================
// Bitset: Construction
// ============================================================================

TEST(BitsetTest, DefaultConstructionAllZero)
{
    Bitset<64> b;
    EXPECT_TRUE(b.None());
    EXPECT_FALSE(b.Any());
    EXPECT_EQ(b.Count(), 0u);
}

TEST(BitsetTest, ConstructFromValue)
{
    Bitset<64> b(0b1010);
    EXPECT_TRUE(b.Test(1));
    EXPECT_TRUE(b.Test(3));
    EXPECT_FALSE(b.Test(0));
    EXPECT_FALSE(b.Test(2));
    EXPECT_EQ(b.Count(), 2u);
}

TEST(BitsetTest, ConstructFromValueSmallBitset)
{
    // Only 4 bits: value 0xFF should be masked to 0x0F
    Bitset<4> b(0xFF);
    EXPECT_EQ(b.Count(), 4u);
    EXPECT_TRUE(b.All());
}

// ============================================================================
// Bitset: Set / Reset / Flip / Test
// ============================================================================

TEST(BitsetTest, SetBit)
{
    Bitset<32> b;
    b.Set(5);
    EXPECT_TRUE(b.Test(5));
    EXPECT_EQ(b.Count(), 1u);
}

TEST(BitsetTest, SetBitWithFalse)
{
    Bitset<32> b;
    b.Set(5, true);
    EXPECT_TRUE(b.Test(5));
    b.Set(5, false);
    EXPECT_FALSE(b.Test(5));
}

TEST(BitsetTest, ResetBit)
{
    Bitset<32> b;
    b.Set(10);
    EXPECT_TRUE(b.Test(10));
    b.Reset(10);
    EXPECT_FALSE(b.Test(10));
}

TEST(BitsetTest, FlipBit)
{
    Bitset<32> b;
    b.Flip(7);
    EXPECT_TRUE(b.Test(7));
    b.Flip(7);
    EXPECT_FALSE(b.Test(7));
}

TEST(BitsetTest, OperatorBracketRead)
{
    Bitset<16> b;
    b.Set(3);
    EXPECT_TRUE(b[3]);
    EXPECT_FALSE(b[4]);
}

// ============================================================================
// Bitset: Bulk operations
// ============================================================================

TEST(BitsetTest, SetAll)
{
    Bitset<100> b;
    b.SetAll();
    EXPECT_TRUE(b.All());
    EXPECT_EQ(b.Count(), 100u);
}

TEST(BitsetTest, ResetAll)
{
    Bitset<100> b;
    b.SetAll();
    b.ResetAll();
    EXPECT_TRUE(b.None());
    EXPECT_EQ(b.Count(), 0u);
}

TEST(BitsetTest, FlipAll)
{
    Bitset<8> b(0b10101010);
    b.FlipAll();
    EXPECT_TRUE(b.Test(0));
    EXPECT_FALSE(b.Test(1));
    EXPECT_TRUE(b.Test(2));
    EXPECT_FALSE(b.Test(3));
    EXPECT_EQ(b.Count(), 4u);
}

// ============================================================================
// Bitset: Bitwise operators
// ============================================================================

TEST(BitsetTest, BitwiseAnd)
{
    Bitset<8> a(0b11001100);
    Bitset<8> b(0b10101010);
    auto c = a & b;
    EXPECT_EQ(c.Count(), 2u);
    EXPECT_TRUE(c.Test(3));
    EXPECT_TRUE(c.Test(7));
}

TEST(BitsetTest, BitwiseOr)
{
    Bitset<8> a(0b11001100);
    Bitset<8> b(0b10101010);
    auto c = a | b;
    EXPECT_EQ(c.Count(), 6u);
}

TEST(BitsetTest, BitwiseXor)
{
    Bitset<8> a(0b11001100);
    Bitset<8> b(0b10101010);
    auto c = a ^ b;
    // XOR: 01100110 => bits 1,2,5,6
    EXPECT_EQ(c.Count(), 4u);
}

TEST(BitsetTest, BitwiseNot)
{
    Bitset<8> a(0b11001100);
    auto c = ~a;
    EXPECT_EQ(c.Count(), 4u);
    EXPECT_TRUE(c.Test(0));
    EXPECT_TRUE(c.Test(1));
    EXPECT_FALSE(c.Test(2));
    EXPECT_FALSE(c.Test(3));
}

TEST(BitsetTest, BitwiseAndAssign)
{
    Bitset<8> a(0b11110000);
    Bitset<8> b(0b10101010);
    a &= b;
    EXPECT_EQ(a.Count(), 2u);
}

TEST(BitsetTest, BitwiseOrAssign)
{
    Bitset<8> a(0b11110000);
    Bitset<8> b(0b00001111);
    a |= b;
    EXPECT_TRUE(a.All());
}

TEST(BitsetTest, BitwiseXorAssign)
{
    Bitset<8> a(0b11111111);
    Bitset<8> b(0b11111111);
    a ^= b;
    EXPECT_TRUE(a.None());
}

// ============================================================================
// Bitset: Count (popcount)
// ============================================================================

TEST(BitsetTest, CountEmpty)
{
    Bitset<128> b;
    EXPECT_EQ(b.Count(), 0u);
}

TEST(BitsetTest, CountSomeBits)
{
    Bitset<128> b;
    b.Set(0);
    b.Set(63);
    b.Set(64);
    b.Set(127);
    EXPECT_EQ(b.Count(), 4u);
}

TEST(BitsetTest, CountAllBits)
{
    Bitset<128> b;
    b.SetAll();
    EXPECT_EQ(b.Count(), 128u);
}

// ============================================================================
// Bitset: None / Any / All
// ============================================================================

TEST(BitsetTest, NoneOnEmpty)
{
    Bitset<32> b;
    EXPECT_TRUE(b.None());
    EXPECT_FALSE(b.Any());
    EXPECT_FALSE(b.All());
}

TEST(BitsetTest, AnyOnPartial)
{
    Bitset<32> b;
    b.Set(15);
    EXPECT_FALSE(b.None());
    EXPECT_TRUE(b.Any());
    EXPECT_FALSE(b.All());
}

TEST(BitsetTest, AllOnFull)
{
    Bitset<32> b;
    b.SetAll();
    EXPECT_FALSE(b.None());
    EXPECT_TRUE(b.Any());
    EXPECT_TRUE(b.All());
}

// ============================================================================
// Bitset: Size
// ============================================================================

TEST(BitsetTest, SizeReturnsN)
{
    Bitset<100> b;
    EXPECT_EQ(b.Size(), 100u);
}

// ============================================================================
// Bitset: FindFirst / FindLast / FindNext
// ============================================================================

TEST(BitsetTest, FindFirstNone)
{
    Bitset<64> b;
    EXPECT_EQ(b.FindFirst(), Bitset<64>::Npos);
}

TEST(BitsetTest, FindFirstSingle)
{
    Bitset<64> b;
    b.Set(42);
    EXPECT_EQ(b.FindFirst(), 42u);
}

TEST(BitsetTest, FindFirstMultiple)
{
    Bitset<128> b;
    b.Set(70);
    b.Set(10);
    b.Set(100);
    EXPECT_EQ(b.FindFirst(), 10u);
}

TEST(BitsetTest, FindLastNone)
{
    Bitset<64> b;
    EXPECT_EQ(b.FindLast(), Bitset<64>::Npos);
}

TEST(BitsetTest, FindLastSingle)
{
    Bitset<128> b;
    b.Set(42);
    EXPECT_EQ(b.FindLast(), 42u);
}

TEST(BitsetTest, FindLastMultiple)
{
    Bitset<128> b;
    b.Set(10);
    b.Set(70);
    b.Set(100);
    EXPECT_EQ(b.FindLast(), 100u);
}

TEST(BitsetTest, FindNextBasic)
{
    Bitset<128> b;
    b.Set(10);
    b.Set(50);
    b.Set(100);
    EXPECT_EQ(b.FindNext(10), 50u);
    EXPECT_EQ(b.FindNext(50), 100u);
    EXPECT_EQ(b.FindNext(100), Bitset<128>::Npos);
}

TEST(BitsetTest, FindNextFromBeforeFirst)
{
    Bitset<128> b;
    b.Set(5);
    b.Set(70);
    // FindNext(0) looks for bit > 0
    size_t next = b.FindNext(0);
    EXPECT_EQ(next, 5u);
}

// ============================================================================
// Bitset: Large bitset (>64 bits)
// ============================================================================

TEST(BitsetTest, LargeBitset256)
{
    Bitset<256> b;
    b.Set(0);
    b.Set(63);
    b.Set(64);
    b.Set(127);
    b.Set(128);
    b.Set(200);
    b.Set(255);
    EXPECT_EQ(b.Count(), 7u);
    EXPECT_EQ(b.FindFirst(), 0u);
    EXPECT_EQ(b.FindLast(), 255u);
}

TEST(BitsetTest, LargeBitsetSetAllAndCount)
{
    Bitset<200> b;
    b.SetAll();
    EXPECT_EQ(b.Count(), 200u);
    EXPECT_TRUE(b.All());
}

// ============================================================================
// Bitset: Comparison
// ============================================================================

TEST(BitsetTest, Equality)
{
    Bitset<64> a(0x1234);
    Bitset<64> b(0x1234);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(BitsetTest, Inequality)
{
    Bitset<64> a(0x1234);
    Bitset<64> b(0x5678);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

// ============================================================================
// BitVector: Construction
// ============================================================================

TEST(BitVectorTest, DefaultConstruction)
{
    BitVector<> bv;
    EXPECT_EQ(bv.Size(), 0u);
    EXPECT_TRUE(bv.Empty());
}

TEST(BitVectorTest, ConstructWithSize)
{
    BitVector<> bv(100);
    EXPECT_EQ(bv.Size(), 100u);
    EXPECT_FALSE(bv.Empty());
    // All bits should be false
    EXPECT_EQ(bv.Count(), 0u);
}

TEST(BitVectorTest, ConstructWithSizeAndValue)
{
    BitVector<> bv(100, true);
    EXPECT_EQ(bv.Size(), 100u);
    EXPECT_EQ(bv.Count(), 100u);
}

// ============================================================================
// BitVector: Resize
// ============================================================================

TEST(BitVectorTest, ResizeGrow)
{
    BitVector<> bv;
    bv.Resize(50);
    EXPECT_EQ(bv.Size(), 50u);
    EXPECT_EQ(bv.Count(), 0u);
}

TEST(BitVectorTest, ResizeGrowWithValue)
{
    BitVector<> bv(10, true);
    bv.Resize(50, true);
    EXPECT_EQ(bv.Size(), 50u);
    EXPECT_EQ(bv.Count(), 50u);
}

TEST(BitVectorTest, ResizeShrink)
{
    BitVector<> bv(100, true);
    bv.Resize(50);
    EXPECT_EQ(bv.Size(), 50u);
}

// ============================================================================
// BitVector: PushBack / PopBack
// ============================================================================

TEST(BitVectorTest, PushBackBits)
{
    BitVector<> bv;
    bv.PushBack(true);
    bv.PushBack(false);
    bv.PushBack(true);
    EXPECT_EQ(bv.Size(), 3u);
    EXPECT_TRUE(bv.Test(0));
    EXPECT_FALSE(bv.Test(1));
    EXPECT_TRUE(bv.Test(2));
}

TEST(BitVectorTest, PopBack)
{
    BitVector<> bv;
    bv.PushBack(true);
    bv.PushBack(false);
    bv.PushBack(true);
    bv.PopBack();
    EXPECT_EQ(bv.Size(), 2u);
    bv.PopBack();
    EXPECT_EQ(bv.Size(), 1u);
}

TEST(BitVectorTest, PushBackMany)
{
    BitVector<> bv;
    for (int i = 0; i < 200; ++i)
        bv.PushBack(i % 3 == 0);
    EXPECT_EQ(bv.Size(), 200u);
    for (int i = 0; i < 200; ++i)
        EXPECT_EQ(bv.Test(static_cast<size_t>(i)), (i % 3 == 0));
}

// ============================================================================
// BitVector: Set / Reset / Test
// ============================================================================

TEST(BitVectorTest, SetAndTest)
{
    BitVector<> bv(64);
    bv.Set(0);
    bv.Set(31);
    bv.Set(63);
    EXPECT_TRUE(bv.Test(0));
    EXPECT_TRUE(bv.Test(31));
    EXPECT_TRUE(bv.Test(63));
    EXPECT_FALSE(bv.Test(1));
}

TEST(BitVectorTest, ResetBit)
{
    BitVector<> bv(32, true);
    bv.Reset(10);
    EXPECT_FALSE(bv.Test(10));
    EXPECT_EQ(bv.Count(), 31u);
}

TEST(BitVectorTest, OperatorBracket)
{
    BitVector<> bv(16);
    bv.Set(5);
    EXPECT_TRUE(bv[5]);
    EXPECT_FALSE(bv[6]);
}

// ============================================================================
// BitVector: Count
// ============================================================================

TEST(BitVectorTest, CountEmpty)
{
    BitVector<> bv;
    EXPECT_EQ(bv.Count(), 0u);
}

TEST(BitVectorTest, CountPartial)
{
    BitVector<> bv(100);
    bv.Set(0);
    bv.Set(50);
    bv.Set(99);
    EXPECT_EQ(bv.Count(), 3u);
}

// ============================================================================
// BitVector: FindFirst / FindNext
// ============================================================================

TEST(BitVectorTest, FindFirstEmpty)
{
    BitVector<> bv;
    EXPECT_EQ(bv.FindFirst(), BitVector<>::Npos);
}

TEST(BitVectorTest, FindFirstPresent)
{
    BitVector<> bv(128);
    bv.Set(42);
    bv.Set(100);
    EXPECT_EQ(bv.FindFirst(), 42u);
}

TEST(BitVectorTest, FindNextBasic)
{
    BitVector<> bv(128);
    bv.Set(10);
    bv.Set(50);
    bv.Set(100);
    EXPECT_EQ(bv.FindNext(10), 50u);
    EXPECT_EQ(bv.FindNext(50), 100u);
    EXPECT_EQ(bv.FindNext(100), BitVector<>::Npos);
}

// ============================================================================
// BitVector: Size / Empty / Clear / Reserve
// ============================================================================

TEST(BitVectorTest, SizeAndCapacity)
{
    BitVector<> bv(100);
    EXPECT_EQ(bv.Size(), 100u);
    EXPECT_GE(bv.Capacity(), 100u);
}

TEST(BitVectorTest, Clear)
{
    BitVector<> bv(100, true);
    bv.Clear();
    EXPECT_EQ(bv.Size(), 0u);
    EXPECT_TRUE(bv.Empty());
}

TEST(BitVectorTest, Reserve)
{
    BitVector<> bv;
    bv.Reserve(1000);
    EXPECT_GE(bv.Capacity(), 1000u);
    EXPECT_EQ(bv.Size(), 0u); // reserve does not change size
}

// ============================================================================
// BitVector: Copy / Move
// ============================================================================

TEST(BitVectorTest, CopyConstruct)
{
    BitVector<> bv(64);
    bv.Set(0);
    bv.Set(63);
    BitVector<> copy(bv);
    EXPECT_EQ(copy.Size(), 64u);
    EXPECT_TRUE(copy.Test(0));
    EXPECT_TRUE(copy.Test(63));
}

TEST(BitVectorTest, MoveConstruct)
{
    BitVector<> bv(64);
    bv.Set(0);
    bv.Set(63);
    BitVector<> moved(std::move(bv));
    EXPECT_EQ(moved.Size(), 64u);
    EXPECT_TRUE(moved.Test(0));
    EXPECT_TRUE(moved.Test(63));
    EXPECT_EQ(bv.Size(), 0u);
}

TEST(BitVectorTest, Equality)
{
    BitVector<> a(32);
    a.Set(5);
    a.Set(20);
    BitVector<> b(32);
    b.Set(5);
    b.Set(20);
    EXPECT_TRUE(a == b);
    b.Set(10);
    EXPECT_TRUE(a != b);
}
