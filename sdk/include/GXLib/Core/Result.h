#pragma once
/// @file Result.h
/// @brief 値またはエラーを保持する型 (std::expected 相当)
///
/// Rust の Result<T, E> や C++23 の std::expected<T, E> に相当する型。
/// 関数の戻り値として成功値またはエラー情報を返す。
/// @addtogroup grp_core
/// @{

#include "Container/ContainerFwd.h"

namespace gx
{

/// @brief エラーコード
enum class ErrorCode : uint32_t
{
    None = 0,
    FileNotFound,
    PermissionDenied,
    InvalidFormat,
    OutOfMemory,
    NetworkError,
    ConnectionRefused,
    ConnectionClosed,
    Timeout,
    InvalidArgument,
    NotInitialized,
    AlreadyExists,
    NotFound,
    IOError,
    ParseError,
    EncryptionError,
    BufferOverflow,
    Unknown = 0xFFFF,
};

/// @brief エラー情報
struct Error
{
    ErrorCode   code = ErrorCode::None;
    gx::String  message;

    Error() = default;
    Error(ErrorCode c, const gx::String& msg = {}) : code(c), message(msg) {}

    bool operator==(ErrorCode c) const { return code == c; }
    bool operator!=(ErrorCode c) const { return code != c; }
    explicit operator bool() const { return code != ErrorCode::None; }
};

/// @brief 成功値またはエラーを保持する型
/// @tparam T 成功時の値の型
template<typename T>
class Result
{
public:
    /// 成功で構築
    Result(const T& value) : m_value(value), m_hasValue(true) {}
    Result(T&& value) : m_value(std::move(value)), m_hasValue(true) {}

    /// エラーで構築
    Result(const Error& err) : m_error(err), m_hasValue(false) {}
    Result(Error&& err) : m_error(std::move(err)), m_hasValue(false) {}
    Result(ErrorCode code, const gx::String& msg = {}) : m_error(code, msg), m_hasValue(false) {}

    /// コピー・ムーブ
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    /// 成功判定
    bool IsOk() const { return m_hasValue; }
    bool IsError() const { return !m_hasValue; }
    explicit operator bool() const { return m_hasValue; }

    /// 値取得 (成功時のみ)
    T& Value() { return m_value; }
    const T& Value() const { return m_value; }
    T& operator*() { return m_value; }
    const T& operator*() const { return m_value; }
    T* operator->() { return &m_value; }
    const T* operator->() const { return &m_value; }

    /// エラー取得
    const Error& GetError() const { return m_error; }
    ErrorCode GetErrorCode() const { return m_error.code; }

    /// 値またはデフォルト値を返す
    T ValueOr(const T& defaultValue) const { return m_hasValue ? m_value : defaultValue; }

    /// 成功ヘルパー
    static Result Ok(const T& value) { return Result(value); }
    static Result Ok(T&& value) { return Result(std::move(value)); }

    /// エラーヘルパー
    static Result Err(ErrorCode code, const gx::String& msg = {}) { return Result(code, msg); }

private:
    T     m_value{};
    Error m_error;
    bool  m_hasValue = false;
};

/// @brief void特殊化 -- 成功/失敗のみ
template<>
class Result<void>
{
public:
    Result() : m_hasValue(true) {}
    Result(const Error& err) : m_error(err), m_hasValue(false) {}
    Result(Error&& err) : m_error(std::move(err)), m_hasValue(false) {}
    Result(ErrorCode code, const gx::String& msg = {}) : m_error(code, msg), m_hasValue(false) {}

    /// コピー・ムーブ
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    bool IsOk() const { return m_hasValue; }
    bool IsError() const { return !m_hasValue; }
    explicit operator bool() const { return m_hasValue; }

    const Error& GetError() const { return m_error; }
    ErrorCode GetErrorCode() const { return m_error.code; }

    static Result Ok() { return Result(); }
    static Result Err(ErrorCode code, const gx::String& msg = {}) { return Result(code, msg); }

private:
    Error m_error;
    bool  m_hasValue = false;
};

} // namespace gx
/// @}
