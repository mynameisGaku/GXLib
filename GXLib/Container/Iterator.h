#pragma once
/// @file Iterator.h
/// @brief コンテナ用イテレータ基盤

#include "pch_common.h"
#include <iterator>

namespace gx::container {

/// @brief 連続メモリイテレータ (C++20 contiguous_iterator_tag)
template <typename T>
class ContiguousIterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    ContiguousIterator() noexcept = default;
    explicit ContiguousIterator(T* ptr) noexcept : m_ptr(ptr) {}

    __forceinline reference operator*() const noexcept { return *m_ptr; }
    __forceinline pointer   operator->() const noexcept { return m_ptr; }
    __forceinline reference operator[](difference_type n) const noexcept { return m_ptr[n]; }

    __forceinline ContiguousIterator& operator++() noexcept { ++m_ptr; return *this; }
    __forceinline ContiguousIterator  operator++(int) noexcept { auto tmp = *this; ++m_ptr; return tmp; }
    __forceinline ContiguousIterator& operator--() noexcept { --m_ptr; return *this; }
    __forceinline ContiguousIterator  operator--(int) noexcept { auto tmp = *this; --m_ptr; return tmp; }

    __forceinline ContiguousIterator& operator+=(difference_type n) noexcept { m_ptr += n; return *this; }
    __forceinline ContiguousIterator& operator-=(difference_type n) noexcept { m_ptr -= n; return *this; }

    __forceinline friend ContiguousIterator operator+(ContiguousIterator it, difference_type n) noexcept { return ContiguousIterator(it.m_ptr + n); }
    __forceinline friend ContiguousIterator operator+(difference_type n, ContiguousIterator it) noexcept { return ContiguousIterator(it.m_ptr + n); }
    __forceinline friend ContiguousIterator operator-(ContiguousIterator it, difference_type n) noexcept { return ContiguousIterator(it.m_ptr - n); }
    __forceinline friend difference_type    operator-(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr - b.m_ptr; }

    __forceinline friend bool operator==(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr == b.m_ptr; }
    __forceinline friend bool operator!=(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr != b.m_ptr; }
    __forceinline friend bool operator<(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr < b.m_ptr; }
    __forceinline friend bool operator>(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr > b.m_ptr; }
    __forceinline friend bool operator<=(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr <= b.m_ptr; }
    __forceinline friend bool operator>=(ContiguousIterator a, ContiguousIterator b) noexcept { return a.m_ptr >= b.m_ptr; }

    T* GetPtr() const noexcept { return m_ptr; }

private:
    T* m_ptr = nullptr;
};

/// @brief イテレータ検証結果
enum class IteratorValidity : int
{
    Valid       = 0,  // 有効
    NotOwned    = 1,  // このコンテナに属さない
    Invalidated = 2,  // 無効化済み
};

} // namespace gx::container
