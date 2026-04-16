#pragma once
/// @file IntrusiveHashMultiMap.h
/// @brief 侵入型ハッシュマルチマップ — 重複キー許可、キー抽出ファンクタでマップセマンティクス

#include "pch_common.h"
#include "Container/IntrusiveHashSet.h"
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace gx::container {

/// @brief 侵入型ハッシュマルチマップ (重複キー許可)
///
/// IntrusiveHashSet をベースに、KeyExtract でマップセマンティクスを提供。
/// 同一キーの複数要素を保持できる。
///
/// @tparam T 要素型
/// @tparam NodeMember IntrusiveHashNode T::* メンバポインタ
/// @tparam KeyType キー型
/// @tparam KeyExtract T からキーを抽出するファンクタ
/// @tparam KeyHash キーのハッシュファンクタ
/// @tparam KeyEqual キーの等価比較ファンクタ
template <typename T,
          IntrusiveHashNode T::*NodeMember,
          typename KeyType,
          typename KeyExtract,
          typename KeyHash,
          typename KeyEqual>
class IntrusiveHashMultiMap
{
    struct ObjHash
    {
        KeyExtract m_extract;
        KeyHash    m_keyHash;
        size_t operator()(const T& obj) const { return m_keyHash(m_extract(obj)); }
    };

    struct ObjEqual
    {
        KeyExtract m_extract;
        KeyEqual   m_keyEqual;
        bool operator()(const T& a, const T& b) const { return m_keyEqual(m_extract(a), m_extract(b)); }
    };

    using SetType = IntrusiveHashSet<T, NodeMember, ObjHash, ObjEqual>;

public:
    using size_type = size_t;
    using iterator  = typename SetType::iterator;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    IntrusiveHashMultiMap() noexcept = default;
    explicit IntrusiveHashMultiMap(size_type initialBucketCount) : m_set(initialBucketCount) {}
    ~IntrusiveHashMultiMap() = default;

    IntrusiveHashMultiMap(const IntrusiveHashMultiMap&) = delete;
    IntrusiveHashMultiMap& operator=(const IntrusiveHashMultiMap&) = delete;

    IntrusiveHashMultiMap(IntrusiveHashMultiMap&& other) noexcept : m_set(std::move(other.m_set)) {}

    IntrusiveHashMultiMap& operator=(IntrusiveHashMultiMap&& other) noexcept
    {
        m_set = std::move(other.m_set);
        return *this;
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_set.Size(); }
    __forceinline bool      Empty() const noexcept { return m_set.Empty(); }
    __forceinline size_type BucketCount() const noexcept { return m_set.BucketCount(); }

    // ========================================================================
    // Iterators
    // ========================================================================

    iterator begin() noexcept { return m_set.begin(); }
    iterator end()   noexcept { return m_set.end(); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 挿入 (重複キー許可)
    void Insert(T& obj) { m_set.Insert(obj); }

    /// @brief 削除 (特定の要素を削除)
    void Remove(T& obj) { m_set.Remove(obj); }

    /// @brief キーで最初の一致要素を検索
    T* Find(const KeyType& key) noexcept
    {
        KeyHash kh;
        KeyEqual ke;
        size_t h = kh(key);
        return m_set.Find(key, h, [&ke](const T& obj, const KeyType& k) {
            KeyExtract extract;
            return ke(extract(obj), k);
        });
    }

    /// @brief 指定キーに一致する全要素に対してコールバックを実行
    template <typename Callback>
    void FindAll(const KeyType& key, Callback callback) noexcept
    {
        KeyHash kh;
        KeyEqual ke;
        size_t h = kh(key);
        if (m_set.BucketCount() == 0) return;
        size_type bucket = h % m_set.BucketCount();

        // Find first, then walk chain
        T* first = Find(key);
        if (!first) return;

        IntrusiveHashNode* node = &(first->*NodeMember);
        while (node)
        {
            T* obj = NodeToObj(node);
            KeyExtract extract;
            if (ke(extract(*obj), key))
                callback(*obj);
            node = node->hashNext;
        }
    }

    /// @brief キーの存在チェック
    bool Contains(const KeyType& key) noexcept { return Find(key) != nullptr; }

    void Rehash(size_type newBucketCount) { m_set.Rehash(newBucketCount); }
    void Clear() noexcept { m_set.Clear(); }
    bool Validate() const noexcept { return m_set.Validate(); }

private:
    static T* NodeToObj(IntrusiveHashNode* node) noexcept
    {
        static const size_t offset =
            reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*NodeMember));
        return reinterpret_cast<T*>(reinterpret_cast<char*>(node) - offset);
    }

    SetType m_set;
};

} // namespace gx::container
