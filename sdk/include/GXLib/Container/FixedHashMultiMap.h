#pragma once
/// @file FixedHashMultiMap.h
/// @brief 固定容量ハッシュマルチマップ — 重複キー許可

#include "pch_common.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量ハッシュマルチマップ (オープンアドレス法、重複キー許可)
/// @tparam Key キー型
/// @tparam Value 値型
/// @tparam N バケット数
/// @tparam HashFunc ハッシュ関数
/// @tparam KeyEqual 等価比較
template <typename Key, typename Value, size_t N,
          typename HashFunc = Hash<Key>, typename KeyEqual = EqualTo<Key>>
class FixedHashMultiMap
{
    struct Slot
    {
        alignas(alignof(Key))   char keyStorage[sizeof(Key)];
        alignas(alignof(Value)) char valStorage[sizeof(Value)];
        bool occupied = false;

        Key*         GetKey()       noexcept { return reinterpret_cast<Key*>(keyStorage); }
        const Key*   GetKey() const noexcept { return reinterpret_cast<const Key*>(keyStorage); }
        Value*       GetValue()       noexcept { return reinterpret_cast<Value*>(valStorage); }
        const Value* GetValue() const noexcept { return reinterpret_cast<const Value*>(valStorage); }

        void Destroy()
        {
            if (occupied)
            {
                GetKey()->~Key();
                GetValue()->~Value();
                occupied = false;
            }
        }
    };

public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = size_t;

    static constexpr size_type kCapacity = N;

    // ========================================================================
    // Iterator
    // ========================================================================
    class Iterator
    {
    public:
        Iterator() noexcept = default;
        Iterator(Slot* slot, Slot* end) noexcept : m_slot(slot), m_end(end) { SkipEmpty(); }

        __forceinline const Key& GetKey() const noexcept { return *m_slot->GetKey(); }
        __forceinline Value&     GetValue() noexcept { return *m_slot->GetValue(); }
        __forceinline const Value& GetValue() const noexcept { return *m_slot->GetValue(); }

        __forceinline Iterator& operator++() noexcept { ++m_slot; SkipEmpty(); return *this; }
        __forceinline Iterator  operator++(int) noexcept { auto t = *this; ++(*this); return t; }
        __forceinline friend bool operator==(Iterator a, Iterator b) noexcept { return a.m_slot == b.m_slot; }
        __forceinline friend bool operator!=(Iterator a, Iterator b) noexcept { return a.m_slot != b.m_slot; }

        Slot* GetSlot() const noexcept { return m_slot; }

    private:
        void SkipEmpty() { while (m_slot != m_end && !m_slot->occupied) ++m_slot; }
        Slot* m_slot = nullptr;
        Slot* m_end = nullptr;
    };

    using iterator = Iterator;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedHashMultiMap() noexcept = default;
    ~FixedHashMultiMap() { Clear(); }

    FixedHashMultiMap(const FixedHashMultiMap& other)
    {
        for (size_type i = 0; i < N; ++i)
            if (other.m_slots[i].occupied)
                Insert(*other.m_slots[i].GetKey(), *other.m_slots[i].GetValue());
    }

    FixedHashMultiMap& operator=(const FixedHashMultiMap& other)
    {
        if (this != &other)
        {
            Clear();
            for (size_type i = 0; i < N; ++i)
                if (other.m_slots[i].occupied)
                    Insert(*other.m_slots[i].GetKey(), *other.m_slots[i].GetValue());
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

    /// @brief 指定キーに一致する最初の要素を検索
    Value* Find(const Key& key) noexcept
    {
        if (m_size == 0) return nullptr;
        size_type h = m_hash(key) % N;
        size_type start = h;
        do
        {
            if (!m_slots[h].occupied) return nullptr;
            if (m_equal(*m_slots[h].GetKey(), key)) return m_slots[h].GetValue();
            h = (h + 1) % N;
        } while (h != start);
        return nullptr;
    }

    /// @brief 指定キーの出現回数
    size_type Count(const Key& key) const noexcept
    {
        size_type count = 0;
        if (m_size == 0) return 0;
        size_type h = m_hash(key) % N;
        size_type start = h;
        do
        {
            if (!m_slots[h].occupied) break;
            if (m_equal(*m_slots[h].GetKey(), key)) ++count;
            h = (h + 1) % N;
        } while (h != start);
        return count;
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 挿入 (重複キー許可)
    iterator Insert(const Key& key, const Value& value)
    {
        assert(m_size < N && "FixedHashMultiMap overflow");
        size_type h = m_hash(key) % N;
        while (m_slots[h].occupied)
            h = (h + 1) % N;

        ::new (static_cast<void*>(m_slots[h].GetKey())) Key(key);
        ::new (static_cast<void*>(m_slots[h].GetValue())) Value(value);
        m_slots[h].occupied = true;
        ++m_size;
        return Iterator(m_slots + h, m_slots + N);
    }

    /// @brief 指定キーの全エントリを削除
    size_type Erase(const Key& key)
    {
        size_type removed = 0;
        for (size_type i = 0; i < N; ++i)
        {
            if (m_slots[i].occupied && m_equal(*m_slots[i].GetKey(), key))
            {
                m_slots[i].Destroy();
                --m_size;
                ++removed;
            }
        }
        // Rehash remaining entries after removal (full rebuild for simplicity)
        if (removed > 0)
            Rebuild();
        return removed;
    }

    void Clear() noexcept
    {
        for (size_type i = 0; i < N; ++i)
            m_slots[i].Destroy();
        m_size = 0;
    }

    bool Validate() const noexcept { return m_size <= N; }

private:
    void Rebuild()
    {
        Slot temp[N];
        size_type count = 0;
        for (size_type i = 0; i < N; ++i)
        {
            if (m_slots[i].occupied)
            {
                ::new (static_cast<void*>(temp[count].GetKey())) Key(std::move(*m_slots[i].GetKey()));
                ::new (static_cast<void*>(temp[count].GetValue())) Value(std::move(*m_slots[i].GetValue()));
                temp[count].occupied = true;
                m_slots[i].Destroy();
                ++count;
            }
        }
        m_size = 0;
        for (size_type i = 0; i < count; ++i)
        {
            Insert(*temp[i].GetKey(), *temp[i].GetValue());
            temp[i].Destroy();
        }
    }

    Slot      m_slots[N] = {};
    size_type m_size = 0;
    HashFunc  m_hash;
    KeyEqual  m_equal;
};

} // namespace gx::container
