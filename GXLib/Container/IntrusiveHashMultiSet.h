#pragma once
/// @file IntrusiveHashMultiSet.h
/// @brief 侵入型ハッシュマルチセット — 重複キー許可の侵入型ハッシュコンテナ

#include "pch_common.h"
#include "Container/IntrusiveHashSet.h"
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace gx::container {

/// @brief 侵入型ハッシュマルチセット (重複キー許可)
///
/// IntrusiveHashSet と同じ構造だが、Insert 時に重複チェックをしない。
/// FindAll で同一キーの全要素を走査できる。
///
/// @tparam T 要素型
/// @tparam NodeMember IntrusiveHashNode T::* メンバポインタ
/// @tparam HashFunc T からハッシュ値を計算するファンクタ
/// @tparam EqualFunc 等価比較ファンクタ
template <typename T,
          IntrusiveHashNode T::*NodeMember,
          typename HashFunc,
          typename EqualFunc>
class IntrusiveHashMultiSet
{
    using SetType = IntrusiveHashSet<T, NodeMember, HashFunc, EqualFunc>;

public:
    using size_type = size_t;
    using iterator  = typename SetType::iterator;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    IntrusiveHashMultiSet() noexcept = default;
    explicit IntrusiveHashMultiSet(size_type initialBucketCount) : m_set(initialBucketCount) {}
    ~IntrusiveHashMultiSet() = default;

    IntrusiveHashMultiSet(const IntrusiveHashMultiSet&) = delete;
    IntrusiveHashMultiSet& operator=(const IntrusiveHashMultiSet&) = delete;

    IntrusiveHashMultiSet(IntrusiveHashMultiSet&& other) noexcept : m_set(std::move(other.m_set)) {}

    IntrusiveHashMultiSet& operator=(IntrusiveHashMultiSet&& other) noexcept
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

    /// @brief 挿入 (重複チェックなし — マルチセットなので常に挿入)
    void Insert(T& obj) { m_set.Insert(obj); }

    /// @brief 削除 (特定の要素を削除)
    void Remove(T& obj) { m_set.Remove(obj); }

    /// @brief 最初に一致する要素を返す
    T* Find(const T& ref) noexcept { return m_set.Find(ref); }

    /// @brief キーで検索 (テンプレート化)
    template <typename KeyType, typename Predicate>
    T* Find(const KeyType& key, size_t keyHash, Predicate pred) noexcept
    {
        return m_set.Find(key, keyHash, pred);
    }

    /// @brief 指定キーに一致する全要素に対してコールバックを実行
    /// @param key 検索キー
    /// @param keyHash key のハッシュ値
    /// @param pred 比較述語
    /// @param callback 各一致要素に対して呼ばれるコールバック void(T&)
    template <typename KeyType, typename Predicate, typename Callback>
    void FindAll(const KeyType& key, size_t keyHash, Predicate pred, Callback callback) noexcept
    {
        if (m_set.BucketCount() == 0) return;
        size_type bucket = keyHash % m_set.BucketCount();
        // Walk the bucket chain manually — access internals via first Find + chain walk
        // We use the set's Find to get the first, then follow hashNext
        T* first = m_set.Find(key, keyHash, pred);
        if (!first) return;

        // Walk from the found node
        IntrusiveHashNode* node = &(first->*NodeMember);
        while (node)
        {
            T* obj = NodeToObj(node);
            if (pred(*obj, key))
                callback(*obj);
            node = node->hashNext;
        }
    }

    /// @brief 指定キーの出現数
    template <typename KeyType, typename Predicate>
    size_type Count(const KeyType& key, size_t keyHash, Predicate pred) const noexcept
    {
        if (m_set.BucketCount() == 0) return 0;
        size_type count = 0;
        // We need const access to buckets — walk the bucket
        // Since we don't expose buckets directly, use a manual approach:
        // Note: const_cast is safe here as we only read
        auto& self = const_cast<IntrusiveHashMultiSet&>(*this);
        T* first = self.m_set.Find(key, keyHash, pred);
        if (!first) return 0;

        IntrusiveHashNode* node = &(first->*NodeMember);
        while (node)
        {
            T* obj = NodeToObj(node);
            if (pred(*obj, key))
                ++count;
            node = node->hashNext;
        }
        return count;
    }

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
