#pragma once
/// @file HashMultiMap.h
/// @brief ハッシュマルチマップ (Robin Hood, 重複キー許可)

#include "pch_common.h"
#include "Container/HashTable.h"

namespace gx::container {

/// @brief ハッシュマルチマップ — 重複キーを許可するキー/値ペアコンテナ
///
/// Robin Hood open-addressing 実装。同一キーに対して複数の値を格納可能。
///
/// @tparam Key       キー型
/// @tparam Value     値型
/// @tparam HashFunc  ハッシュ関数 (デフォルト: Hash<Key>)
/// @tparam EqualFunc 等価比較ファンクタ (デフォルト: EqualTo<Key>)
/// @tparam Alloc     アロケータ (デフォルト: Allocator)
template <typename Key, typename Value,
          typename HashFunc  = Hash<Key>,
          typename EqualFunc = EqualTo<Key>,
          typename Alloc     = Allocator>
class HashMultiMap : public HashTable<std::pair<const Key, Value>, Key,
                                       PairExtractKey<std::pair<const Key, Value>>,
                                       HashFunc, EqualFunc, Alloc, true>
{
    using base_type = HashTable<std::pair<const Key, Value>, Key,
                                 PairExtractKey<std::pair<const Key, Value>>,
                                 HashFunc, EqualFunc, Alloc, true>;

public:
    using typename base_type::key_type;
    using typename base_type::value_type;
    using typename base_type::size_type;
    using typename base_type::iterator;
    using typename base_type::const_iterator;
    using mapped_type = Value;

    // ====================================================================
    // コンストラクタ
    // ====================================================================

    HashMultiMap() = default;

    explicit HashMultiMap(size_type initialCapacity,
                          const HashFunc& hash = HashFunc(),
                          const EqualFunc& eq  = EqualFunc(),
                          const Alloc& alloc   = Alloc())
        : base_type(initialCapacity, hash, eq, alloc) {}

    HashMultiMap(std::initializer_list<value_type> ilist)
    {
        this->Reserve(ilist.size());
        for (auto& v : ilist)
            this->Insert(v);
    }

    // ====================================================================
    // 等範囲
    // ====================================================================

    /// @brief 指定キーに一致する全要素の範囲を返す
    using base_type::EqualRange;
};

} // namespace gx::container
