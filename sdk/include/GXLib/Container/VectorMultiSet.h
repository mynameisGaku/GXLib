#pragma once
/// @file VectorMultiSet.h
/// @brief ソート済みベクタマルチセット — 重複キー許可

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/Hash.h"
#include "Container/TypeTraits.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief ソート済みベクタマルチセット (重複キー許可)
/// @tparam Key キー型
/// @tparam Compare 比較関数
/// @tparam Alloc アロケータ
template <typename Key, typename Compare = Less<Key>, typename Alloc = Allocator>
class VectorMultiSet
{
public:
    using key_type       = Key;
    using value_type     = Key;
    using size_type      = size_t;
    using iterator       = Key*;
    using const_iterator = const Key*;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    VectorMultiSet() noexcept = default;
    explicit VectorMultiSet(const Compare& comp) : m_comp(comp) {}

    VectorMultiSet(const VectorMultiSet& other) : m_comp(other.m_comp)
    {
        if (other.m_size > 0)
        {
            GrowTo(other.m_size);
            for (size_type i = 0; i < other.m_size; ++i)
                ::new (static_cast<void*>(m_data + i)) Key(other.m_data[i]);
            m_size = other.m_size;
        }
    }

    VectorMultiSet(VectorMultiSet&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity), m_comp(other.m_comp)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    ~VectorMultiSet()
    {
        Clear();
        DeallocBuffer();
    }

    VectorMultiSet& operator=(const VectorMultiSet& other)
    {
        if (this != &other)
        {
            Clear();
            if (other.m_size > 0)
            {
                Reserve(other.m_size);
                for (size_type i = 0; i < other.m_size; ++i)
                    ::new (static_cast<void*>(m_data + i)) Key(other.m_data[i]);
                m_size = other.m_size;
            }
            m_comp = other.m_comp;
        }
        return *this;
    }

    VectorMultiSet& operator=(VectorMultiSet&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            DeallocBuffer();
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_comp = other.m_comp;
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline size_type Capacity() const noexcept { return m_capacity; }
    __forceinline bool      Empty() const noexcept { return m_size == 0; }

    void Reserve(size_type newCap)
    {
        if (newCap > m_capacity)
            GrowTo(newCap);
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return m_data; }
    __forceinline const_iterator begin()  const noexcept { return m_data; }
    __forceinline iterator       end()          noexcept { return m_data + m_size; }
    __forceinline const_iterator end()    const noexcept { return m_data + m_size; }

    // ========================================================================
    // Lookup
    // ========================================================================

    iterator Find(const Key& key) noexcept
    {
        size_type idx = LowerBoundIdx(key);
        if (idx < m_size && IsEqual(key, m_data[idx]))
            return m_data + idx;
        return end();
    }

    const_iterator Find(const Key& key) const noexcept
    {
        size_type idx = LowerBoundIdx(key);
        if (idx < m_size && IsEqual(key, m_data[idx]))
            return m_data + idx;
        return end();
    }

    bool Contains(const Key& key) const noexcept { return Find(key) != end(); }

    size_type Count(const Key& key) const noexcept
    {
        return UpperBoundIdx(key) - LowerBoundIdx(key);
    }

    iterator LowerBound(const Key& key) noexcept { return m_data + LowerBoundIdx(key); }
    const_iterator LowerBound(const Key& key) const noexcept { return m_data + LowerBoundIdx(key); }
    iterator UpperBound(const Key& key) noexcept { return m_data + UpperBoundIdx(key); }
    const_iterator UpperBound(const Key& key) const noexcept { return m_data + UpperBoundIdx(key); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 挿入 (重複許可、upper_bound 位置に挿入で安定順序)
    iterator Insert(const Key& key)
    {
        size_type idx = UpperBoundIdx(key);
        EnsureCapacity(m_size + 1);
        ShiftRight(idx);
        ::new (static_cast<void*>(m_data + idx)) Key(key);
        ++m_size;
        return m_data + idx;
    }

    /// @brief 指定キーの全エントリを削除
    size_type Erase(const Key& key)
    {
        size_type lo = LowerBoundIdx(key);
        size_type hi = UpperBoundIdx(key);
        size_type count = hi - lo;
        if (count == 0) return 0;

        for (size_type i = lo; i < hi; ++i)
            m_data[i].~Key();
        for (size_type i = hi; i < m_size; ++i)
        {
            ::new (static_cast<void*>(m_data + i - count)) Key(std::move(m_data[i]));
            m_data[i].~Key();
        }
        m_size -= count;
        return count;
    }

    void Clear() noexcept
    {
        destroy_range(m_data, m_size);
        m_size = 0;
    }

    bool Validate() const noexcept
    {
        for (size_type i = 1; i < m_size; ++i)
            if (m_comp(m_data[i], m_data[i - 1]))
                return false;
        return true;
    }

private:
    __forceinline bool IsEqual(const Key& a, const Key& b) const noexcept
    {
        return !m_comp(a, b) && !m_comp(b, a);
    }

    size_type LowerBoundIdx(const Key& key) const noexcept
    {
        size_type lo = 0, hi = m_size;
        while (lo < hi)
        {
            size_type mid = lo + (hi - lo) / 2;
            if (m_comp(m_data[mid], key))
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }

    size_type UpperBoundIdx(const Key& key) const noexcept
    {
        size_type lo = 0, hi = m_size;
        while (lo < hi)
        {
            size_type mid = lo + (hi - lo) / 2;
            if (m_comp(key, m_data[mid]))
                hi = mid;
            else
                lo = mid + 1;
        }
        return lo;
    }

    void EnsureCapacity(size_type required)
    {
        if (required <= m_capacity) return;
        size_type newCap = m_capacity ? m_capacity * 2 : 8;
        if (newCap < required) newCap = required;
        GrowTo(newCap);
    }

    void GrowTo(size_type newCap)
    {
        Alloc alloc;
        auto* newData = static_cast<Key*>(alloc.allocate(newCap * sizeof(Key), alignof(Key), 0));
        if (m_data)
        {
            uninitialized_relocate(newData, m_data, m_size);
            alloc.deallocate(m_data, m_capacity * sizeof(Key));
        }
        m_data = newData;
        m_capacity = newCap;
    }

    void DeallocBuffer()
    {
        if (m_data)
        {
            Alloc alloc;
            alloc.deallocate(m_data, m_capacity * sizeof(Key));
            m_data = nullptr;
            m_capacity = 0;
        }
    }

    void ShiftRight(size_type idx)
    {
        for (size_type i = m_size; i > idx; --i)
        {
            ::new (static_cast<void*>(m_data + i)) Key(std::move(m_data[i - 1]));
            m_data[i - 1].~Key();
        }
    }

    Key*      m_data = nullptr;
    size_type m_size = 0;
    size_type m_capacity = 0;
    Compare   m_comp;
};

} // namespace gx::container
