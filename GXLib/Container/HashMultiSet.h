#pragma once
/// @file HashMultiSet.h
/// @brief ハッシュマルチセット (Robin Hood, 重複キー許可)

#include "pch_common.h"
#include "Container/HashTable.h"

namespace gx::container {

/// @brief ハッシュマルチセット — 重複値を許可する非順序コンテナ
///
/// Robin Hood open-addressing 実装。同一値を複数格納可能。
///
/// @tparam Key       値型 (キーと値が同一)
/// @tparam HashFunc  ハッシュ関数 (デフォルト: Hash<Key>)
/// @tparam EqualFunc 等価比較ファンクタ (デフォルト: EqualTo<Key>)
/// @tparam Alloc     アロケータ (デフォルト: Allocator)
template <typename Key,
          typename HashFunc  = Hash<Key>,
          typename EqualFunc = EqualTo<Key>,
          typename Alloc     = Allocator>
class HashMultiSet : public HashTable<Key, Key,
                                       IdentityExtractKey<Key>,
                                       HashFunc, EqualFunc, Alloc, true>
{
    using base_type = HashTable<Key, Key,
                                 IdentityExtractKey<Key>,
                                 HashFunc, EqualFunc, Alloc, true>;

public:
    using typename base_type::key_type;
    using typename base_type::value_type;
    using typename base_type::size_type;
    using typename base_type::iterator;
    using typename base_type::const_iterator;

    // ====================================================================
    // コンストラクタ
    // ====================================================================

    HashMultiSet() = default;

    explicit HashMultiSet(size_type initialCapacity,
                          const HashFunc& hash = HashFunc(),
                          const EqualFunc& eq  = EqualFunc(),
                          const Alloc& alloc   = Alloc())
        : base_type(initialCapacity, hash, eq, alloc) {}

    HashMultiSet(std::initializer_list<value_type> ilist)
    {
        this->Reserve(ilist.size());
        for (auto& v : ilist)
            this->Insert(v);
    }

    // ====================================================================
    // 等範囲
    // ====================================================================

    /// @brief 指定値に一致する全要素の範囲を返す
    using base_type::EqualRange;
};

} // namespace gx::container
