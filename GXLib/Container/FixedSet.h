#pragma once
/// @file FixedSet.h
/// @brief 固定容量ソート済みセット — 内部バッファにソート済み配列で格納

#include "pch_common.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量ソート済みセット (ソート済み配列、二分探索)
/// @tparam Key キー型
/// @tparam N 最大要素数
/// @tparam Compare 比較関数
template <typename Key, size_t N, typename Compare = Less<Key>>
class FixedSet
{
public:
    using key_type       = Key;
    using size_type      = size_t;
    using iterator       = Key*;
    using const_iterator = const Key*;

    static constexpr size_type kCapacity = N;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedSet() noexcept = default;
    ~FixedSet() { Clear(); }

    FixedSet(const FixedSet& other) : m_size(0)
    {
        for (size_type i = 0; i < other.m_size; ++i)
            ::new (static_cast<void*>(GetSlot(i))) Key(other.GetKey(i));
        m_size = other.m_size;
    }

    FixedSet& operator=(const FixedSet& other)
    {
        if (this != &other)
        {
            Clear();
            for (size_type i = 0; i < other.m_size; ++i)
                ::new (static_cast<void*>(GetSlot(i))) Key(other.GetKey(i));
            m_size = other.m_size;
        }
        return *this;
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline bool      Empty() const noexcept { return m_size == 0; }
    __forceinline size_type MaxSize() const noexcept { return N; }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return GetSlot(0); }
    __forceinline const_iterator begin()  const noexcept { return GetSlot(0); }
    __forceinline iterator       end()          noexcept { return GetSlot(m_size); }
    __forceinline const_iterator end()    const noexcept { return GetSlot(m_size); }

    // ========================================================================
    // Lookup
    // ========================================================================

    iterator Find(const Key& key) noexcept
    {
        size_type idx = LowerBound(key);
        if (idx < m_size && !m_comp(key, GetKey(idx)) && !m_comp(GetKey(idx), key))
            return GetSlot(idx);
        return end();
    }

    const_iterator Find(const Key& key) const noexcept
    {
        size_type idx = LowerBound(key);
        if (idx < m_size && !m_comp(key, GetKey(idx)) && !m_comp(GetKey(idx), key))
            return GetSlot(idx);
        return end();
    }

    bool Contains(const Key& key) const noexcept { return Find(key) != end(); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    struct InsertResult { iterator it; bool inserted; };

    InsertResult Insert(const Key& key)
    {
        size_type idx = LowerBound(key);
        if (idx < m_size && !m_comp(key, GetKey(idx)) && !m_comp(GetKey(idx), key))
            return { GetSlot(idx), false };

        assert(m_size < N && "FixedSet overflow");
        ShiftRight(idx);
        ::new (static_cast<void*>(GetSlot(idx))) Key(key);
        ++m_size;
        return { GetSlot(idx), true };
    }

    bool Erase(const Key& key)
    {
        size_type idx = LowerBound(key);
        if (idx >= m_size || m_comp(key, GetKey(idx)) || m_comp(GetKey(idx), key))
            return false;
        GetKey(idx).~Key();
        ShiftLeft(idx + 1);
        --m_size;
        return true;
    }

    void Clear() noexcept
    {
        for (size_type i = 0; i < m_size; ++i)
            GetKey(i).~Key();
        m_size = 0;
    }

    bool Validate() const noexcept { return m_size <= N; }

private:
    Key*       GetSlot(size_type i)       noexcept { return reinterpret_cast<Key*>(m_storage) + i; }
    const Key* GetSlot(size_type i) const noexcept { return reinterpret_cast<const Key*>(m_storage) + i; }
    Key&       GetKey(size_type i)       noexcept { return *GetSlot(i); }
    const Key& GetKey(size_type i) const noexcept { return *GetSlot(i); }

    size_type LowerBound(const Key& key) const noexcept
    {
        size_type lo = 0, hi = m_size;
        while (lo < hi)
        {
            size_type mid = lo + (hi - lo) / 2;
            if (m_comp(GetKey(mid), key))
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }

    void ShiftRight(size_type idx)
    {
        for (size_type i = m_size; i > idx; --i)
        {
            ::new (static_cast<void*>(GetSlot(i))) Key(std::move(GetKey(i - 1)));
            GetKey(i - 1).~Key();
        }
    }

    void ShiftLeft(size_type idx)
    {
        for (size_type i = idx; i < m_size; ++i)
        {
            ::new (static_cast<void*>(GetSlot(i - 1))) Key(std::move(GetKey(i)));
            GetKey(i).~Key();
        }
    }

    alignas(alignof(Key)) char m_storage[N * sizeof(Key)];
    size_type m_size = 0;
    Compare   m_comp;
};

} // namespace gx::container
