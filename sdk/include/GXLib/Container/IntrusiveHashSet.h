#pragma once
/// @file IntrusiveHashSet.h
/// @brief 侵入型ハッシュセット — 要素自体にノードを埋め込み、別鎖法で管理

#include "pch_common.h"
#include "Container/Allocator.h"
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 侵入型ハッシュノード基底 (要素に埋め込む)
struct IntrusiveHashNode
{
    IntrusiveHashNode* hashNext = nullptr;
    size_t cachedHash = 0;
};

/// @brief 侵入型ハッシュセット (別鎖法)
///
/// 要素型Tは IntrusiveHashNode メンバを持ち、NodeMember でそのメンバポインタを指定する。
/// 要素のメモリ管理は呼び出し側の責任。Insert/Remove はリンク操作のみ。
///
/// @tparam T 要素型
/// @tparam NodeMember IntrusiveHashNode T::* メンバポインタ
/// @tparam HashFunc T からハッシュ値を計算するファンクタ (size_t operator()(const T&))
/// @tparam EqualFunc 等価比較ファンクタ (bool operator()(const T&, const T&))
template <typename T,
          IntrusiveHashNode T::*NodeMember,
          typename HashFunc,
          typename EqualFunc>
class IntrusiveHashSet
{
public:
    using size_type = size_t;

    // ========================================================================
    // Iterator
    // ========================================================================
    class Iterator
    {
    public:
        Iterator() noexcept = default;
        Iterator(IntrusiveHashNode** buckets, size_type bucketCount, size_type bucket, IntrusiveHashNode* node) noexcept
            : m_buckets(buckets), m_bucketCount(bucketCount), m_bucket(bucket), m_node(node)
        {
            AdvanceToValid();
        }

        __forceinline T& operator*() const noexcept { return *NodeToObj(m_node); }
        __forceinline T* operator->() const noexcept { return NodeToObj(m_node); }

        __forceinline Iterator& operator++() noexcept
        {
            m_node = m_node->hashNext;
            if (!m_node)
            {
                ++m_bucket;
                m_node = nullptr;
                AdvanceToValid();
            }
            return *this;
        }

        __forceinline Iterator operator++(int) noexcept { auto t = *this; ++(*this); return t; }

        __forceinline friend bool operator==(Iterator a, Iterator b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(Iterator a, Iterator b) noexcept { return a.m_node != b.m_node; }

    private:
        void AdvanceToValid()
        {
            while (!m_node && m_bucket < m_bucketCount)
            {
                m_node = m_buckets[m_bucket];
                if (!m_node) ++m_bucket;
            }
        }

        IntrusiveHashNode** m_buckets = nullptr;
        size_type m_bucketCount = 0;
        size_type m_bucket = 0;
        IntrusiveHashNode* m_node = nullptr;
    };

    using iterator = Iterator;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    IntrusiveHashSet() noexcept = default;

    explicit IntrusiveHashSet(size_type initialBucketCount)
    {
        Rehash(initialBucketCount);
    }

    ~IntrusiveHashSet()
    {
        FreeBuckets();
    }

    IntrusiveHashSet(const IntrusiveHashSet&) = delete;
    IntrusiveHashSet& operator=(const IntrusiveHashSet&) = delete;

    IntrusiveHashSet(IntrusiveHashSet&& other) noexcept
        : m_buckets(other.m_buckets), m_bucketCount(other.m_bucketCount), m_size(other.m_size)
    {
        other.m_buckets = nullptr;
        other.m_bucketCount = 0;
        other.m_size = 0;
    }

