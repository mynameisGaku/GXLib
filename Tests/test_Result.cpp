/// @file test_Result.cpp
/// @brief Result<T> 型のユニットテスト

#include <gtest/gtest.h>
#include "Core/Result.h"
using namespace gx;

// ============================================================================
// ErrorCode / Error
// ============================================================================

TEST(ErrorCodeTest, NoneIsZero)
{
    EXPECT_EQ(static_cast<uint32_t>(ErrorCode::None), 0u);
}

TEST(ErrorCodeTest, UnknownIs0xFFFF)
{
    EXPECT_EQ(static_cast<uint32_t>(ErrorCode::Unknown), 0xFFFFu);
}

TEST(ErrorTest, DefaultConstruction)
{
    Error err;
    EXPECT_EQ(err.code, ErrorCode::None);
    EXPECT_TRUE(err.message.empty());
    EXPECT_FALSE(static_cast<bool>(err));
}

TEST(ErrorTest, ConstructWithCode)
{
    Error err(ErrorCode::FileNotFound);
    EXPECT_EQ(err.code, ErrorCode::FileNotFound);
    EXPECT_TRUE(err.message.empty());
    EXPECT_TRUE(static_cast<bool>(err));
}

TEST(ErrorTest, ConstructWithCodeAndMessage)
{
    Error err(ErrorCode::IOError, "disk full");
    EXPECT_EQ(err.code, ErrorCode::IOError);
    EXPECT_EQ(err.message, "disk full");
}

TEST(ErrorTest, ComparisonOperators)
{
    Error err(ErrorCode::Timeout);
    EXPECT_TRUE(err == ErrorCode::Timeout);
    EXPECT_FALSE(err == ErrorCode::None);
    EXPECT_TRUE(err != ErrorCode::None);
    EXPECT_FALSE(err != ErrorCode::Timeout);
}

// ============================================================================
// Result<T> - Construction
// ============================================================================

TEST(ResultTest, ConstructFromValue)
{
    Result<int> r(42);
    EXPECT_TRUE(r.IsOk());
    EXPECT_FALSE(r.IsError());
    EXPECT_EQ(r.Value(), 42);
}

TEST(ResultTest, ConstructFromMoveValue)
{
    gx::String str("hello");
    Result<gx::String> r(std::move(str));
    EXPECT_TRUE(r.IsOk());
    EXPECT_EQ(r.Value(), "hello");
}

TEST(ResultTest, ConstructFromError)
{
    Error err(ErrorCode::FileNotFound, "not found");
    Result<int> r(err);
    EXPECT_TRUE(r.IsError());
    EXPECT_FALSE(r.IsOk());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::FileNotFound);
    EXPECT_EQ(r.GetError().message, "not found");
}

TEST(ResultTest, ConstructFromMoveError)
{
    Error err(ErrorCode::NetworkError, "timeout");
    Result<int> r(std::move(err));
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::NetworkError);
}

TEST(ResultTest, ConstructFromErrorCode)
{
    Result<int> r(ErrorCode::InvalidArgument, "bad arg");
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::InvalidArgument);
    EXPECT_EQ(r.GetError().message, "bad arg");
}

TEST(ResultTest, ConstructFromErrorCodeOnly)
{
    Result<int> r(ErrorCode::OutOfMemory);
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::OutOfMemory);
}

// ============================================================================
// Result<T> - Bool conversion
// ============================================================================

TEST(ResultTest, BoolConversionSuccess)
{
    Result<int> r(10);
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(ResultTest, BoolConversionError)
{
    Result<int> r(ErrorCode::Unknown);
    EXPECT_FALSE(static_cast<bool>(r));
}

// ============================================================================
// Result<T> - Value access
// ============================================================================

TEST(ResultTest, ValueAccess)
{
    Result<int> r(100);
    EXPECT_EQ(r.Value(), 100);
    EXPECT_EQ(*r, 100);
}

TEST(ResultTest, ValueMutability)
{
    Result<int> r(10);
    r.Value() = 20;
    EXPECT_EQ(r.Value(), 20);
}

TEST(ResultTest, DereferenceOperator)
{
    Result<int> r(55);
    EXPECT_EQ(*r, 55);
    *r = 66;
    EXPECT_EQ(*r, 66);
}

TEST(ResultTest, ArrowOperator)
{
    struct Foo { int x = 5; };
    Result<Foo> r(Foo{10});
    EXPECT_EQ(r->x, 10);
}

TEST(ResultTest, ConstArrowOperator)
{
    struct Foo { int x = 7; };
    const Result<Foo> r(Foo{7});
    EXPECT_EQ(r->x, 7);
}

TEST(ResultTest, ConstValueAccess)
{
    const Result<int> r(42);
    EXPECT_EQ(r.Value(), 42);
    EXPECT_EQ(*r, 42);
}

// ============================================================================
// Result<T> - ValueOr
// ============================================================================

TEST(ResultTest, ValueOrSuccess)
{
    Result<int> r(42);
    EXPECT_EQ(r.ValueOr(0), 42);
}

TEST(ResultTest, ValueOrError)
{
    Result<int> r(ErrorCode::FileNotFound);
    EXPECT_EQ(r.ValueOr(-1), -1);
}

TEST(ResultTest, ValueOrString)
{
    Result<gx::String> r(ErrorCode::ParseError);
    EXPECT_EQ(r.ValueOr("default"), "default");
}

// ============================================================================
// Result<T> - Static helpers
// ============================================================================

TEST(ResultTest, StaticOk)
{
    auto r = Result<int>::Ok(99);
    EXPECT_TRUE(r.IsOk());
    EXPECT_EQ(r.Value(), 99);
}

TEST(ResultTest, StaticOkMove)
{
    gx::String str("moved");
    auto r = Result<gx::String>::Ok(std::move(str));
    EXPECT_TRUE(r.IsOk());
    EXPECT_EQ(r.Value(), "moved");
}

TEST(ResultTest, StaticErr)
{
    auto r = Result<int>::Err(ErrorCode::PermissionDenied, "no access");
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::PermissionDenied);
    EXPECT_EQ(r.GetError().message, "no access");
}

