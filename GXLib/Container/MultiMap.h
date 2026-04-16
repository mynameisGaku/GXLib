#pragma once
/// @file MultiMap.h
/// @brief 順序付きマルチマップ (赤黒木, 重複キー許可)

#include "pch_common.h"
#include "Container/RBTree.h"

namespace gx::container {

/// @brief 順序付きマルチマップ — 重複キーを許可するソート済み連想コンテナ
///
/// 赤黒木による O(log N) 検索・挿入・削除。同一キーに対して複数の値を格納可能。
///
/// @tparam Key     キー型
/// @tparam Value   値型
/// @tparam Compare 比較関数 (デフォルト: Less<Key>)
/// @tparam Alloc   アロケータ (デフォルト: Allocator)
template <typename Key, typename Value,
          typename Compare = Less<Key>,
          typename Alloc   = Allocator>
class MultiMap : public RBTree<std::pair<const Key, Value>, Key,
                                PairExtractKey<std::pair<const Key, Value>>,
                                Compare, Alloc, true>
{
    using base_type = RBTree<std::pair<const Key, Value>, Key,
                              PairExtractKey<std::pair<const Key, Value>>,
                              Compare, Alloc, true>;

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

    MultiMap() = default;

    explicit MultiMap(const Compare& comp, const Alloc& alloc = Alloc())
        : base_type(comp, alloc) {}

    MultiMap(std::initializer_list<value_type> ilist)
    {
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
