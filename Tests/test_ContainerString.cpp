/// @file test_ContainerString.cpp
/// @brief Container String and WString tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Container/String.h"
#include "Container/WString.h"

#include <string>
#include <string_view>

using namespace gx::container;

// ============================================================================
// String Construction
// ============================================================================

TEST(StringConstruction, Default)
{
    String s;
    EXPECT_EQ(s.Size(), 0u);
    EXPECT_TRUE(s.Empty());
    EXPECT_STREQ(s.c_str(), "");
}

TEST(StringConstruction, FromCStr)
{
    String s("hello");
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringConstruction, FromNullCStr)
{
    String s(nullptr);
    EXPECT_EQ(s.Size(), 0u);
    EXPECT_TRUE(s.Empty());
}

TEST(StringConstruction, FromCStrWithLength)
{
    String s("hello world", 5);
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringConstruction, FromCharRepeat)
{
    String s(5, 'x');
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.c_str(), "xxxxx");
}

TEST(StringConstruction, FromCharRepeatLong)
{
    String s(50, 'A');
    EXPECT_EQ(s.Size(), 50u);
    for (size_t i = 0; i < 50; ++i)
        EXPECT_EQ(s[i], 'A');
}

TEST(StringConstruction, FromStdString)
{
    gx::String std_s = "std string";
    String s(std_s);
    EXPECT_EQ(s.Size(), 10u);
    EXPECT_STREQ(s.c_str(), "std string");
}

TEST(StringConstruction, FromStringView)
{
    std::string_view sv = "view";
    String s(sv);
    EXPECT_EQ(s.Size(), 4u);
    EXPECT_STREQ(s.c_str(), "view");
}

TEST(StringConstruction, CopyConstructor)
{
    String s1("hello");
    String s2(s1);
    EXPECT_EQ(s2.Size(), 5u);
    EXPECT_STREQ(s2.c_str(), "hello");
    EXPECT_EQ(s1.Size(), 5u); // original unchanged
}

TEST(StringConstruction, MoveConstructor)
{
    String s1("hello");
    String s2(std::move(s1));
    EXPECT_EQ(s2.Size(), 5u);
    EXPECT_STREQ(s2.c_str(), "hello");
    EXPECT_TRUE(s1.Empty());
}

TEST(StringConstruction, CopyConstructorLong)
{
    String s1("This is a very long string that exceeds SSO capacity for sure");
    String s2(s1);
    EXPECT_EQ(s2.Size(), s1.Size());
    EXPECT_STREQ(s2.c_str(), s1.c_str());
}

TEST(StringConstruction, MoveConstructorLong)
{
    String s1("This is a very long string that exceeds SSO capacity for sure");
    size_t origSize = s1.Size();
    String s2(std::move(s1));
    EXPECT_EQ(s2.Size(), origSize);
    EXPECT_TRUE(s1.Empty());
}

// ============================================================================
// SSO behavior
// ============================================================================

TEST(StringSSO, ShortStringStaysInline)
{
    String s("short");
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_LE(s.Size(), String::k_SSOCapacity);
    EXPECT_TRUE(s.Validate());
}

TEST(StringSSO, ExactSSOCapacity)
{
    // 22 characters is the SSO max
    String s(22, 'a');
    EXPECT_EQ(s.Size(), 22u);
    EXPECT_EQ(s.Capacity(), String::k_SSOCapacity);
    EXPECT_TRUE(s.Validate());
}

TEST(StringSSO, HeapAllocationBeyondSSO)
{
    String s(23, 'b');
    EXPECT_EQ(s.Size(), 23u);
    EXPECT_GT(s.Capacity(), String::k_SSOCapacity);
    EXPECT_TRUE(s.Validate());
}

TEST(StringSSO, LongStringOnHeap)
{
    String s("This string is definitely longer than twenty-two characters!");
    EXPECT_GT(s.Size(), 22u);
    EXPECT_TRUE(s.Validate());
}

// ============================================================================
// Assignment
// ============================================================================

TEST(StringAssignment, CopyAssignment)
{
    String s1("hello");
    String s2;
    s2 = s1;
    EXPECT_STREQ(s2.c_str(), "hello");
}