TEST(ResultTest, StaticErrCodeOnly)
{
    auto r = Result<int>::Err(ErrorCode::Timeout);
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::Timeout);
    EXPECT_TRUE(r.GetError().message.empty());
}

// ============================================================================
// Result<T> - Error accessors on error result
// ============================================================================

TEST(ResultTest, GetErrorOnError)
{
    Result<int> r(ErrorCode::ConnectionRefused, "refused");
    EXPECT_EQ(r.GetError().code, ErrorCode::ConnectionRefused);
    EXPECT_EQ(r.GetError().message, "refused");
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::ConnectionRefused);
}

TEST(ResultTest, GetErrorOnSuccess)
{
    Result<int> r(42);
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::None);
}

// ============================================================================
// Result<T> - Copy/Move semantics
// ============================================================================

TEST(ResultTest, CopySuccess)
{
    Result<int> r1(42);
    Result<int> r2 = r1;
    EXPECT_TRUE(r2.IsOk());
    EXPECT_EQ(r2.Value(), 42);
    EXPECT_TRUE(r1.IsOk());
    EXPECT_EQ(r1.Value(), 42);
}

TEST(ResultTest, CopyError)
{
    Result<int> r1(ErrorCode::IOError, "io");
    Result<int> r2 = r1;
    EXPECT_TRUE(r2.IsError());
    EXPECT_EQ(r2.GetErrorCode(), ErrorCode::IOError);
    EXPECT_EQ(r2.GetError().message, "io");
}

TEST(ResultTest, MoveSuccess)
{
    Result<gx::String> r1(gx::String("hello"));
    Result<gx::String> r2 = std::move(r1);
    EXPECT_TRUE(r2.IsOk());
    EXPECT_EQ(r2.Value(), "hello");
}

TEST(ResultTest, MoveError)
{
    Result<gx::String> r1(ErrorCode::EncryptionError, "enc fail");
    Result<gx::String> r2 = std::move(r1);
    EXPECT_TRUE(r2.IsError());
    EXPECT_EQ(r2.GetErrorCode(), ErrorCode::EncryptionError);
}

TEST(ResultTest, CopyAssignSuccess)
{
    Result<int> r1(10);
    Result<int> r2(ErrorCode::Unknown);
    r2 = r1;
    EXPECT_TRUE(r2.IsOk());
    EXPECT_EQ(r2.Value(), 10);
}

TEST(ResultTest, MoveAssignSuccess)
{
    Result<int> r1(10);
    Result<int> r2(ErrorCode::Unknown);
    r2 = std::move(r1);
    EXPECT_TRUE(r2.IsOk());
    EXPECT_EQ(r2.Value(), 10);
}

// ============================================================================
// Result<T> - Various types
// ============================================================================

TEST(ResultTest, FloatValue)
{
    Result<float> r(3.14f);
    EXPECT_TRUE(r.IsOk());
    EXPECT_FLOAT_EQ(r.Value(), 3.14f);
}

TEST(ResultTest, VectorValue)
{
    gx::Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    Result<gx::Vector<int>> r(std::move(vec));
    EXPECT_TRUE(r.IsOk());
    EXPECT_EQ(r.Value().size(), 3u);
    EXPECT_EQ(r.Value()[0], 1);
}

TEST(ResultTest, AllErrorCodes)
{
    // Verify each error code can be used
    auto test = [](ErrorCode code) {
        Result<int> r(code);
        EXPECT_TRUE(r.IsError());
        EXPECT_EQ(r.GetErrorCode(), code);
    };
    test(ErrorCode::FileNotFound);
    test(ErrorCode::PermissionDenied);
    test(ErrorCode::InvalidFormat);
    test(ErrorCode::OutOfMemory);
    test(ErrorCode::NetworkError);
    test(ErrorCode::ConnectionRefused);
    test(ErrorCode::ConnectionClosed);
    test(ErrorCode::Timeout);
    test(ErrorCode::InvalidArgument);
    test(ErrorCode::NotInitialized);
    test(ErrorCode::AlreadyExists);
    test(ErrorCode::NotFound);
    test(ErrorCode::IOError);
    test(ErrorCode::ParseError);
    test(ErrorCode::EncryptionError);
    test(ErrorCode::BufferOverflow);
    test(ErrorCode::Unknown);
}

