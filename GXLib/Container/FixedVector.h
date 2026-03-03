#pragma once
/// @file FixedVector.h
/// @brief 固定容量ベクタ — 内部バッファ使用、オーバーフロー時ヒープ退避オプション付き

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/TypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量ベクタ
/// @tparam T 要素型
/// @tparam N 内部バッファ容量
/// @tparam AllowOverflow true なら満杯時にヒープへ退避
template <typename T, size_t N, bool AllowOverflow = false>
class FixedVector
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

    static constexpr size_type kFixedCapacity = N;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedVector() noexcept
        : m_data(GetFixedBuffer()), m_size(0), m_capacity(N), m_usingHeap(false)
    {
    }

    explicit FixedVector(size_type count)
        : m_data(GetFixedBuffer()), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        Resize(count);
    }

    FixedVector(size_type count, const T& value)
        : m_data(GetFixedBuffer()), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        Resize(count, value);
    }

    FixedVector(std::initializer_list<T> init)
        : m_data(GetFixedBuffer()), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        Reserve(init.size());
        for (auto& v : init)
            PushBack(v);
    }

    FixedVector(const FixedVector& other)
        : m_data(GetFixedBuffer()), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        Reserve(other.m_size);
        uninitialized_copy(m_data, other.m_data, other.m_size);
        m_size = other.m_size;
    }

    FixedVector(FixedVector&& other) noexcept
        : m_data(GetFixedBuffer()), m_size(0), m_capacity(N), m_usingHeap(false)
    {
        if (other.m_usingHeap)
        {
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_usingHeap = true;
            other.m_data = other.GetFixedBuffer();
            other.m_size = 0;
            other.m_capacity = N;
            other.m_usingHeap = false;
        }
        else
        {
            uninitialized_move(m_data, other.m_data, other.m_size);
            m_size = other.m_size;
            destroy_range(other.m_data, other.m_size);
            other.m_size = 0;
        }
    }

    ~FixedVector()
    {
        Clear();
        FreeHeap();
    }

    FixedVector& operator=(const FixedVector& other)
    {
        if (this != &other)
        {
            Clear();
            Reserve(other.m_size);
            uninitialized_copy(m_data, other.m_data, other.m_size);
            m_size = other.m_size;
        }
        return *this;
    }

    FixedVector& operator=(FixedVector&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            FreeHeap();
            if (other.m_usingHeap)
            {
                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                m_usingHeap = true;
                other.m_data = other.GetFixedBuffer();
                other.m_size = 0;
                other.m_capacity = N;
                other.m_usingHeap = false;
            }
            else
            {
                m_data = GetFixedBuffer();
                m_capacity = N;
                m_usingHeap = false;
                uninitialized_move(m_data, other.m_data, other.m_size);
                m_size = other.m_size;
                destroy_range(other.m_data, other.m_size);
                other.m_size = 0;
            }
        }
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    __forceinline reference operator[](size_type i) noexcept
    {
        assert(i < m_size);
        return m_data[i];
    }

    __forceinline const_reference operator[](size_type i) const noexcept
    {
        assert(i < m_size);
        return m_data[i];
    }

    __forceinline reference Front() noexcept { assert(m_size > 0); return m_data[0]; }
    __forceinline const_reference Front() const noexcept { assert(m_size > 0); return m_data[0]; }
    __forceinline reference Back() noexcept { assert(m_size > 0); return m_data[m_size - 1]; }
    __forceinline const_reference Back() const noexcept { assert(m_size > 0); return m_data[m_size - 1]; }
    __forceinline pointer Data() noexcept { return m_data; }
    __forceinline const_pointer Data() const noexcept { return m_data; }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return m_data; }
    __forceinline const_iterator begin()  const noexcept { return m_data; }
    __forceinline const_iterator cbegin() const noexcept { return m_data; }
    __forceinline iterator       end()          noexcept { return m_data + m_size; }
    __forceinline const_iterator end()    const noexcept { return m_data + m_size; }
    __forceinline const_iterator cend()   const noexcept { return m_data + m_size; }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline size_type Capacity() const noexcept { return m_capacity; }
    __forceinline bool Empty() const noexcept { return m_size == 0; }
    __forceinline bool IsUsingHeap() const noexcept { return m_usingHeap; }

    void Reserve(size_type newCap)
    {
        if (newCap <= m_capacity)
            return;
        if constexpr (!AllowOverflow)
        {
            assert(newCap <= N && "FixedVector overflow (AllowOverflow=false)");
            return;
        }
        else
        {
            GrowTo(newCap);
        }
    }

    void ShrinkToFit()
    {
        // No-op for fixed vector unless using heap
        if (!m_usingHeap)
            return;
        if (m_size <= N)
        {
            // Move back to fixed buffer
            T* fixed = GetFixedBuffer();
            uninitialized_relocate(fixed, m_data, m_size);
            Allocator alloc;
            alloc.deallocate(m_data, m_capacity * sizeof(T));
            m_data = fixed;
            m_capacity = N;
            m_usingHeap = false;
        }
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    void PushBack(const T& value)
    {
        EnsureCapacity(m_size + 1);
        ::new (static_cast<void*>(m_data + m_size)) T(value);
        ++m_size;
    }

    void PushBack(T&& value)
    {
        EnsureCapacity(m_size + 1);
        ::new (static_cast<void*>(m_data + m_size)) T(std::move(value));
        ++m_size;
    }

    template <typename... Args>
    reference EmplaceBack(Args&&... args)
    {
        EnsureCapacity(m_size + 1);
        T* p = ::new (static_cast<void*>(m_data + m_size)) T(std::forward<Args>(args)...);
        ++m_size;
        return *p;
    }

    void PopBack() noexcept
    {
        assert(m_size > 0);
        --m_size;
        m_data[m_size].~T();
    }

    iterator Insert(const_iterator pos, const T& value)
    {
        size_type idx = static_cast<size_type>(pos - m_data);
        assert(idx <= m_size);
        EnsureCapacity(m_size + 1);
        ShiftRight(idx, 1);
        ::new (static_cast<void*>(m_data + idx)) T(value);
        ++m_size;
        return m_data + idx;
    }

    iterator Insert(const_iterator pos, T&& value)
    {
        size_type idx = static_cast<size_type>(pos - m_data);
        assert(idx <= m_size);
        EnsureCapacity(m_size + 1);
        ShiftRight(idx, 1);
        ::new (static_cast<void*>(m_data + idx)) T(std::move(value));
        ++m_size;
        return m_data + idx;
    }

    iterator Erase(const_iterator pos)
    {
        size_type idx = static_cast<size_type>(pos - m_data);
        assert(idx < m_size);
        m_data[idx].~T();
        ShiftLeft(idx + 1, 1);
        --m_size;
        return m_data + idx;
    }

    iterator Erase(const_iterator first, const_iterator last)
    {
        size_type from = static_cast<size_type>(first - m_data);
        size_type to = static_cast<size_type>(last - m_data);
        assert(from <= to && to <= m_size);
        size_type count = to - from;
        if (count == 0) return m_data + from;
        destroy_range(m_data + from, count);
        ShiftLeft(to, count);
        m_size -= count;
        return m_data + from;
    }

    void Clear() noexcept
    {
        destroy_range(m_data, m_size);
        m_size = 0;
    }

    void Resize(size_type newSize)
    {
        if (newSize > m_size)
        {
            EnsureCapacity(newSize);
            uninitialized_default_construct(m_data + m_size, newSize - m_size);
        }
        else if (newSize < m_size)
        {
            destroy_range(m_data + newSize, m_size - newSize);
        }
        m_size = newSize;
    }

    void Resize(size_type newSize, const T& value)
    {
        if (newSize > m_size)
        {
            EnsureCapacity(newSize);
            uninitialized_fill(m_data + m_size, newSize - m_size, value);
        }
        else if (newSize < m_size)
        {
            destroy_range(m_data + newSize, m_size - newSize);
        }
        m_size = newSize;
    }

    void Swap(FixedVector& other) noexcept
    {
        FixedVector tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const FixedVector& a, const FixedVector& b) noexcept
    {
        if (a.m_size != b.m_size) return false;
        for (size_type i = 0; i < a.m_size; ++i)
            if (!(a.m_data[i] == b.m_data[i])) return false;
        return true;
    }

    friend bool operator!=(const FixedVector& a, const FixedVector& b) noexcept
    {
        return !(a == b);
    }

    // ========================================================================
    // Validation (EASTL extension)
    // ========================================================================

    bool Validate() const noexcept { return m_size <= m_capacity && m_data != nullptr; }

private:
    __forceinline T* GetFixedBuffer() noexcept
    {
        return reinterpret_cast<T*>(m_buffer);
    }

    __forceinline const T* GetFixedBuffer() const noexcept
    {
        return reinterpret_cast<const T*>(m_buffer);
    }

    void EnsureCapacity(size_type required)
    {
        if (required <= m_capacity)
            return;
        if constexpr (!AllowOverflow)
        {
            assert(false && "FixedVector overflow (AllowOverflow=false)");
        }
        else
        {
            size_type newCap = m_capacity + m_capacity / 2;
            if (newCap < required) newCap = required;
            GrowTo(newCap);
        }
    }

    void GrowTo(size_type newCap)
    {
        Allocator alloc;
        T* newData = static_cast<T*>(alloc.allocate(newCap * sizeof(T), alignof(T), 0));
        uninitialized_relocate(newData, m_data, m_size);
        if (m_usingHeap)
            alloc.deallocate(m_data, m_capacity * sizeof(T));
        m_data = newData;
        m_capacity = newCap;
        m_usingHeap = true;
    }

    void FreeHeap()
    {
        if (m_usingHeap)
        {
            Allocator alloc;
            alloc.deallocate(m_data, m_capacity * sizeof(T));
            m_data = GetFixedBuffer();
            m_capacity = N;
            m_usingHeap = false;
        }
    }

    /// @brief idx以降の要素をcount分右にシフト (ムーブ)
    void ShiftRight(size_type idx, size_type count)
    {
        if (m_size > idx)
        {
            // 後ろから順にムーブ
            for (size_type i = m_size; i > idx; --i)
            {
                ::new (static_cast<void*>(m_data + i + count - 1)) T(std::move(m_data[i - 1]));
                m_data[i - 1].~T();
            }
        }
    }

    /// @brief idx以降の要素をcount分左にシフト (ムーブ)
    void ShiftLeft(size_type idx, size_type count)
    {
        for (size_type i = idx; i < m_size; ++i)
        {
            ::new (static_cast<void*>(m_data + i - count)) T(std::move(m_data[i]));
            m_data[i].~T();
        }
    }

    // ========================================================================
    // Data members
    // ========================================================================
    alignas(alignof(T)) char m_buffer[N * sizeof(T)];
    T*        m_data;
    size_type m_size;
    size_type m_capacity;
    bool      m_usingHeap;
};

} // namespace gx::container
