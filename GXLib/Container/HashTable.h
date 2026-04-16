#pragma once
/// @file HashTable.h
/// @brief Robin Hood オープンアドレス法ハッシュテーブル (内部実装)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/Hash.h"
#include "Container/TypeTraits.h"
#include <cassert>
#include <cstring>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

// ============================================================================
// HashTable — Robin Hood open-addressing
// ============================================================================

/// @brief Robin Hood オープンアドレス法ハッシュテーブル
///
/// 平坦配列 + 線形走査。挿入時に PSL (probe sequence length) の短いエントリを
/// 押し出して均等化する Robin Hood ヒューリスティクスを採用。
/// 削除は backward-shift (トゥームストーン不要)。
///
/// @tparam Value      格納値型
/// @tparam Key        キー型
/// @tparam ExtractKey Value から Key を取り出すファンクタ
/// @tparam HashFunc   ハッシュ関数 (Hash<Key> など)
/// @tparam EqualFunc  等価比較ファンクタ (EqualTo<Key> など)
/// @tparam Alloc      アロケータ型
/// @tparam bMultiKey  true で重複キー許可
template <typename Value, typename Key, typename ExtractKey,
          typename HashFunc, typename EqualFunc,
          typename Alloc, bool bMultiKey>
class HashTable
{
public:
    // ====================================================================
    // 型エイリアス
    // ====================================================================
    using key_type       = Key;
    using value_type     = Value;
    using size_type      = size_t;
    using difference_type = ptrdiff_t;
    using hasher         = HashFunc;
    using key_equal      = EqualFunc;
    using allocator_type = Alloc;
    using reference      = value_type&;
    using const_reference = const value_type&;
    using pointer        = value_type*;
    using const_pointer  = const value_type*;

    static constexpr size_type k_MinCapacity = 8;
    static constexpr float     k_MaxLoadFactor = 0.75f;

private:
    // ====================================================================
    // スロット
    // ====================================================================
    struct Slot
    {
        union {
            value_type value;
        };
        size_type hashCode;
        uint8_t   distance; // 0 = empty, >= 1 = occupied (PSL)

        Slot() noexcept : hashCode(0), distance(0) {}
        ~Slot() {} // value のデストラクタは明示的に呼ぶ
    };

public:
    // ====================================================================
    // イテレータ
    // ====================================================================

    /// @brief 前方イテレータ (非空スロットを走査)
    template <typename ValueRef, typename SlotPtr>
    class IteratorImpl
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Value;
        using difference_type   = ptrdiff_t;
        using pointer           = ValueRef*;
        using reference         = ValueRef&;

        IteratorImpl() noexcept = default;
        IteratorImpl(SlotPtr slot, SlotPtr end) noexcept
            : m_slot(slot), m_end(end) {}

        __forceinline reference operator*()  const noexcept { return m_slot->value; }
        __forceinline pointer   operator->() const noexcept { return &m_slot->value; }

        __forceinline IteratorImpl& operator++() noexcept
        {
            ++m_slot;
            SkipEmpty();
            return *this;
        }

        __forceinline IteratorImpl operator++(int) noexcept
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        __forceinline friend bool operator==(const IteratorImpl& a, const IteratorImpl& b) noexcept
        {
            return a.m_slot == b.m_slot;
        }
        __forceinline friend bool operator!=(const IteratorImpl& a, const IteratorImpl& b) noexcept
        {
            return a.m_slot != b.m_slot;
        }

        // const_iterator への暗黙変換
        operator IteratorImpl<const Value, const Slot*>() const noexcept
        {
            return IteratorImpl<const Value, const Slot*>(m_slot, m_end);
        }

        SlotPtr GetSlot() const noexcept { return m_slot; }

    private:
        friend class HashTable;

        __forceinline void SkipEmpty() noexcept
        {
            while (m_slot != m_end && m_slot->distance == 0)
                ++m_slot;
        }