TEST(StringAssignment, CopyAssignmentSelf)
{
    String s("hello");
    s = s;
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringAssignment, MoveAssignment)
{
    String s1("hello");
    String s2;
    s2 = std::move(s1);
    EXPECT_STREQ(s2.c_str(), "hello");
    EXPECT_TRUE(s1.Empty());
}

TEST(StringAssignment, CStrAssignment)
{
    String s;
    s = "assigned";
    EXPECT_STREQ(s.c_str(), "assigned");
}

TEST(StringAssignment, StringViewAssignment)
{
    String s;
    s = std::string_view("view assign");
    EXPECT_STREQ(s.c_str(), "view assign");
}

TEST(StringAssignment, CharAssignment)
{
    String s("hello");
    s = 'X';
    EXPECT_EQ(s.Size(), 1u);
    EXPECT_EQ(s[0], 'X');
}

// ============================================================================
// Element Access
// ============================================================================

TEST(StringAccess, OperatorBracket)
{
    String s("abc");
    EXPECT_EQ(s[0], 'a');
    EXPECT_EQ(s[1], 'b');
    EXPECT_EQ(s[2], 'c');
}

TEST(StringAccess, OperatorBracketModify)
{
    String s("abc");
    s[1] = 'X';
    EXPECT_EQ(s[1], 'X');
}

TEST(StringAccess, Front)
{
    String s("hello");
    EXPECT_EQ(s.Front(), 'h');
}

TEST(StringAccess, Back)
{
    String s("hello");
    EXPECT_EQ(s.Back(), 'o');
}

TEST(StringAccess, CStr)
{
    String s("test");
    EXPECT_STREQ(s.c_str(), "test");
}

TEST(StringAccess, Data)
{
    String s("abc");
    const char* p = s.Data();
    EXPECT_EQ(p[0], 'a');
}

// ============================================================================
// Iterators
// ============================================================================

TEST(StringIterators, BeginEnd)
{
    String s("abc");
    gx::String result;
    for (auto it = s.begin(); it != s.end(); ++it)
        result += *it;
    EXPECT_EQ(result, "abc");
}

TEST(StringIterators, RangeFor)
{
    String s("xyz");
    gx::String result;
    for (char c : s)
        result += c;
    EXPECT_EQ(result, "xyz");
}

// ============================================================================
// Capacity
// ============================================================================

TEST(StringCapacity, Size)
{
    EXPECT_EQ(String("abc").Size(), 3u);
    EXPECT_EQ(String("").Size(), 0u);
}

TEST(StringCapacity, Length)
{
    String s("abc");
    EXPECT_EQ(s.Length(), s.Size());
}

TEST(StringCapacity, Empty)
{
    EXPECT_TRUE(String().Empty());
    EXPECT_FALSE(String("a").Empty());
}

TEST(StringCapacity, CapacitySSO)
{
    String s("short");
    EXPECT_EQ(s.Capacity(), String::k_SSOCapacity);
}

TEST(StringCapacity, Reserve)
{
    String s("hello");
    s.Reserve(100);
    EXPECT_GE(s.Capacity(), 100u);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringCapacity, ReserveSmallerNoOp)
{
    String s("hello");
    size_t cap = s.Capacity();
    s.Reserve(2);
    EXPECT_EQ(s.Capacity(), cap);
}

TEST(StringCapacity, ReserveSStoHeap)
{
    String s("short");
    s.Reserve(50);
    EXPECT_GE(s.Capacity(), 50u);
    EXPECT_STREQ(s.c_str(), "short");
}

TEST(StringCapacity, ShrinkToFitHeapToSSO)
{
    String s("This is a long string that definitely goes to heap");
    s.erase(5); // Now "This "
    s.ShrinkToFit();
    EXPECT_EQ(s.Capacity(), String::k_SSOCapacity);
    EXPECT_STREQ(s.c_str(), "This ");
}

TEST(StringCapacity, ShrinkToFitHeapShrink)
{
    String s;
    s.Reserve(200);
    s.append("hello");
    s.ShrinkToFit();
    // "hello" (5 chars) fits in SSO, so ShrinkToFit transitions back to SSO
    EXPECT_EQ(s.Capacity(), String::k_SSOCapacity);
}

