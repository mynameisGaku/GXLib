#pragma once
/// @file FixedMap.h
/// @brief 固定容量ソート済みマップ — 内部バッファにソート済み配列で格納

#include "pch_common.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量ソート済みマップ (ソート済み配列、二分探索)
/// @tparam Key キー型
/// @tparam Value 値型
/// @tparam N 最大要素数
/// @tparam Compare 比較関数
template <typename Key, typename Value, size_t N, typename Compare = Less<Key>>
class FixedMap
{
    struct Pair
    {
        Key   first;
        Value second;
    };

public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = size_t;
    using iterator    = Pair*;
    using const_iterator = const Pair*;

    static constexpr size_type kCapacity = N;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedMap() noexcept = default;
    ~FixedMap() { Clear(); }

    FixedMap(const FixedMap& other) : m_size(0)
    {
        for (size_type i = 0; i < other.m_size; ++i)
            ::new (static_cast<void*>(GetSlot(i))) Pair(other.GetPair(i));
        m_size = other.m_size;
    }

    FixedMap& operator=(const FixedMap& other)
    {
        if (this != &other)
        {
            Clear();
            for (size_type i = 0; i < other.m_size; ++i)
                ::new (static_cast<void*>(GetSlot(i))) Pair(other.GetPair(i));
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
        if (idx < m_size && !m_comp(key, GetPair(idx).first) && !m_comp(GetPair(idx).first, key))
            return GetSlot(idx);
        return end();
    }

    const_iterator Find(const Key& key) const noexcept
    {
        size_type idx = LowerBound(key);
        if (idx < m_size && !m_comp(key, GetPair(idx).first) && !m_comp(GetPair(idx).first, key))
            return GetSlot(idx);
        return end();
    }

    bool Contains(const Key& key) const noexcept { return Find(key) != end(); }

    Value& operator[](const Key& key)
    {
        size_type idx = LowerBound(key);
        if (idx < m_size && !m_comp(key, GetPair(idx).first) && !m_comp(GetPair(idx).first, key))
            return GetPair(idx).second;
        // Insert
        assert(m_size < N && "FixedMap overflow");
        ShiftRight(idx);
        ::new (static_cast<void*>(GetSlot(idx))) Pair{key, Value{}};
        ++m_size;
        return GetPair(idx).second;
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    struct InsertResult { iterator it; bool inserted; };

    InsertResult Insert(const Key& key, const Value& value)
    {
        size_type idx = LowerBound(key);
        if (idx < m_size && !m_comp(key, GetPair(idx).first) && !m_comp(GetPair(idx).first, key))
            return { GetSlot(idx), false };

        assert(m_size < N && "FixedMap overflow");
        ShiftRight(idx);
        ::new (static_cast<void*>(GetSlot(idx))) Pair{key, value};
        ++m_size;
        return { GetSlot(idx), true };
    }

    bool Erase(const Key& key)
    {
        size_type idx = LowerBound(key);
        if (idx >= m_size || m_comp(key, GetPair(idx).first) || m_comp(GetPair(idx).first, key))
            return false;
        GetPair(idx).~Pair();
        ShiftLeft(idx + 1);
        --m_size;
        return true;
    }

    void Clear() noexcept
    {
        for (size_type i = 0; i < m_size; ++i)
            GetPair(i).~Pair();
        m_size = 0;
    }

    bool Validate() const noexcept { return m_size <= N; }

private:
    Pair*       GetSlot(size_type i)       noexcept { return reinterpret_cast<Pair*>(m_storage) + i; }
    const Pair* GetSlot(size_type i) const noexcept { return reinterpret_cast<const Pair*>(m_storage) + i; }
    Pair&       GetPair(size_type i)       noexcept { return *GetSlot(i); }
    const Pair& GetPair(size_type i) const noexcept { return *GetSlot(i); }

    size_type LowerBound(const Key& key) const noexcept
    {
        size_type lo = 0, hi = m_size;
        while (lo < hi)
        {
            size_type mid = lo + (hi - lo) / 2;
            if (m_comp(GetPair(mid).first, key))
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
            ::new (static_cast<void*>(GetSlot(i))) Pair(std::move(GetPair(i - 1)));
            GetPair(i - 1).~Pair();
        }
    }

    void ShiftLeft(size_type idx)
    {
        for (size_type i = idx; i < m_size; ++i)
        {
            ::new (static_cast<void*>(GetSlot(i - 1))) Pair(std::move(GetPair(i)));
            GetPair(i).~Pair();
        }
    }

    alignas(alignof(Pair)) char m_storage[N * sizeof(Pair)];
    size_type m_size = 0;
    Compare   m_comp;
};

} // namespace gx::container