        SlotPtr m_slot = nullptr;
        SlotPtr m_end  = nullptr;
    };

    using iterator       = IteratorImpl<Value, Slot*>;
    using const_iterator = IteratorImpl<const Value, const Slot*>;

    // ====================================================================
    // insert 戻り値
    // ====================================================================
    struct InsertResult
    {
        iterator  first;
        bool      second; // true = 挿入された
    };

    // ====================================================================
    // コンストラクタ / デストラクタ
    // ====================================================================

    HashTable()
        : m_allocPair(Alloc(), nullptr), m_capacity(0), m_size(0) {}

    explicit HashTable(size_type initialCapacity, const HashFunc& hash = HashFunc(),
                       const EqualFunc& eq = EqualFunc(), const Alloc& alloc = Alloc())
        : m_hash(hash), m_equal(eq), m_allocPair(alloc, nullptr),
          m_capacity(0), m_size(0)
    {
        if (initialCapacity > 0)
            AllocateSlots(RoundUpCapacity(initialCapacity));
    }

    HashTable(const HashTable& other)
        : m_hash(other.m_hash), m_equal(other.m_equal),
          m_allocPair(other.GetAllocator(), nullptr),
          m_capacity(0), m_size(0)
    {
        if (other.m_size > 0)
        {
            AllocateSlots(other.m_capacity);
            for (size_type i = 0; i < other.m_capacity; ++i)
            {
                if (other.GetSlots()[i].distance != 0)
                {
                    Slot& dst = GetSlots()[i];
                    const Slot& src = other.GetSlots()[i];
                    ::new (static_cast<void*>(&dst.value)) value_type(src.value);
                    dst.hashCode = src.hashCode;
                    dst.distance = src.distance;
                }
            }
            m_size = other.m_size;
        }
    }

    HashTable(HashTable&& other) noexcept
        : m_hash(std::move(other.m_hash)), m_equal(std::move(other.m_equal)),
          m_allocPair(std::move(other.m_allocPair)),
          m_capacity(other.m_capacity), m_size(other.m_size)
    {
        other.m_allocPair.Second() = nullptr;
        other.m_capacity = 0;
        other.m_size = 0;
    }

    ~HashTable()
    {
        Clear();
        DeallocateSlots();
    }

    HashTable& operator=(const HashTable& other)
    {
        if (this != &other)
        {
            HashTable tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    HashTable& operator=(HashTable&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            DeallocateSlots();

            m_hash   = std::move(other.m_hash);
            m_equal  = std::move(other.m_equal);
            m_allocPair = std::move(other.m_allocPair);
            m_capacity = other.m_capacity;
            m_size     = other.m_size;

            other.m_allocPair.Second() = nullptr;
            other.m_capacity = 0;
            other.m_size = 0;
        }
        return *this;
    }

    // ====================================================================
    // 容量
    // ====================================================================

    __forceinline size_type Size()     const noexcept { return m_size; }
    __forceinline size_type Capacity() const noexcept { return m_capacity; }
    __forceinline bool      Empty()    const noexcept { return m_size == 0; }

    __forceinline float LoadFactor() const noexcept
    {
        return m_capacity == 0 ? 0.0f : static_cast<float>(m_size) / static_cast<float>(m_capacity);
    }

    __forceinline float MaxLoadFactor() const noexcept { return k_MaxLoadFactor; }

    // ====================================================================
    // イテレータ
    // ====================================================================

    __forceinline iterator begin() noexcept
    {
        Slot* s = GetSlots();
        Slot* e = s + m_capacity;
        iterator it(s, e);
        it.SkipEmpty();
        return it;
    }

    __forceinline iterator end() noexcept
    {
        Slot* e = GetSlots() + m_capacity;
        return iterator(e, e);
    }

    __forceinline const_iterator begin() const noexcept
    {
        const Slot* s = GetSlots();
        const Slot* e = s + m_capacity;
        const_iterator it(s, e);
        it.SkipEmpty();
        return it;
    }

    __forceinline const_iterator end() const noexcept
    {
        const Slot* e = GetSlots() + m_capacity;
        return const_iterator(e, e);
    }

    __forceinline const_iterator cbegin() const noexcept { return begin(); }
    __forceinline const_iterator cend()   const noexcept { return end(); }

    // ====================================================================
    // 検索
    // ====================================================================

    /// @brief キーで検索
    __forceinline iterator Find(const key_type& key) noexcept
    {
        return FindInternal(key, m_hash(key));
    }

    __forceinline const_iterator Find(const key_type& key) const noexcept
    {
        return const_cast<HashTable*>(this)->FindInternal(key, m_hash(key));
    }

    /// @brief 異種キーで検索 (heterogeneous lookup)
    template <typename AltKey, typename AltHash, typename AltEqual>
    iterator FindAs(const AltKey& key, const AltHash& hash, const AltEqual& eq) noexcept
    {
        if (m_capacity == 0)
            return end();

        const size_type hc = hash(key);
        const size_type mask = m_capacity - 1;
        size_type idx = hc & mask;
        uint8_t dist = 1;
        Slot* slots = GetSlots();

        while (true)
        {
            Slot& slot = slots[idx];
            if (slot.distance == 0)
                return end();
            if (slot.distance < dist)
                return end(); // Robin Hood 特性: これ以上探す必要なし
            if (slot.hashCode == hc && eq(ExtractKey()(slot.value), key))
                return MakeIterator(&slot);
            ++dist;
            idx = (idx + 1) & mask;
        }
    }

    template <typename AltKey, typename AltHash, typename AltEqual>
    const_iterator FindAs(const AltKey& key, const AltHash& hash, const AltEqual& eq) const noexcept
    {
        return const_cast<HashTable*>(this)->FindAs(key, hash, eq);
    }

    /// @brief キーの存在判定
    __forceinline bool Contains(const key_type& key) const noexcept
    {
        return Find(key) != end();
    }

    /// @brief キーの出現数 (bMultiKey=false なら 0 or 1)
    size_type Count(const key_type& key) const noexcept
    {
        if constexpr (!bMultiKey)
        {
            return Contains(key) ? 1 : 0;
        }
        else
        {
            size_type n = 0;
            if (m_capacity == 0)
                return 0;
            const size_type hc = m_hash(key);
            const size_type mask = m_capacity - 1;
            size_type idx = hc & mask;
            uint8_t dist = 1;
            const Slot* slots = GetSlots();

            while (true)
            {
                const Slot& slot = slots[idx];
                if (slot.distance == 0 || slot.distance < dist)
                    break;
                if (slot.hashCode == hc && m_equal(ExtractKey()(slot.value), key))
                    ++n;
                ++dist;
                idx = (idx + 1) & mask;
            }
            return n;
        }
    }

    /// @brief 等範囲 (bMultiKey=true で複数一致を返す)
    std::pair<iterator, iterator> EqualRange(const key_type& key) noexcept
    {
        iterator first = Find(key);
        if (first == end())
            return { end(), end() };

        if constexpr (!bMultiKey)
        {
            iterator next = first;
            ++next;
            return { first, next };
        }
        else
        {
            // first 以降の連続一致を探す
            const size_type hc = m_hash(key);
            const size_type mask = m_capacity - 1;
            Slot* slots = GetSlots();
            size_type idx = static_cast<size_type>(first.GetSlot() - slots);
            idx = (idx + 1) & mask;
            uint8_t dist = static_cast<uint8_t>(first.GetSlot()->distance + 1);

            while (true)
            {
                Slot& slot = slots[idx];
                if (slot.distance == 0 || slot.distance < dist)
                    break;
                if (slot.hashCode == hc && m_equal(ExtractKey()(slot.value), key))
                {
                    // 一致が続く
                }
                ++dist;
                idx = (idx + 1) & mask;
            }
            // idx は最初の非一致スロット (或いは空)
            // ただしスロット配列上で非連続かもしれないので、イテレータで走査
            iterator last = first;
            ++last;
            while (last != end())
            {
                if (!m_equal(ExtractKey()(*last), key))
                    break;
                ++last;
            }
            return { first, last };
        }
    }

    std::pair<const_iterator, const_iterator> EqualRange(const key_type& key) const noexcept
    {
        auto [f, l] = const_cast<HashTable*>(this)->EqualRange(key);
        return { const_iterator(f.GetSlot(), end().GetSlot()), const_iterator(l.GetSlot(), end().GetSlot()) };
    }

    // ====================================================================
    // 挿入
    // ====================================================================

    /// @brief 値をコピー挿入
    InsertResult Insert(const value_type& value)
    {
        return InsertValue(value);
    }

    /// @brief 値をムーブ挿入
    InsertResult Insert(value_type&& value)
    {
        return InsertValue(std::move(value));
    }

    /// @brief in-place 構築挿入
    template <typename... Args>
    InsertResult Emplace(Args&&... args)
    {
        // まずスタック上に構築し、キーを使って重複チェックしてから挿入
        // (ヒープ上に構築するより単純)
        // alignas で value_type のストレージを確保
        alignas(value_type) unsigned char storage[sizeof(value_type)];
        value_type* pValue = ::new (static_cast<void*>(storage)) value_type(std::forward<Args>(args)...);

        auto result = InsertValue(std::move(*pValue));
        if (!result.second)
        {
            // 挿入されなかった場合のみデストラクト (成功時は move 済み)
            pValue->~value_type();
        }
        else
        {
            // move 元をデストラクト
            pValue->~value_type();
        }
        return result;
    }

    // ====================================================================
    // 削除
    // ====================================================================

    /// @brief イテレータで指定した要素を削除
    iterator Erase(const_iterator pos)
    {
        assert(pos != end());
        Slot* slots = GetSlots();
        size_type idx = static_cast<size_type>(pos.GetSlot() - static_cast<const Slot*>(slots));
        assert(idx < m_capacity && slots[idx].distance != 0);

        // 値のデストラクト
        slots[idx].value.~value_type();
        slots[idx].distance = 0;
        --m_size;

        // backward-shift deletion
        const size_type mask = m_capacity - 1;
        size_type prev = idx;
        size_type cur = (idx + 1) & mask;

        while (slots[cur].distance > 1)
        {
            // cur のエントリを prev にシフト
            ::new (static_cast<void*>(&slots[prev].value)) value_type(std::move(slots[cur].value));
            slots[prev].hashCode = slots[cur].hashCode;
            slots[prev].distance = slots[cur].distance - 1;

            slots[cur].value.~value_type();
            slots[cur].distance = 0;

            prev = cur;
            cur = (cur + 1) & mask;
        }

        // prev の次のスロットからイテレータを構成
        // 削除でシフトが起きたので idx を返す
        Slot* e = slots + m_capacity;
        iterator it(slots + idx, e);
        if (slots[idx].distance == 0)
            it.SkipEmpty();
        return it;
    }

    /// @brief キーで削除 (最初の一致のみ、bMultiKey でも1つ)
    size_type Erase(const key_type& key)
    {
        iterator it = Find(key);
        if (it == end())
            return 0;
        Erase(it);
        return 1;
    }

    // ====================================================================
    // クリア
    // ====================================================================

    /// @brief 全要素を破棄 (メモリは保持)
    void Clear() noexcept
    {
        if (m_size == 0)
            return;
        Slot* slots = GetSlots();
        for (size_type i = 0; i < m_capacity; ++i)
        {
            if (slots[i].distance != 0)
            {
                slots[i].value.~value_type();
                slots[i].distance = 0;
            }
        }
        m_size = 0;
    }

    /// @brief メモリリークを受け入れてリセット (テスト / デバッグ用)
    void ResetLoseMemory() noexcept
    {
        m_allocPair.Second() = nullptr;
        m_capacity = 0;
        m_size = 0;
    }

    // ====================================================================
    // リハッシュ / リザーブ
    // ====================================================================

    /// @brief 最低 n 個のバケットを確保してリハッシュ
    void Rehash(size_type n)
    {
        n = RoundUpCapacity(n);
        if (n <= m_capacity && m_capacity > 0)
            return;
        RehashInternal(n);
    }

    /// @brief n 個の要素を格納できるように予約
    void Reserve(size_type n)
    {
        size_type required = static_cast<size_type>(
            static_cast<float>(n) / k_MaxLoadFactor) + 1;
        Rehash(required);
    }

    // ====================================================================
    // スワップ
    // ====================================================================

    void Swap(HashTable& other) noexcept
    {
        using std::swap;
        swap(m_hash,      other.m_hash);
        swap(m_equal,     other.m_equal);
        swap(m_allocPair, other.m_allocPair);
        swap(m_capacity,  other.m_capacity);
        swap(m_size,      other.m_size);
    }

    // ====================================================================
    // アクセサ
    // ====================================================================

    __forceinline hasher         HashFunction()   const noexcept { return m_hash; }
    __forceinline key_equal      KeyEq()          const noexcept { return m_equal; }
    __forceinline allocator_type GetAllocator()   const noexcept { return m_allocPair.First(); }

    // ====================================================================
    // std compatibility aliases
    // ====================================================================

    __forceinline size_type size() const noexcept { return Size(); }
    __forceinline bool empty() const noexcept { return Empty(); }
    __forceinline float load_factor() const noexcept { return LoadFactor(); }
    __forceinline float max_load_factor() const noexcept { return MaxLoadFactor(); }
    __forceinline size_type bucket_count() const noexcept { return Capacity(); }
    __forceinline iterator find(const key_type& key) noexcept { return Find(key); }
    __forceinline const_iterator find(const key_type& key) const noexcept { return Find(key); }
    __forceinline bool contains(const key_type& key) const noexcept { return Contains(key); }
    __forceinline size_type count(const key_type& key) const noexcept { return Count(key); }
    InsertResult insert(const value_type& value) { return Insert(value); }
    InsertResult insert(value_type&& value) { return Insert(std::move(value)); }
    template <typename... Args>
    InsertResult emplace(Args&&... args) { return Emplace(std::forward<Args>(args)...); }
    iterator erase(const_iterator pos) { return Erase(pos); }
    size_type erase(const key_type& key) { return Erase(key); }
    void clear() noexcept { Clear(); }
    void reserve(size_type n) { Reserve(n); }
    void rehash(size_type n) { Rehash(n); }
    void swap(HashTable& other) noexcept { Swap(other); }
    std::pair<iterator, iterator> equal_range(const key_type& key) noexcept { return EqualRange(key); }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const noexcept { return EqualRange(key); }

    // ====================================================================
    // デバッグ / 検証
    // ====================================================================

    /// @brief 内部整合性を検証 (デバッグ用)
    bool Validate() const noexcept
    {
        if (m_capacity == 0)
            return m_size == 0 && GetSlots() == nullptr;

        // capacity は2の冪
        if ((m_capacity & (m_capacity - 1)) != 0)
            return false;

        size_type count = 0;
        const Slot* slots = GetSlots();
        const size_type mask = m_capacity - 1;

        for (size_type i = 0; i < m_capacity; ++i)
        {
            const Slot& slot = slots[i];
            if (slot.distance == 0)
                continue;

            ++count;

            // hashCode の整合性
            const key_type& key = ExtractKey()(slot.value);
            size_type expectedHash = m_hash(key);
            if (slot.hashCode != expectedHash)
                return false;

            // distance の整合性
            size_type idealIdx = slot.hashCode & mask;
            size_type actualDist = (i >= idealIdx)
                ? (i - idealIdx + 1)
                : (m_capacity - idealIdx + i + 1);
            if (slot.distance != static_cast<uint8_t>(actualDist))
                return false;
        }

        return count == m_size;
    }

    // ====================================================================
    // 比較演算子
    // ====================================================================

    friend bool operator==(const HashTable& a, const HashTable& b) noexcept
    {
        if (a.m_size != b.m_size)
            return false;
        for (auto it = a.begin(); it != a.end(); ++it)
        {
            if (!b.Contains(ExtractKey()(*it)))
                return false;
        }
        return true;
    }

    friend bool operator!=(const HashTable& a, const HashTable& b) noexcept
    {
        return !(a == b);
    }

protected:
    // ====================================================================
    // 内部ヘルパー
    // ====================================================================

    /// @brief イテレータ構築ヘルパー
    __forceinline iterator MakeIterator(Slot* slot) noexcept
    {
        return iterator(slot, GetSlots() + m_capacity);
    }

    /// @brief n を2の冪に切り上げ (最小 k_MinCapacity)
    static __forceinline size_type RoundUpCapacity(size_type n) noexcept
    {
        if (n < k_MinCapacity)
            return k_MinCapacity;
        // 2の冪に切り上げ
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        if constexpr (sizeof(size_type) > 4)
            n |= n >> 32;
        return n + 1;
    }

    /// @brief ロードファクタ超過判定
    __forceinline bool NeedsGrow() const noexcept
    {
        return m_capacity == 0 ||
               (m_size + 1) > static_cast<size_type>(k_MaxLoadFactor * static_cast<float>(m_capacity));
    }

    /// @brief スロット配列へのポインタ取得
    __forceinline Slot* GetSlots() noexcept { return m_allocPair.Second(); }
    __forceinline const Slot* GetSlots() const noexcept { return m_allocPair.Second(); }

    // ====================================================================
    // メモリ管理
    // ====================================================================

    void AllocateSlots(size_type cap)
    {
        assert(cap > 0 && (cap & (cap - 1)) == 0);
        size_type bytes = cap * sizeof(Slot);
        void* mem = m_allocPair.First().allocate(bytes, alignof(Slot), 0, 0);
        Slot* slots = static_cast<Slot*>(mem);
        for (size_type i = 0; i < cap; ++i)
        {
            slots[i].hashCode = 0;
            slots[i].distance = 0;
        }
        m_allocPair.Second() = slots;
        m_capacity = cap;
    }

    void DeallocateSlots()
    {
        if (GetSlots())
        {
            m_allocPair.First().deallocate(GetSlots(), m_capacity * sizeof(Slot));
            m_allocPair.Second() = nullptr;
            m_capacity = 0;
        }
    }

    // ====================================================================
    // 検索内部
    // ====================================================================

    __forceinline iterator FindInternal(const key_type& key, size_type hc) noexcept
    {
        if (m_capacity == 0)
            return end();

        const size_type mask = m_capacity - 1;
        size_type idx = hc & mask;
        uint8_t dist = 1;
        Slot* slots = GetSlots();

        while (true)
        {
            Slot& slot = slots[idx];
            if (slot.distance == 0)
                return end();
            if (slot.distance < dist)
                return end(); // Robin Hood: 早期終了
            if (slot.hashCode == hc && m_equal(ExtractKey()(slot.value), key))
                return MakeIterator(&slot);
            ++dist;
            idx = (idx + 1) & mask;
        }
    }

    // ====================================================================
    // 挿入内部
    // ====================================================================

    template <typename V>
    InsertResult InsertValue(V&& value)
    {
        const key_type& key = ExtractKey()(value);
        const size_type hc = m_hash(key);

        // bMultiKey=false なら重複チェック
        if constexpr (!bMultiKey)
        {
            if (m_capacity > 0)
            {
                iterator it = FindInternal(key, hc);
                if (it != end())
                    return { it, false };
            }
        }

        // ロードファクタチェック → 必要ならグロウ
        if (NeedsGrow())
        {
            size_type newCap = m_capacity == 0 ? k_MinCapacity : m_capacity * 2;
            RehashInternal(newCap);
        }

        return InsertRobinHood(std::forward<V>(value), hc);
    }

    /// @brief Robin Hood 挿入の核
    template <typename V>
    InsertResult InsertRobinHood(V&& value, size_type hc)
    {
        const size_type mask = m_capacity - 1;
        size_type idx = hc & mask;
        uint8_t dist = 1;
        Slot* slots = GetSlots();

        // Robin Hood: 挿入候補値を持ち回る
        // value, hc, dist が "漂流する" エントリ
        alignas(value_type) unsigned char drifterStorage[sizeof(value_type)];
        value_type* drifter = ::new (static_cast<void*>(drifterStorage)) value_type(std::forward<V>(value));
        size_type drifterHash = hc;
        uint8_t drifterDist = dist;

        Slot* insertedSlot = nullptr;

        while (true)
        {
            Slot& slot = slots[idx];

            if (slot.distance == 0)
            {
                // 空スロットに配置
                ::new (static_cast<void*>(&slot.value)) value_type(std::move(*drifter));
                slot.hashCode = drifterHash;
                slot.distance = drifterDist;
                drifter->~value_type();
                ++m_size;
                if (insertedSlot == nullptr)
                    insertedSlot = &slot;
                return { MakeIterator(insertedSlot), true };
            }

            if (slot.distance < drifterDist)
            {
                // Robin Hood swap: 既存エントリの PSL が短い → 入れ替え
                if (insertedSlot == nullptr)
                    insertedSlot = &slot;

                // swap drifter <-> slot
                value_type tmpValue(std::move(slot.value));
                size_type tmpHash = slot.hashCode;
                uint8_t tmpDist = slot.distance;

                slot.value.~value_type();
                ::new (static_cast<void*>(&slot.value)) value_type(std::move(*drifter));
                slot.hashCode = drifterHash;
                slot.distance = drifterDist;

                drifter->~value_type();
                ::new (static_cast<void*>(drifter)) value_type(std::move(tmpValue));
                drifterHash = tmpHash;
                drifterDist = tmpDist;
            }

            ++drifterDist;
            idx = (idx + 1) & mask;

            // 安全弁: distance が 255 を超えることはありえない (正常運用では)
            assert(drifterDist < 250 && "hash table probe overflow — bad hash or extreme load");
        }
    }

    // ====================================================================
    // リハッシュ
    // ====================================================================

    void RehashInternal(size_type newCap)
    {
        assert(newCap > 0 && (newCap & (newCap - 1)) == 0);

        Slot* oldSlots = GetSlots();
        size_type oldCap = m_capacity;
        size_type oldSize = m_size;

        // 新しいスロットを確保
        m_allocPair.Second() = nullptr;
        m_capacity = 0;
        m_size = 0;
        AllocateSlots(newCap);

        // 旧エントリを再挿入
        if (oldSlots)
        {
            for (size_type i = 0; i < oldCap; ++i)
            {
                if (oldSlots[i].distance != 0)
                {
                    InsertRobinHood(std::move(oldSlots[i].value), oldSlots[i].hashCode);
                    oldSlots[i].value.~value_type();
                }
            }
            m_allocPair.First().deallocate(oldSlots, oldCap * sizeof(Slot));
        }

        (void)oldSize;
    }

    // ====================================================================
    // メンバ
    // ====================================================================

    HashFunc m_hash{};
    EqualFunc m_equal{};
    CompressedPair<Alloc, Slot*> m_allocPair;
    size_type m_capacity = 0;
    size_type m_size     = 0;
};

} // namespace gx::container