// ============================================================================
// Append / operator+=
// ============================================================================

TEST(StringAppend, AppendCStr)
{
    String s("hello");
    s.append(" world");
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringAppend, AppendCStrWithLen)
{
    String s("hello");
    s.append(" world!!!", 6);
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringAppend, AppendString)
{
    String s("hello");
    String other(" world");
    s.append(other);
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringAppend, AppendStringView)
{
    String s("hello");
    s.append(std::string_view(" view"));
    EXPECT_STREQ(s.c_str(), "hello view");
}

TEST(StringAppend, AppendNChars)
{
    String s("hi");
    s.append(3, '!');
    EXPECT_STREQ(s.c_str(), "hi!!!");
}

TEST(StringAppend, PushBack)
{
    String s("ab");
    s.push_back('c');
    EXPECT_STREQ(s.c_str(), "abc");
}

TEST(StringAppend, OperatorPlusEqualsString)
{
    String s("hello");
    String other(" world");
    s += other;
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringAppend, OperatorPlusEqualsCStr)
{
    String s("hello");
    s += " world";
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringAppend, OperatorPlusEqualsStringView)
{
    String s("hello");
    s += std::string_view(" sv");
    EXPECT_STREQ(s.c_str(), "hello sv");
}

TEST(StringAppend, OperatorPlusEqualsChar)
{
    String s("ab");
    s += 'c';
    EXPECT_STREQ(s.c_str(), "abc");
}

TEST(StringAppend, AppendEmptyNoOp)
{
    String s("hello");
    s.append("", 0);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringAppend, AppendTriggerGrow)
{
    String s(20, 'a');
    s.append("bbbbb"); // Should trigger SSO->heap transition
    EXPECT_EQ(s.Size(), 25u);
    EXPECT_TRUE(s.Validate());
}

// ============================================================================
// Insert
// ============================================================================

TEST(StringInsert, InsertAtBeginning)
{
    String s("world");
    s.insert(0, "hello ");
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringInsert, InsertAtEnd)
{
    String s("hello");
    s.insert(5, " world");
    EXPECT_STREQ(s.c_str(), "hello world");
}

TEST(StringInsert, InsertInMiddle)
{
    String s("helo");
    s.insert(2, "l");
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringInsert, InsertNChars)
{
    String s("hd");
    s.insert(1, 2, 'e');
    EXPECT_STREQ(s.c_str(), "heed");
}

TEST(StringInsert, InsertEmptyNoOp)
{
    String s("hello");
    s.insert(2, "", 0);
    EXPECT_STREQ(s.c_str(), "hello");
}

// ============================================================================
// Erase
// ============================================================================

TEST(StringErase, EraseFromMiddle)
{
    String s("hello world");
    s.erase(5, 6);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringErase, EraseFromBeginning)
{
    String s("hello");
    s.erase(0, 2);
    EXPECT_STREQ(s.c_str(), "llo");
}

TEST(StringErase, EraseToEnd)
{
    String s("hello world");
    s.erase(5);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringErase, EraseAll)
{
    String s("hello");
    s.erase(0);
    EXPECT_TRUE(s.Empty());
}

TEST(StringErase, EraseZeroCount)
{
    String s("hello");
    s.erase(2, 0);
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringErase, EraseCountExceedsSize)
{
    String s("hello");
    s.erase(3, 100);
    EXPECT_STREQ(s.c_str(), "hel");
}

// ============================================================================
// Replace
// ============================================================================

TEST(StringReplace, ReplaceMiddle)
{
    String s("hello world");
    s.replace(6, 5, "there");
    EXPECT_STREQ(s.c_str(), "hello there");
}

TEST(StringReplace, ReplaceShorter)
{
    String s("abcdef");
    s.replace(2, 2, "X");
    EXPECT_STREQ(s.c_str(), "abXef");
}

TEST(StringReplace, ReplaceLonger)
{
    String s("abcdef");
    s.replace(2, 2, "XXXX");
    EXPECT_STREQ(s.c_str(), "abXXXXef");
}