    IntrusiveHashSet& operator=(IntrusiveHashSet&& other) noexcept
    {
        if (this != &other)
        {
            FreeBuckets();
            m_buckets = other.m_buckets;
            m_bucketCount = other.m_bucketCount;
            m_size = other.m_size;
            other.m_buckets = nullptr;
            other.m_bucketCount = 0;
            other.m_size = 0;
        }
        return *this;
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline bool      Empty() const noexcept { return m_size == 0; }
    __forceinline size_type BucketCount() const noexcept { return m_bucketCount; }

    float LoadFactor() const noexcept
    {
        return m_bucketCount > 0 ? static_cast<float>(m_size) / static_cast<float>(m_bucketCount) : 0.0f;
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    iterator begin() noexcept { return Iterator(m_buckets, m_bucketCount, 0, nullptr); }
    iterator end()   noexcept { return Iterator(m_buckets, m_bucketCount, m_bucketCount, nullptr); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 要素を挿入 (重複チェックなし)
    void Insert(T& obj)
    {
        if (m_bucketCount == 0)
            Rehash(16);
        else if (LoadFactor() > 0.75f)
            Rehash(m_bucketCount * 2);

        IntrusiveHashNode& node = obj.*NodeMember;
        node.cachedHash = m_hash(obj);
        size_type bucket = node.cachedHash % m_bucketCount;
        node.hashNext = m_buckets[bucket];
        m_buckets[bucket] = &node;
        ++m_size;
    }

    /// @brief 要素を削除
    void Remove(T& obj)
    {
        if (m_bucketCount == 0) return;
        IntrusiveHashNode& node = obj.*NodeMember;
        size_type bucket = node.cachedHash % m_bucketCount;

        IntrusiveHashNode** pp = &m_buckets[bucket];
        while (*pp)
        {
            if (*pp == &node)
            {
                *pp = node.hashNext;
                node.hashNext = nullptr;
                --m_size;
                return;
            }
            pp = &(*pp)->hashNext;
        }
        assert(false && "IntrusiveHashSet::Remove: object not found");
    }

    /// @brief キーで検索 (テンプレート化されたキー型を受け入れる)
    /// @param key 検索キー
    /// @param keyHash key のハッシュ値
    /// @param pred 比較述語 bool(const T&, const KeyType&)
    template <typename KeyType, typename Predicate>
    T* Find(const KeyType& key, size_t keyHash, Predicate pred) noexcept
    {
        if (m_bucketCount == 0) return nullptr;
        size_type bucket = keyHash % m_bucketCount;
        IntrusiveHashNode* cur = m_buckets[bucket];
        while (cur)
        {
            T* obj = NodeToObj(cur);
            if (pred(*obj, key))
                return obj;
            cur = cur->hashNext;
        }
        return nullptr;
    }

    /// @brief 要素同士の等価比較で検索
    T* Find(const T& ref) noexcept
    {
        size_t h = m_hash(ref);
        return Find(ref, h, [this](const T& a, const T& b) { return m_equal(a, b); });
    }

    /// @brief バケット配列を再構築
    void Rehash(size_type newBucketCount)
    {
        if (newBucketCount == 0) newBucketCount = 1;

        // Collect existing nodes
        IntrusiveHashNode* all = nullptr;
        if (m_buckets)
        {
            for (size_type i = 0; i < m_bucketCount; ++i)
            {
                IntrusiveHashNode* cur = m_buckets[i];
                while (cur)
                {
                    IntrusiveHashNode* next = cur->hashNext;
                    cur->hashNext = all;
                    all = cur;
                    cur = next;
                }
            }
        }

        FreeBuckets();

        Allocator alloc;
        m_buckets = static_cast<IntrusiveHashNode**>(
            alloc.allocate(newBucketCount * sizeof(IntrusiveHashNode*)));
        m_bucketCount = newBucketCount;
        std::memset(m_buckets, 0, m_bucketCount * sizeof(IntrusiveHashNode*));

        // Re-insert
        while (all)
        {
            IntrusiveHashNode* next = all->hashNext;
            size_type bucket = all->cachedHash % m_bucketCount;
            all->hashNext = m_buckets[bucket];
            m_buckets[bucket] = all;
            all = next;
        }
    }

    /// @brief 全リンクを解除 (要素は破棄しない)
    void Clear() noexcept
    {
        if (m_buckets)
        {
            for (size_type i = 0; i < m_bucketCount; ++i)
            {
                IntrusiveHashNode* cur = m_buckets[i];
                while (cur)
                {
                    IntrusiveHashNode* next = cur->hashNext;
                    cur->hashNext = nullptr;
                    cur = next;
                }
                m_buckets[i] = nullptr;
            }
        }
        m_size = 0;
    }

    bool Validate() const noexcept { return true; }

private:
    static T* NodeToObj(IntrusiveHashNode* node) noexcept
    {
        // Compute offset of NodeMember within T using null pointer trick
        static const size_t offset =
            reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*NodeMember));
        return reinterpret_cast<T*>(reinterpret_cast<char*>(node) - offset);
    }

    void FreeBuckets()
    {
        if (m_buckets)
        {
            Allocator alloc;
            alloc.deallocate(m_buckets, m_bucketCount * sizeof(IntrusiveHashNode*));
            m_buckets = nullptr;
            m_bucketCount = 0;
        }
    }

    IntrusiveHashNode** m_buckets = nullptr;
    size_type m_bucketCount = 0;
    size_type m_size = 0;
    HashFunc  m_hash;
    EqualFunc m_equal;
};

} // namespace gx::container
