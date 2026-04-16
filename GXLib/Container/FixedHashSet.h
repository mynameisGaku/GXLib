#pragma once
/// @file FixedHashSet.h
/// @brief 固定容量ハッシュセット — 内部バッファにオープンアドレス法で格納

#include "pch_common.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量ハッシュセット (オープンアドレス法、線形探索)
/// @tparam Key キー型
/// @tparam N バケット数
/// @tparam HashFunc ハッシュ関数
/// @tparam KeyEqual 等価比較
template <typename Key, size_t N,
          typename HashFunc = Hash<Key>, typename KeyEqual = EqualTo<Key>>
class FixedHashSet
{
    struct Slot
    {
        alignas(alignof(Key)) char keyStorage[sizeof(Key)];
        bool occupied = false;

        Key*       GetKey()       noexcept { return reinterpret_cast<Key*>(keyStorage); }
        const Key* GetKey() const noexcept { return reinterpret_cast<const Key*>(keyStorage); }

        void Destroy()
        {
            if (occupied)
            {
                GetKey()->~Key();
                occupied = false;
            }
        }
    };

public:
    using key_type  = Key;
    using size_type = size_t;

    static constexpr size_type kCapacity = N;

    // ========================================================================
    // Iterator
    // ========================================================================
    class Iterator
    {
    public:
        Iterator() noexcept = default;
        Iterator(Slot* slot, Slot* end) noexcept : m_slot(slot), m_end(end) { SkipEmpty(); }

        __forceinline const Key& operator*() const noexcept { return *m_slot->GetKey(); }
        __forceinline const Key* operator->() const noexcept { return m_slot->GetKey(); }
        __forceinline Iterator& operator++() noexcept { ++m_slot; SkipEmpty(); return *this; }
        __forceinline Iterator  operator++(int) noexcept { auto t = *this; ++(*this); return t; }
        __forceinline friend bool operator==(Iterator a, Iterator b) noexcept { return a.m_slot == b.m_slot; }
        __forceinline friend bool operator!=(Iterator a, Iterator b) noexcept { return a.m_slot != b.m_slot; }

    private:
        void SkipEmpty() { while (m_slot != m_end && !m_slot->occupied) ++m_slot; }
        Slot* m_slot = nullptr;
        Slot* m_end = nullptr;
    };

    using iterator       = Iterator;
    using const_iterator = Iterator;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedHashSet() noexcept = default;
    ~FixedHashSet() { Clear(); }

    FixedHashSet(const FixedHashSet& other)
    {
        for (size_type i = 0; i < N; ++i)
            if (other.m_slots[i].occupied)
                Insert(*other.m_slots[i].GetKey());
    }

    FixedHashSet& operator=(const FixedHashSet& other)
    {
        if (this != &other)
        {
            Clear();
            for (size_type i = 0; i < N; ++i)
                if (other.m_slots[i].occupied)
                    Insert(*other.m_slots[i].GetKey());
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

    iterator begin() noexcept { return Iterator(m_slots, m_slots + N); }
    iterator end()   noexcept { return Iterator(m_slots + N, m_slots + N); }

    // ========================================================================
    // Lookup
    // ========================================================================

    bool Contains(const Key& key) const noexcept { return FindSlot(key) != N; }

    iterator Find(const Key& key) noexcept
    {
        size_type idx = FindSlot(key);
        if (idx != N) return Iterator(m_slots + idx, m_slots + N);
        return end();
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    struct InsertResult { iterator it; bool inserted; };

    InsertResult Insert(const Key& key)
    {
        size_type idx = FindSlot(key);
        if (idx != N)
            return { Iterator(m_slots + idx, m_slots + N), false };

        assert(m_size < N && "FixedHashSet overflow");
        size_type h = m_hash(key) % N;
        while (m_slots[h].occupied)
            h = (h + 1) % N;

        ::new (static_cast<void*>(m_slots[h].GetKey())) Key(key);
        m_slots[h].occupied = true;
        ++m_size;
        return { Iterator(m_slots + h, m_slots + N), true };
    }

    bool Erase(const Key& key)
    {
        size_type idx = FindSlot(key);
        if (idx == N) return false;
        m_slots[idx].Destroy();
        --m_size;
        // Rehash cluster
        size_type i = (idx + 1) % N;
        while (m_slots[i].occupied)
        {
            Key k = std::move(*m_slots[i].GetKey());
            m_slots[i].Destroy();
            --m_size;
            Insert(std::move(k));
            i = (i + 1) % N;
        }
        return true;
    }

    void Clear() noexcept
    {
        for (size_type i = 0; i < N; ++i)
            m_slots[i].Destroy();
        m_size = 0;
    }

    bool Validate() const noexcept { return m_size <= N; }

    InsertResult Insert(Key&& key)
    {
        size_type idx = FindSlot(key);
        if (idx != N)
            return { Iterator(m_slots + idx, m_slots + N), false };

        assert(m_size < N && "FixedHashSet overflow");
        size_type h = m_hash(key) % N;
        while (m_slots[h].occupied)
            h = (h + 1) % N;
        ::new (static_cast<void*>(m_slots[h].GetKey())) Key(std::move(key));
        m_slots[h].occupied = true;
        ++m_size;
        return { Iterator(m_slots + h, m_slots + N), true };
    }

private:
    size_type FindSlot(const Key& key) const noexcept
    {
        if (m_size == 0) return N;
        size_type h = m_hash(key) % N;
        size_type start = h;
        do
        {
            if (!m_slots[h].occupied) return N;
            if (m_equal(*m_slots[h].GetKey(), key)) return h;
            h = (h + 1) % N;
        } while (h != start);
        return N;
    }

    Slot      m_slots[N] = {};
    size_type m_size = 0;
    HashFunc  m_hash;
    KeyEqual  m_equal;
};

} // namespace gx::container
