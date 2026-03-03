#pragma once
/// @file Array.h
/// @brief 固定長配列 (constexpr対応)

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定サイズ配列コンテナ (std::array互換、constexpr対応)
/// @tparam T 要素型
/// @tparam N 配列サイズ
template <typename T, size_t N>
class Array
{
public:
    // ========================================================================
    // Type aliases
    // ========================================================================
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 添字アクセス (境界チェックあり assert)
    __forceinline constexpr reference operator[](size_type i) noexcept
    {
        assert(i < N);
        return m_data[i];
    }

    /// @brief 添字アクセス (const)
    __forceinline constexpr const_reference operator[](size_type i) const noexcept
    {
        assert(i < N);
        return m_data[i];
    }

    /// @brief 先頭要素
    __forceinline constexpr reference Front() noexcept
    {
        static_assert(N > 0, "Front() on empty Array");
        return m_data[0];
    }

    /// @brief 先頭要素 (const)
    __forceinline constexpr const_reference Front() const noexcept
    {
        static_assert(N > 0, "Front() on empty Array");
        return m_data[0];
    }

    /// @brief 末尾要素
    __forceinline constexpr reference Back() noexcept
    {
        static_assert(N > 0, "Back() on empty Array");
        return m_data[N - 1];
    }

    /// @brief 末尾要素 (const)
    __forceinline constexpr const_reference Back() const noexcept
    {
        static_assert(N > 0, "Back() on empty Array");
        return m_data[N - 1];
    }

    /// @brief 生ポインタ取得
    __forceinline constexpr pointer Data() noexcept { return m_data; }

    /// @brief 生ポインタ取得 (const)
    __forceinline constexpr const_pointer Data() const noexcept { return m_data; }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline constexpr iterator       begin()        noexcept { return m_data; }
    __forceinline constexpr const_iterator begin()  const noexcept { return m_data; }
    __forceinline constexpr const_iterator cbegin() const noexcept { return m_data; }
    __forceinline constexpr iterator       end()          noexcept { return m_data + N; }
    __forceinline constexpr const_iterator end()    const noexcept { return m_data + N; }
    __forceinline constexpr const_iterator cend()   const noexcept { return m_data + N; }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 要素数
    __forceinline constexpr size_type Size() const noexcept { return N; }

    /// @brief 最大要素数 (= Size)
    __forceinline constexpr size_type MaxSize() const noexcept { return N; }

    /// @brief 空かどうか
    __forceinline constexpr bool Empty() const noexcept { return N == 0; }

    // ========================================================================
    // Operations
    // ========================================================================

    /// @brief 全要素を指定値で埋める
    constexpr void Fill(const T& value)
    {
        for (size_type i = 0; i < N; ++i)
            m_data[i] = value;
    }

    /// @brief 他のArrayとスワップ
    constexpr void Swap(Array& other) noexcept(std::is_nothrow_swappable_v<T>)
    {
        for (size_type i = 0; i < N; ++i)
            std::swap(m_data[i], other.m_data[i]);
    }

    // ========================================================================
    // std compatibility aliases
    // ========================================================================

    __forceinline constexpr size_type size() const noexcept { return Size(); }
    __forceinline constexpr bool empty() const noexcept { return Empty(); }
    __forceinline constexpr reference front() noexcept { return Front(); }
    __forceinline constexpr const_reference front() const noexcept { return Front(); }
    __forceinline constexpr reference back() noexcept { return Back(); }
    __forceinline constexpr const_reference back() const noexcept { return Back(); }
    __forceinline constexpr pointer data() noexcept { return Data(); }
    __forceinline constexpr const_pointer data() const noexcept { return Data(); }
    __forceinline constexpr reference at(size_type i) { assert(i < N); return m_data[i]; }
    __forceinline constexpr const_reference at(size_type i) const { assert(i < N); return m_data[i]; }
    constexpr void fill(const T& value) { Fill(value); }
    constexpr void swap(Array& other) noexcept(std::is_nothrow_swappable_v<T>) { Swap(other); }
    __forceinline constexpr size_type max_size() const noexcept { return N; }

    // ========================================================================
    // Validation (EASTL extension)
    // ========================================================================

    /// @brief コンテナの内部状態を検証 (固定配列は常に有効)
    bool Validate() const { return true; }

    // ========================================================================
    // Comparison
    // ========================================================================

    constexpr friend bool operator==(const Array& a, const Array& b)
    {
        for (size_type i = 0; i < N; ++i)
            if (!(a.m_data[i] == b.m_data[i])) return false;
        return true;
    }

    constexpr friend bool operator!=(const Array& a, const Array& b)
    {
        return !(a == b);
    }

    constexpr friend bool operator<(const Array& a, const Array& b)
    {
        for (size_type i = 0; i < N; ++i)
        {
            if (a.m_data[i] < b.m_data[i]) return true;
            if (b.m_data[i] < a.m_data[i]) return false;
        }
        return false;
    }

    constexpr friend bool operator>(const Array& a, const Array& b)  { return b < a; }
    constexpr friend bool operator<=(const Array& a, const Array& b) { return !(b < a); }
    constexpr friend bool operator>=(const Array& a, const Array& b) { return !(a < b); }

    // ========================================================================
    // Data (public for aggregate initialization)
    // ========================================================================

    T m_data[N];
};

/// @brief Array のスワップ (ADL用)
template <typename T, size_t N>
constexpr void swap(Array<T, N>& a, Array<T, N>& b) noexcept(noexcept(a.Swap(b)))
{
    a.Swap(b);
}

} // namespace gx::container
