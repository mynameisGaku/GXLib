#pragma once
/// @file Span.h
/// @brief 非所有ビュー (std::span互換)

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace gx::container {

/// @brief 動的エクステント定数
inline constexpr size_t DynamicExtent = static_cast<size_t>(-1);

/// @brief 連続メモリ上の非所有ビュー
/// @tparam T 要素型
/// @tparam Extent 静的エクステント (DynamicExtent = 動的)
template <typename T, size_t Extent = DynamicExtent>
class Span
{
public:
    // ========================================================================
    // Type aliases
    // ========================================================================
    using element_type    = T;
    using value_type      = std::remove_cv_t<T>;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = T*;
    using const_iterator  = const T*;

    // ========================================================================
    // Constructors
    // ========================================================================

    /// @brief デフォルトコンストラクタ (空ビュー)
    constexpr Span() noexcept : m_data(nullptr), m_size(0) {}

    /// @brief ポインタ + サイズ
    constexpr Span(T* data, size_type count) noexcept : m_data(data), m_size(count) {}

    /// @brief 範囲 [first, last)
    constexpr Span(T* first, T* last) noexcept
        : m_data(first)
        , m_size(static_cast<size_type>(last - first))
    {}

    /// @brief C配列から構築
    template <size_t N>
    constexpr Span(T (&arr)[N]) noexcept : m_data(arr), m_size(N) {}

    /// @brief コンテナから構築 (data() + size() を持つ型)
    template <typename Container,
              typename = std::void_t<
                  decltype(std::declval<Container&>().data()),
                  decltype(std::declval<Container&>().size())>>
    constexpr Span(Container& c) noexcept
        : m_data(c.data())
        , m_size(static_cast<size_type>(c.size()))
    {}

    /// @brief constコンテナから構築
    template <typename Container,
              typename = std::void_t<
                  decltype(std::declval<const Container&>().data()),
                  decltype(std::declval<const Container&>().size())>,
              typename = void>
    constexpr Span(const Container& c) noexcept
        : m_data(c.data())
        , m_size(static_cast<size_type>(c.size()))
    {}

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 生ポインタ取得
    __forceinline constexpr pointer Data() const noexcept { return m_data; }

    /// @brief 要素数
    __forceinline constexpr size_type Size() const noexcept { return m_size; }

    /// @brief バイトサイズ
    __forceinline constexpr size_type SizeBytes() const noexcept { return m_size * sizeof(T); }

    /// @brief 空かどうか
    __forceinline constexpr bool Empty() const noexcept { return m_size == 0; }

    /// @brief 添字アクセス (境界チェックあり assert)
    __forceinline constexpr reference operator[](size_type i) const noexcept
    {
        assert(i < m_size);
        return m_data[i];
    }

    /// @brief 先頭要素
    __forceinline constexpr reference Front() const noexcept
    {
        assert(m_size > 0);
        return m_data[0];
    }

    /// @brief 末尾要素
    __forceinline constexpr reference Back() const noexcept
    {
        assert(m_size > 0);
        return m_data[m_size - 1];
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline constexpr iterator       begin()  const noexcept { return m_data; }
    __forceinline constexpr iterator       end()    const noexcept { return m_data + m_size; }
    __forceinline constexpr const_iterator cbegin() const noexcept { return m_data; }
    __forceinline constexpr const_iterator cend()   const noexcept { return m_data + m_size; }

    // ========================================================================
    // Subviews
    // ========================================================================

    /// @brief 先頭 count 要素のサブビュー
    constexpr Span<T, DynamicExtent> First(size_type count) const noexcept
    {
        assert(count <= m_size);
        return Span<T, DynamicExtent>(m_data, count);
    }

    /// @brief 末尾 count 要素のサブビュー
    constexpr Span<T, DynamicExtent> Last(size_type count) const noexcept
    {
        assert(count <= m_size);
        return Span<T, DynamicExtent>(m_data + m_size - count, count);
    }

    /// @brief 部分ビュー
    constexpr Span<T, DynamicExtent> Subspan(size_type offset, size_type count = DynamicExtent) const noexcept
    {
        assert(offset <= m_size);
        size_type len = (count == DynamicExtent) ? (m_size - offset) : count;
        assert(offset + len <= m_size);
        return Span<T, DynamicExtent>(m_data + offset, len);
    }

private:
    T*        m_data;
    size_type m_size;
};

// ============================================================================
// Deduction guides
// ============================================================================

template <typename T, size_t N>
Span(T (&)[N]) -> Span<T, N>;

template <typename T>
Span(T*, size_t) -> Span<T>;

template <typename T>
Span(T*, T*) -> Span<T>;

} // namespace gx::container
