#pragma once
/// @file IntrusiveHashMap.h
/// @brief 侵入型ハッシュマップ — キー抽出ファンクタでマップセマンティクスを提供

#include "pch_common.h"
#include "Container/IntrusiveHashSet.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace gx::container {

/// @brief 侵入型ハッシュマップ
///
/// IntrusiveHashSet の上にキー抽出ファンクタを重ねてマップセマンティクスを提供する。
/// キーは要素Tの中に含まれ、KeyExtract ファンクタで取り出す。
///
/// @tparam T 要素型
/// @tparam NodeMember IntrusiveHashNode T::* メンバポインタ
/// @tparam KeyType キー型
/// @tparam KeyExtract T からキーを抽出するファンクタ (const KeyType& operator()(const T&))
/// @tparam KeyHash キーのハッシュファンクタ (size_t operator()(const KeyType&))
/// @tparam KeyEqual キーの等価比較ファンクタ (bool operator()(const KeyType&, const KeyType&))
template <typename T,
          IntrusiveHashNode T::*NodeMember,
          typename KeyType,
          typename KeyExtract,
          typename KeyHash,
          typename KeyEqual>
class IntrusiveHashMap
{
    /// @brief T のハッシュ (KeyExtract + KeyHash を合成)
    struct ObjHash
    {
        KeyExtract m_extract;
        KeyHash    m_keyHash;
        size_t operator()(const T& obj) const { return m_keyHash(m_extract(obj)); }
    };

    /// @brief T の等価比較 (KeyExtract + KeyEqual を合成)
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

    IntrusiveHashMap() noexcept = default;
    explicit IntrusiveHashMap(size_type initialBucketCount) : m_set(initialBucketCount) {}
    ~IntrusiveHashMap() = default;

    IntrusiveHashMap(const IntrusiveHashMap&) = delete;
    IntrusiveHashMap& operator=(const IntrusiveHashMap&) = delete;

    IntrusiveHashMap(IntrusiveHashMap&& other) noexcept : m_set(std::move(other.m_set)) {}

    IntrusiveHashMap& operator=(IntrusiveHashMap&& other) noexcept
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

    /// @brief 挿入 (重複チェックなし — 呼び出し側が保証)
    void Insert(T& obj) { m_set.Insert(obj); }

    /// @brief 削除
    void Remove(T& obj) { m_set.Remove(obj); }

    /// @brief キーで検索
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

    /// @brief 指定キーの存在チェック
    bool Contains(const KeyType& key) noexcept { return Find(key) != nullptr; }

    void Rehash(size_type newBucketCount) { m_set.Rehash(newBucketCount); }

    void Clear() noexcept { m_set.Clear(); }

    bool Validate() const noexcept { return m_set.Validate(); }

private:
    SetType m_set;
};

} // namespace gx::container