// ============================================================================
// Result<void> specialization
// ============================================================================

TEST(ResultVoidTest, DefaultSuccess)
{
    Result<void> r;
    EXPECT_TRUE(r.IsOk());
    EXPECT_FALSE(r.IsError());
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(ResultVoidTest, StaticOk)
{
    auto r = Result<void>::Ok();
    EXPECT_TRUE(r.IsOk());
}

TEST(ResultVoidTest, ConstructFromError)
{
    Error err(ErrorCode::FileNotFound, "missing");
    Result<void> r(err);
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::FileNotFound);
    EXPECT_EQ(r.GetError().message, "missing");
}

TEST(ResultVoidTest, ConstructFromMoveError)
{
    Error err(ErrorCode::Timeout, "timed out");
    Result<void> r(std::move(err));
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::Timeout);
}

TEST(ResultVoidTest, ConstructFromErrorCode)
{
    Result<void> r(ErrorCode::PermissionDenied, "denied");
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::PermissionDenied);
}

TEST(ResultVoidTest, StaticErr)
{
    auto r = Result<void>::Err(ErrorCode::IOError, "write failed");
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::IOError);
    EXPECT_EQ(r.GetError().message, "write failed");
}

TEST(ResultVoidTest, StaticErrCodeOnly)
{
    auto r = Result<void>::Err(ErrorCode::NotInitialized);
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::NotInitialized);
    EXPECT_TRUE(r.GetError().message.empty());
}

TEST(ResultVoidTest, BoolConversion)
{
    Result<void> ok;
    Result<void> err(ErrorCode::Unknown);
    EXPECT_TRUE(static_cast<bool>(ok));
    EXPECT_FALSE(static_cast<bool>(err));
}

TEST(ResultVoidTest, CopySuccess)
{
    Result<void> r1;
    Result<void> r2 = r1;
    EXPECT_TRUE(r2.IsOk());
}

TEST(ResultVoidTest, CopyError)
{
    Result<void> r1(ErrorCode::AlreadyExists, "dup");
    Result<void> r2 = r1;
    EXPECT_TRUE(r2.IsError());
    EXPECT_EQ(r2.GetErrorCode(), ErrorCode::AlreadyExists);
    EXPECT_EQ(r2.GetError().message, "dup");
}

TEST(ResultVoidTest, MoveSuccess)
{
    Result<void> r1;
    Result<void> r2 = std::move(r1);
    EXPECT_TRUE(r2.IsOk());
}

TEST(ResultVoidTest, MoveError)
{
    Result<void> r1(ErrorCode::BufferOverflow, "overflow");
    Result<void> r2 = std::move(r1);
    EXPECT_TRUE(r2.IsError());
    EXPECT_EQ(r2.GetErrorCode(), ErrorCode::BufferOverflow);
}

TEST(ResultVoidTest, GetErrorOnSuccess)
{
    Result<void> r;
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::None);
}

// ============================================================================
// Result<T> - Function return pattern
// ============================================================================

namespace {

Result<int> ParseInt(const gx::String& str)
{
    if (str.empty())
        return Result<int>::Err(ErrorCode::InvalidArgument, "empty string");
    // Simple check: only digits
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] < '0' || str[i] > '9')
            return Result<int>::Err(ErrorCode::ParseError, "non-digit character");
    }
    int value = 0;
    for (size_t i = 0; i < str.size(); ++i)
        value = value * 10 + (str[i] - '0');
    return Result<int>::Ok(value);
}

Result<void> ValidateRange(int value, int lo, int hi)
{
    if (value < lo || value > hi)
        return Result<void>::Err(ErrorCode::InvalidArgument, "out of range");
    return Result<void>::Ok();
}

} // anonymous namespace

TEST(ResultPatternTest, ParseIntSuccess)
{
    auto r = ParseInt("123");
    EXPECT_TRUE(r.IsOk());
    EXPECT_EQ(r.Value(), 123);
}

TEST(ResultPatternTest, ParseIntEmpty)
{
    auto r = ParseInt("");
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::InvalidArgument);
}

TEST(ResultPatternTest, ParseIntBadChar)
{
    auto r = ParseInt("12x");
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::ParseError);
}

TEST(ResultPatternTest, ValidateRangeOk)
{
    auto r = ValidateRange(5, 0, 10);
    EXPECT_TRUE(r.IsOk());
}

TEST(ResultPatternTest, ValidateRangeOutOfBounds)
{
    auto r = ValidateRange(15, 0, 10);
    EXPECT_TRUE(r.IsError());
    EXPECT_EQ(r.GetErrorCode(), ErrorCode::InvalidArgument);
}