TEST(StringReplace, ReplaceWithCStr)
{
    String s("abc");
    s.replace(1, 1, "XY");
    EXPECT_STREQ(s.c_str(), "aXYc");
}

// ============================================================================
// Substr
// ============================================================================

TEST(StringSubstr, FullSubstr)
{
    String s("hello");
    String sub = s.substr();
    EXPECT_STREQ(sub.c_str(), "hello");
}

TEST(StringSubstr, PartialSubstr)
{
    String s("hello world");
    String sub = s.substr(6, 5);
    EXPECT_STREQ(sub.c_str(), "world");
}

TEST(StringSubstr, SubstrFromPos)
{
    String s("hello world");
    String sub = s.substr(6);
    EXPECT_STREQ(sub.c_str(), "world");
}

TEST(StringSubstr, SubstrEmpty)
{
    String s("hello");
    String sub = s.substr(2, 0);
    EXPECT_TRUE(sub.Empty());
}

// ============================================================================
// Find
// ============================================================================

TEST(StringFind, FindCStr)
{
    String s("hello world");
    EXPECT_EQ(s.find("world"), 6u);
}

TEST(StringFind, FindCStrNotFound)
{
    String s("hello");
    EXPECT_EQ(s.find("xyz"), String::npos);
}

TEST(StringFind, FindCStrFromPos)
{
    String s("abcabc");
    EXPECT_EQ(s.find("abc", 1), 3u);
}

TEST(StringFind, FindChar)
{
    String s("hello");
    EXPECT_EQ(s.find('l'), 2u);
}

TEST(StringFind, FindCharNotFound)
{
    String s("hello");
    EXPECT_EQ(s.find('z'), String::npos);
}

TEST(StringFind, FindString)
{
    String s("hello world");
    String needle("world");
    EXPECT_EQ(s.find(needle), 6u);
}

TEST(StringFind, FindEmptyString)
{
    String s("hello");
    EXPECT_EQ(s.find(""), 0u);
}

// ============================================================================
// RFind
// ============================================================================

TEST(StringRFind, RFindCStr)
{
    String s("abcabc");
    EXPECT_EQ(s.rfind("abc"), 3u);
}

TEST(StringRFind, RFindChar)
{
    String s("hello");
    EXPECT_EQ(s.rfind('l'), 3u);
}

TEST(StringRFind, RFindNotFound)
{
    String s("hello");
    EXPECT_EQ(s.rfind("xyz"), String::npos);
}

TEST(StringRFind, RFindWithPos)
{
    String s("abcabc");
    EXPECT_EQ(s.rfind("abc", 2), 0u);
}

TEST(StringRFind, RFindCharNotFound)
{
    String s("hello");
    EXPECT_EQ(s.rfind('z'), String::npos);
}

// ============================================================================
// find_first_of / find_last_of
// ============================================================================

TEST(StringFindOf, FindFirstOf)
{
    String s("hello world");
    EXPECT_EQ(s.find_first_of("ow"), 4u);
}

TEST(StringFindOf, FindFirstOfNotFound)
{
    String s("hello");
    EXPECT_EQ(s.find_first_of("xyz"), String::npos);
}

TEST(StringFindOf, FindLastOf)
{
    String s("hello world");
    EXPECT_EQ(s.find_last_of("ow"), 7u);
}

TEST(StringFindOf, FindLastOfNotFound)
{
    String s("hello");
    EXPECT_EQ(s.find_last_of("xyz"), String::npos);
}

TEST(StringFindOf, FindFirstNotOf)
{
    String s("aaabcd");
    EXPECT_EQ(s.find_first_not_of("a"), 3u);
}

TEST(StringFindOf, FindLastNotOf)
{
    String s("abcaaa");
    EXPECT_EQ(s.find_last_not_of("a"), 2u);
}

// ============================================================================
// Comparison
// ============================================================================

