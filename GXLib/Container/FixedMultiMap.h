#pragma once
/// @file FixedMultiMap.h
/// @brief 固定容量ソート済みマルチマップ — 重複キー許可

#include "pch_common.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量ソート済みマルチマップ (ソート済み配列、二分探索、重複キー許可)
/// @tparam Key キー型
/// @tparam Value 値型
/// @tparam N 最大要素数
/// @tparam Compare 比較関数
template <typename Key, typename Value, size_t N, typename Compare = Less<Key>>
class FixedMultiMap
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

    FixedMultiMap() noexcept = default;
    ~FixedMultiMap() { Clear(); }

    FixedMultiMap(const FixedMultiMap& other) : m_size(0)
    {
        for (size_type i = 0; i < other.m_size; ++i)
            ::new (static_cast<void*>(GetSlot(i))) Pair(other.GetPair(i));
        m_size = other.m_size;
    }

    FixedMultiMap& operator=(const FixedMultiMap& other)
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

    size_type Count(const Key& key) const noexcept
    {
        size_type lo = LowerBound(key);
        size_type hi = UpperBound(key);
        return hi - lo;
    }

    /// @brief equal_range — [lower_bound, upper_bound) を返す
    struct Range { iterator first; iterator last; };

    Range EqualRange(const Key& key) noexcept
    {
        return { GetSlot(LowerBound(key)), GetSlot(UpperBound(key)) };
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    iterator Insert(const Key& key, const Value& value)
    {
        assert(m_size < N && "FixedMultiMap overflow");
        size_type idx = UpperBound(key);
        ShiftRight(idx);
        ::new (static_cast<void*>(GetSlot(idx))) Pair{key, value};
        ++m_size;
        return GetSlot(idx);
    }

    /// @brief 指定キーの全エントリを削除
    size_type Erase(const Key& key)
    {
        size_type lo = LowerBound(key);
        size_type hi = UpperBound(key);
        size_type count = hi - lo;
        if (count == 0) return 0;
        for (size_type i = lo; i < hi; ++i)
            GetPair(i).~Pair();
        // Shift left
        for (size_type i = hi; i < m_size; ++i)
        {
            ::new (static_cast<void*>(GetSlot(i - count))) Pair(std::move(GetPair(i)));
            GetPair(i).~Pair();
        }
        m_size -= count;
        return count;
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

    size_type UpperBound(const Key& key) const noexcept
    {
        size_type lo = 0, hi = m_size;
        while (lo < hi)
        {
            size_type mid = lo + (hi - lo) / 2;
            if (m_comp(key, GetPair(mid).first))
                hi = mid;
            else
                lo = mid + 1;
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

    alignas(alignof(Pair)) char m_storage[N * sizeof(Pair)];
    size_type m_size = 0;
    Compare   m_comp;
};

} // namespace gx::container
