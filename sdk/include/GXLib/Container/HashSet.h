#pragma once
/// @file HashSet.h
/// @brief ハッシュセット (Robin Hood, 重複キー不可)

#include "pch_common.h"
#include "Container/HashTable.h"

namespace gx::container {

/// @brief ハッシュセット — 一意な値を格納する非順序コンテナ
///
/// Robin Hood open-addressing による高速検索。重複不可。
///
/// @tparam Key       値型 (キーと値が同一)
/// @tparam HashFunc  ハッシュ関数 (デフォルト: Hash<Key>)
/// @tparam EqualFunc 等価比較ファンクタ (デフォルト: EqualTo<Key>)
/// @tparam Alloc     アロケータ (デフォルト: Allocator)
template <typename Key,
          typename HashFunc  = Hash<Key>,
          typename EqualFunc = EqualTo<Key>,
          typename Alloc     = Allocator>
class HashSet : public HashTable<Key, Key,
                                  IdentityExtractKey<Key>,
                                  HashFunc, EqualFunc, Alloc, false>
{
    using base_type = HashTable<Key, Key,
                                 IdentityExtractKey<Key>,
                                 HashFunc, EqualFunc, Alloc, false>;

public:
    using typename base_type::key_type;
    using typename base_type::value_type;
    using typename base_type::size_type;
    using typename base_type::iterator;
    using typename base_type::const_iterator;

    // ====================================================================
    // コンストラクタ
    // ====================================================================

    HashSet() = default;

    explicit HashSet(size_type initialCapacity,
                     const HashFunc& hash = HashFunc(),
                     const EqualFunc& eq  = EqualFunc(),
                     const Alloc& alloc   = Alloc())
        : base_type(initialCapacity, hash, eq, alloc) {}

    HashSet(std::initializer_list<value_type> ilist)
    {
        this->Reserve(ilist.size());
        for (auto& v : ilist)
            this->Insert(v);
    }

    // ====================================================================
    // ユーティリティ
    // ====================================================================

    /// @brief 異種キーで検索 (heterogeneous lookup)
    template <typename AltKey, typename AltHash, typename AltEqual>
    __forceinline iterator FindAs(const AltKey& key, const AltHash& hash, const AltEqual& eq) noexcept
    {
        return base_type::FindAs(key, hash, eq);
    }

    template <typename AltKey, typename AltHash, typename AltEqual>
    __forceinline const_iterator FindAs(const AltKey& key, const AltHash& hash, const AltEqual& eq) const noexcept
    {
        return base_type::FindAs(key, hash, eq);
    }
};

} // namespace gx::container
