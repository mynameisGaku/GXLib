#pragma once
/// @file TypeTraits.h
/// @brief 型特性 — trivially_relocatable判定、memcpy最適化ユーティリティ

#include "pch_common.h"
#include <type_traits>
#include <cstring>

namespace gx::container {

/// @brief trivially relocatable (memcpy で安全にムーブ+デストラクト可能)
/// デフォルトは trivially_copyable なら true
template <typename T>
struct is_trivially_relocatable
    : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <typename T>
inline constexpr bool is_trivially_relocatable_v = is_trivially_relocatable<T>::value;

/// ユーザー型を trivially relocatable と宣言するマクロ
#define GX_DECLARE_TRIVIALLY_RELOCATABLE(T) \
    namespace gx::container { \
    template <> struct is_trivially_relocatable<T> : std::true_type {}; \
    }

/// @brief 範囲を未初期化領域にリロケート (move + destroy, or memcpy)
template <typename T>
__forceinline void uninitialized_relocate(T* dest, T* src, size_t count) noexcept
{
    if constexpr (is_trivially_relocatable_v<T>)
    {
        if (count > 0)
            std::memcpy(dest, src, count * sizeof(T));
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
        {
            ::new (static_cast<void*>(dest + i)) T(std::move(src[i]));
            src[i].~T();
        }
    }
}

/// @brief デフォルト構築で範囲を初期化
template <typename T>
__forceinline void uninitialized_default_construct(T* dest, size_t count)
{
    if constexpr (std::is_trivially_default_constructible_v<T>)
    {
        if (count > 0)
            std::memset(dest, 0, count * sizeof(T));
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
            ::new (static_cast<void*>(dest + i)) T();
    }
}

/// @brief 値コピーで範囲を初期化
template <typename T>
__forceinline void uninitialized_fill(T* dest, size_t count, const T& value)
{
    for (size_t i = 0; i < count; ++i)
        ::new (static_cast<void*>(dest + i)) T(value);
}

/// @brief コピー構築で範囲を初期化
template <typename T>
__forceinline void uninitialized_copy(T* dest, const T* src, size_t count)
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        if (count > 0)
            std::memcpy(dest, src, count * sizeof(T));
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
            ::new (static_cast<void*>(dest + i)) T(src[i]);
    }
}

/// @brief 範囲をデストラクト (trivially_destructible なら no-op)
template <typename T>
__forceinline void destroy_range(T* ptr, size_t count) noexcept
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < count; ++i)
            ptr[i].~T();
    }
}

/// @brief ムーブで範囲をコピー
template <typename T>
__forceinline void uninitialized_move(T* dest, T* src, size_t count) noexcept
{
    if constexpr (is_trivially_relocatable_v<T>)
    {
        if (count > 0)
            std::memcpy(dest, src, count * sizeof(T));
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
            ::new (static_cast<void*>(dest + i)) T(std::move(src[i]));
    }
}

} // namespace gx::container
