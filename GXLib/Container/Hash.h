#pragma once
/// @file Hash.h
/// @brief ハッシュ関数 — FNV-1a, MurmurHash3, xxHash, Hash<T>特殊化

#include "pch_common.h"
#include <cstring>
#include <string_view>
#include <typeindex>

namespace gx::container {

// ============================================================================
// FNV-1a
// ============================================================================

__forceinline constexpr size_t FNV1a(const void* data, size_t length) noexcept
{
    if constexpr (sizeof(size_t) == 8)
    {
        size_t hash = 14695981039346656037ULL;
        auto p = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < length; ++i)
        {
            hash ^= static_cast<size_t>(p[i]);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
    else
    {
        size_t hash = 2166136261U;
        auto p = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < length; ++i)
        {
            hash ^= static_cast<size_t>(p[i]);
            hash *= 16777619U;
        }
        return hash;
    }
}

// ============================================================================
// MurmurHash3 (32-bit finalizer for size_t)
// ============================================================================

__forceinline constexpr size_t MurmurHash3Mix(size_t h) noexcept
{
    if constexpr (sizeof(size_t) == 8)
    {
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
    }
    else
    {
        h ^= h >> 16;
        h *= 0x85ebca6bU;
        h ^= h >> 13;
        h *= 0xc2b2ae35U;
        h ^= h >> 16;
    }
    return h;
}

// ============================================================================
// xxHash-style fast hash for integral types
// ============================================================================

__forceinline constexpr size_t HashCombine(size_t seed, size_t value) noexcept
{
    // boost::hash_combine equivalent
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

// ============================================================================
// Hash<T> テンプレート (std::hash 互換 + 拡張)
// ============================================================================

template <typename T>
struct Hash;

// Integral types
template <> struct Hash<bool>     { __forceinline size_t operator()(bool v) const noexcept { return static_cast<size_t>(v); } };
template <> struct Hash<char>     { __forceinline size_t operator()(char v) const noexcept { return static_cast<size_t>(v); } };
template <> struct Hash<int8_t>   { __forceinline size_t operator()(int8_t v) const noexcept { return static_cast<size_t>(v); } };
template <> struct Hash<uint8_t>  { __forceinline size_t operator()(uint8_t v) const noexcept { return static_cast<size_t>(v); } };
template <> struct Hash<int16_t>  { __forceinline size_t operator()(int16_t v) const noexcept { return static_cast<size_t>(v); } };
template <> struct Hash<uint16_t> { __forceinline size_t operator()(uint16_t v) const noexcept { return static_cast<size_t>(v); } };
template <> struct Hash<int32_t>  { __forceinline size_t operator()(int32_t v) const noexcept { return MurmurHash3Mix(static_cast<size_t>(v)); } };
template <> struct Hash<uint32_t> { __forceinline size_t operator()(uint32_t v) const noexcept { return MurmurHash3Mix(static_cast<size_t>(v)); } };
template <> struct Hash<int64_t>  { __forceinline size_t operator()(int64_t v) const noexcept { return MurmurHash3Mix(static_cast<size_t>(v)); } };
template <> struct Hash<uint64_t> { __forceinline size_t operator()(uint64_t v) const noexcept { return MurmurHash3Mix(static_cast<size_t>(v)); } };

// wchar_t (distinct type on Windows, 16-bit)
template <> struct Hash<wchar_t>  { __forceinline size_t operator()(wchar_t v) const noexcept { return static_cast<size_t>(v); } };

// Floating point
template <> struct Hash<float>
{
    __forceinline size_t operator()(float v) const noexcept
    {
        uint32_t bits;
        std::memcpy(&bits, &v, sizeof(bits));
        return MurmurHash3Mix(static_cast<size_t>(bits));
    }
};

template <> struct Hash<double>
{
    __forceinline size_t operator()(double v) const noexcept
    {
        uint64_t bits;
        std::memcpy(&bits, &v, sizeof(bits));
        return MurmurHash3Mix(static_cast<size_t>(bits));
    }
};

// Pointer
template <typename T>
struct Hash<T*>
{
    __forceinline size_t operator()(T* v) const noexcept
    {
        return MurmurHash3Mix(reinterpret_cast<size_t>(v));
    }
};

// String types
template <>
struct Hash<std::string>
{
    __forceinline size_t operator()(const std::string& s) const noexcept
    {
        return FNV1a(s.data(), s.size());
    }
};

template <>
struct Hash<std::wstring>
{
    __forceinline size_t operator()(const std::wstring& s) const noexcept
    {
        return FNV1a(s.data(), s.size() * sizeof(wchar_t));
    }
};

template <>
struct Hash<std::string_view>
{
    __forceinline size_t operator()(std::string_view s) const noexcept
    {
        return FNV1a(s.data(), s.size());
    }
};

template <>
struct Hash<std::wstring_view>
{
    __forceinline size_t operator()(std::wstring_view s) const noexcept
    {
        return FNV1a(s.data(), s.size() * sizeof(wchar_t));
    }
};

template <>
struct Hash<const char*>
{
    size_t operator()(const char* s) const noexcept
    {
        return s ? FNV1a(s, std::strlen(s)) : 0;
    }
};

template <>
struct Hash<const wchar_t*>
{
    size_t operator()(const wchar_t* s) const noexcept
    {
        return s ? FNV1a(s, std::wcslen(s) * sizeof(wchar_t)) : 0;
    }
};

template <>
struct Hash<std::type_index>
{
    __forceinline size_t operator()(const std::type_index& ti) const noexcept
    {
        return ti.hash_code();
    }
};

/// @brief 汎用等価比較 (std::equal_to 相当)
template <typename T = void>
struct EqualTo
{
    __forceinline constexpr bool operator()(const T& a, const T& b) const noexcept { return a == b; }
};

template <>
struct EqualTo<void>
{
    template <typename A, typename B>
    __forceinline constexpr bool operator()(const A& a, const B& b) const noexcept { return a == b; }
};

/// @brief 汎用比較 (std::less 相当)
template <typename T = void>
struct Less
{
    __forceinline constexpr bool operator()(const T& a, const T& b) const noexcept { return a < b; }
};

template <>
struct Less<void>
{
    template <typename A, typename B>
    __forceinline constexpr bool operator()(const A& a, const B& b) const noexcept { return a < b; }
};

// ============================================================================
// キー抽出ファンクタ
// ============================================================================

/// @brief ペアからキーを抽出 (Map/HashMap 用)
template <typename Pair>
struct PairExtractKey
{
    using key_type = typename Pair::first_type;
    __forceinline const key_type& operator()(const Pair& p) const noexcept { return p.first; }
};

/// @brief 値そのものがキー (Set/HashSet 用)
template <typename T>
struct IdentityExtractKey
{
    using key_type = T;
    __forceinline const T& operator()(const T& v) const noexcept { return v; }
};

} // namespace gx::container