TEST(StringComparison, EqualStrings)
{
    String a("hello");
    String b("hello");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(StringComparison, NotEqualStrings)
{
    String a("hello");
    String b("world");
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(StringComparison, LessThan)
{
    String a("abc");
    String b("abd");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(StringComparison, GreaterThan)
{
    String a("xyz");
    String b("abc");
    EXPECT_TRUE(a > b);
}

TEST(StringComparison, LessOrEqual)
{
    String a("abc");
    EXPECT_TRUE(a <= a);
}

TEST(StringComparison, GreaterOrEqual)
{
    String a("abc");
    EXPECT_TRUE(a >= a);
}

TEST(StringComparison, EqualCStr)
{
    String s("hello");
    EXPECT_TRUE(s == "hello");
    EXPECT_TRUE("hello" == s);
    EXPECT_FALSE(s == "world");
}

TEST(StringComparison, NotEqualCStr)
{
    String s("hello");
    EXPECT_FALSE(s != "hello");
    EXPECT_TRUE(s != "world");
}

TEST(StringComparison, EqualStringView)
{
    String s("hello");
    EXPECT_TRUE(s == std::string_view("hello"));
    EXPECT_TRUE(std::string_view("hello") == s);
}

TEST(StringComparison, NotEqualStringView)
{
    String s("hello");
    EXPECT_FALSE(s != std::string_view("hello"));
    EXPECT_TRUE(s != std::string_view("world"));
}

TEST(StringComparison, EqualNullCStr)
{
    String s;
    EXPECT_TRUE(s == (const char*)nullptr);
}

TEST(StringComparison, LessThanShorterPrefix)
{
    String a("ab");
    String b("abc");
    EXPECT_TRUE(a < b);
}

// ============================================================================
// Game extensions
// ============================================================================

TEST(StringGameExt, Sprintf)
{
    String s = String::Sprintf("Hello %s, %d!", "World", 42);
    EXPECT_STREQ(s.c_str(), "Hello World, 42!");
}

TEST(StringGameExt, SprintfEmpty)
{
    String s = String::Sprintf("");
    EXPECT_TRUE(s.Empty());
}

TEST(StringGameExt, AppendSprintf)
{
    String s("Count: ");
    s.AppendSprintf("%d", 42);
    EXPECT_STREQ(s.c_str(), "Count: 42");
}

TEST(StringGameExt, AppendSprintfMultiple)
{
    String s;
    s.AppendSprintf("a=%d ", 1);
    s.AppendSprintf("b=%d", 2);
    EXPECT_STREQ(s.c_str(), "a=1 b=2");
}

TEST(StringGameExt, CompareI)
{
    String a("Hello");
    String b("hello");
    EXPECT_EQ(a.CompareI(b), 0);
}

TEST(StringGameExt, CompareICStr)
{
    String s("HELLO");
    EXPECT_EQ(s.CompareI("hello"), 0);
    EXPECT_NE(s.CompareI("world"), 0);
}

TEST(StringGameExt, CompareINullptr)
{
    String s("hello");
    EXPECT_GT(s.CompareI(nullptr), 0);

    String empty;
    EXPECT_EQ(empty.CompareI(nullptr), 0);
}

TEST(StringGameExt, Trim)
{
    String s("  hello  ");
    s.Trim();
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringGameExt, TrimLeft)
{
    String s("   hello");
    s.TrimLeft();
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringGameExt, TrimRight)
{
    String s("hello   ");
    s.TrimRight();
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringGameExt, TrimEmpty)
{
    String s;
    s.Trim();
    EXPECT_TRUE(s.Empty());
}

TEST(StringGameExt, TrimAllWhitespace)
{
    String s("   \t\n  ");
    s.Trim();
    EXPECT_TRUE(s.Empty());
}

TEST(StringGameExt, TrimNoWhitespace)
{
    String s("hello");
    s.Trim();
    EXPECT_STREQ(s.c_str(), "hello");
}

TEST(StringGameExt, ToLower)
{
    String s("Hello WORLD 123");
    s.ToLower();
    EXPECT_STREQ(s.c_str(), "hello world 123");
}

TEST(StringGameExt, ToUpper)
{
    String s("Hello world 123");
    s.ToUpper();
    EXPECT_STREQ(s.c_str(), "HELLO WORLD 123");
}

TEST(StringGameExt, ToLowerEmpty)
{
    String s;
    s.ToLower();
    EXPECT_TRUE(s.Empty());
}

TEST(StringGameExt, ToUpperEmpty)
{
    String s;
    s.ToUpper();
    EXPECT_TRUE(s.Empty());
}

// ============================================================================
// Conversion
// ============================================================================

TEST(StringConversion, ToStdString)
{
    String s("hello");
    gx::String std_s = s.ToStdString();
    EXPECT_EQ(std_s, "hello");
}

TEST(StringConversion, ToStringView)
{
    String s("hello");
    std::string_view sv = s.ToStringView();
    EXPECT_EQ(sv, "hello");
}

TEST(StringConversion, ImplicitStringViewConversion)
{
    String s("hello");
    std::string_view sv = s;
    EXPECT_EQ(sv, "hello");
}

// ============================================================================
// Resize
// ============================================================================

TEST(StringResize, ResizeGrow)
{
    String s("hi");
    s.resize(5);
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_EQ(s[0], 'h');
    EXPECT_EQ(s[1], 'i');
    EXPECT_EQ(s[2], '\0');
}

TEST(StringResize, ResizeGrowWithChar)
{
    String s("hi");
    s.resize(5, 'x');
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_EQ(s[2], 'x');
    EXPECT_EQ(s[4], 'x');
}

TEST(StringResize, ResizeShrink)
{
    String s("hello world");
    s.resize(5);
    EXPECT_EQ(s.Size(), 5u);
    EXPECT_STREQ(s.c_str(), "hello");
}

// ============================================================================
// Clear
// ============================================================================

TEST(StringClear, ClearSSO)
{
    String s("hello");
    s.clear();
    EXPECT_TRUE(s.Empty());
    EXPECT_STREQ(s.c_str(), "");
}

TEST(StringClear, ClearHeap)
{
    String s("This is a very long string that goes to heap allocation");
    s.clear();
    EXPECT_TRUE(s.Empty());
    EXPECT_STREQ(s.c_str(), "");
}

// ============================================================================
// Swap
// ============================================================================

TEST(StringSwap, SwapSSO)
{
    String a("hello");
    String b("world");
    a.Swap(b);
    EXPECT_STREQ(a.c_str(), "world");
    EXPECT_STREQ(b.c_str(), "hello");
}

TEST(StringSwap, SwapADL)
{
    String a("aaa");
    String b("bbb");
    swap(a, b);
    EXPECT_STREQ(a.c_str(), "bbb");
    EXPECT_STREQ(b.c_str(), "aaa");
}

// ============================================================================
// Concatenation operators
// ============================================================================

TEST(StringConcat, StringPlusString)
{
    String a("hello");
    String b(" world");
    String c = a + b;
    EXPECT_STREQ(c.c_str(), "hello world");
}

TEST(StringConcat, StringPlusCStr)
{
    String a("hello");
    String c = a + " world";
    EXPECT_STREQ(c.c_str(), "hello world");
}

TEST(StringConcat, CStrPlusString)
{
    String b(" world");
    String c = "hello" + b;
    EXPECT_STREQ(c.c_str(), "hello world");
}

TEST(StringConcat, StringPlusChar)
{
    String a("hi");
    String c = a + '!';
    EXPECT_STREQ(c.c_str(), "hi!");
}

TEST(StringConcat, CharPlusString)
{
    String b("ello");
    String c = 'h' + b;
    EXPECT_STREQ(c.c_str(), "hello");
}

// ============================================================================
// Validate
// ============================================================================

TEST(StringValidation, ValidateSSO)
{
    String s("hello");
    EXPECT_TRUE(s.Validate());
}

TEST(StringValidation, ValidateHeap)
{
    String s("This is a very long string that goes to heap");
    EXPECT_TRUE(s.Validate());
}

TEST(StringValidation, ValidateEmpty)
{
    String s;
    EXPECT_TRUE(s.Validate());
}

// ============================================================================
// Hash<String>
// ============================================================================

TEST(StringHash, ConsistentHash)
{
    Hash<String> h;
    String s("hello");
    EXPECT_EQ(h(s), h(String("hello")));
}

TEST(StringHash, DifferentStringsDifferentHash)
{
    Hash<String> h;
    EXPECT_NE(h(String("hello")), h(String("world")));
}

TEST(StringHash, EmptyStringHash)
{
    Hash<String> h;
    size_t result = h(String());
    // Should be FNV offset basis
    (void)result;
}

// ============================================================================
// WString Construction
// ============================================================================

TEST(WStringConstruction, Default)
{
    WString ws;
    EXPECT_EQ(ws.Size(), 0u);
    EXPECT_TRUE(ws.Empty());
    EXPECT_EQ(ws.c_str()[0], L'\0');
}

TEST(WStringConstruction, FromWCharPtr)
{
    WString ws(L"hello");
    EXPECT_EQ(ws.Size(), 5u);
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello"), 0);
}

TEST(WStringConstruction, FromNullPtr)
{
    WString ws(nullptr);
    EXPECT_TRUE(ws.Empty());
}

TEST(WStringConstruction, FromWCharPtrWithLen)
{
    WString ws(L"hello world", 5);
    EXPECT_EQ(ws.Size(), 5u);
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello"), 0);
}

TEST(WStringConstruction, FromCharRepeat)
{
    WString ws(5, L'x');
    EXPECT_EQ(ws.Size(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(ws[i], L'x');
}

TEST(WStringConstruction, FromStdWString)
{
    gx::WString std_ws = L"wide string";
    WString ws(std_ws);
    EXPECT_EQ(ws.Size(), 11u);
}

TEST(WStringConstruction, FromWStringView)
{
    std::wstring_view sv = L"view";
    WString ws(sv);
    EXPECT_EQ(ws.Size(), 4u);
}

TEST(WStringConstruction, CopyConstructor)
{
    WString ws1(L"hello");
    WString ws2(ws1);
    EXPECT_EQ(ws2.Size(), 5u);
    EXPECT_EQ(std::wcscmp(ws2.c_str(), L"hello"), 0);
}

TEST(WStringConstruction, MoveConstructor)
{
    WString ws1(L"hello");
    WString ws2(std::move(ws1));
    EXPECT_EQ(ws2.Size(), 5u);
    EXPECT_TRUE(ws1.Empty());
}

// ============================================================================
// WString Element Access / Size
// ============================================================================

TEST(WStringAccess, CStr)
{
    WString ws(L"test");
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"test"), 0);
}

TEST(WStringAccess, Size)
{
    WString ws(L"abc");
    EXPECT_EQ(ws.Size(), 3u);
}

TEST(WStringAccess, Length)
{
    WString ws(L"abc");
    EXPECT_EQ(ws.Length(), ws.Size());
}

TEST(WStringAccess, OperatorBracket)
{
    WString ws(L"abc");
    EXPECT_EQ(ws[0], L'a');
    EXPECT_EQ(ws[2], L'c');
}

TEST(WStringAccess, CapacitySSO)
{
    WString ws(L"short");
    EXPECT_EQ(ws.Capacity(), WString::k_SSOCapacity);
}

// ============================================================================
// WString Append
// ============================================================================

TEST(WStringAppend, AppendWCharPtr)
{
    WString ws(L"hello");
    ws.append(L" world");
    EXPECT_EQ(ws.Size(), 11u);
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello world"), 0);
}

TEST(WStringAppend, AppendWString)
{
    WString ws(L"hello");
    WString other(L" world");
    ws.append(other);
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello world"), 0);
}

TEST(WStringAppend, OperatorPlusEqualsWString)
{
    WString ws(L"hello");
    WString other(L" world");
    ws += other;
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello world"), 0);
}

TEST(WStringAppend, OperatorPlusEqualsWCharPtr)
{
    WString ws(L"hello");
    ws += L" world";
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello world"), 0);
}

TEST(WStringAppend, OperatorPlusEqualsWChar)
{
    WString ws(L"ab");
    ws += L'c';
    EXPECT_EQ(ws.Size(), 3u);
    EXPECT_EQ(ws[2], L'c');
}

// ============================================================================
// WString Comparison
// ============================================================================

TEST(WStringComparison, Equal)
{
    WString a(L"hello");
    WString b(L"hello");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(WStringComparison, NotEqual)
{
    WString a(L"hello");
    WString b(L"world");
    EXPECT_TRUE(a != b);
}

TEST(WStringComparison, LessThan)
{
    WString a(L"abc");
    WString b(L"abd");
    EXPECT_TRUE(a < b);
}

TEST(WStringComparison, EqualWCharPtr)
{
    WString ws(L"hello");
    EXPECT_TRUE(ws == L"hello");
    EXPECT_TRUE(L"hello" == ws);
    EXPECT_FALSE(ws == L"world");
}

TEST(WStringComparison, NotEqualWCharPtr)
{
    WString ws(L"hello");
    EXPECT_TRUE(ws != L"world");
    EXPECT_TRUE(L"world" != ws);
}

// ============================================================================
// WString Misc
// ============================================================================

TEST(WStringMisc, Clear)
{
    WString ws(L"hello");
    ws.clear();
    EXPECT_TRUE(ws.Empty());
}

TEST(WStringMisc, Resize)
{
    WString ws(L"hi");
    ws.resize(5, L'x');
    EXPECT_EQ(ws.Size(), 5u);
    EXPECT_EQ(ws[2], L'x');
}

TEST(WStringMisc, Swap)
{
    WString a(L"aaa");
    WString b(L"bbb");
    a.Swap(b);
    EXPECT_EQ(std::wcscmp(a.c_str(), L"bbb"), 0);
    EXPECT_EQ(std::wcscmp(b.c_str(), L"aaa"), 0);
}

TEST(WStringMisc, ADLSwap)
{
    WString a(L"aaa");
    WString b(L"bbb");
    swap(a, b);
    EXPECT_EQ(std::wcscmp(a.c_str(), L"bbb"), 0);
}

TEST(WStringMisc, ToStdWString)
{
    WString ws(L"hello");
    gx::WString s = ws.ToStdWString();
    EXPECT_EQ(s, L"hello");
}

TEST(WStringMisc, ToWStringView)
{
    WString ws(L"hello");
    std::wstring_view sv = ws.ToWStringView();
    EXPECT_EQ(sv, L"hello");
}

TEST(WStringMisc, Validate)
{
    WString ws(L"hello");
    EXPECT_TRUE(ws.Validate());
}

TEST(WStringMisc, Concatenation)
{
    WString a(L"hello");
    WString b(L" world");
    WString c = a + b;
    EXPECT_EQ(std::wcscmp(c.c_str(), L"hello world"), 0);
}

TEST(WStringMisc, ConcatWithWCharPtr)
{
    WString a(L"hello");
    WString c = a + L" world";
    EXPECT_EQ(std::wcscmp(c.c_str(), L"hello world"), 0);
}

TEST(WStringMisc, ConcatWCharPtrPlusWString)
{
    WString b(L" world");
    WString c = L"hello" + b;
    EXPECT_EQ(std::wcscmp(c.c_str(), L"hello world"), 0);
}

// ============================================================================
// WString Hash
// ============================================================================

TEST(WStringHash, ConsistentHash)
{
    Hash<WString> h;
    WString ws(L"hello");
    EXPECT_EQ(h(ws), h(WString(L"hello")));
}

TEST(WStringHash, DifferentStringsDifferentHash)
{
    Hash<WString> h;
    EXPECT_NE(h(WString(L"hello")), h(WString(L"world")));
}

// ============================================================================
// WString Assignment
// ============================================================================

TEST(WStringAssignment, CopyAssignment)
{
    WString ws1(L"hello");
    WString ws2;
    ws2 = ws1;
    EXPECT_EQ(std::wcscmp(ws2.c_str(), L"hello"), 0);
}

TEST(WStringAssignment, MoveAssignment)
{
    WString ws1(L"hello");
    WString ws2;
    ws2 = std::move(ws1);
    EXPECT_EQ(std::wcscmp(ws2.c_str(), L"hello"), 0);
    EXPECT_TRUE(ws1.Empty());
}

TEST(WStringAssignment, WCharPtrAssignment)
{
    WString ws;
    ws = L"hello";
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello"), 0);
}

TEST(WStringAssignment, SelfAssignment)
{
    WString ws(L"hello");
    ws = ws;
    EXPECT_EQ(std::wcscmp(ws.c_str(), L"hello"), 0);
}
