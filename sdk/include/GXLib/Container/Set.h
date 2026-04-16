#pragma once
/// @file Set.h
/// @brief 順序付きセット (赤黒木, 重複キー不可)

#include "pch_common.h"
#include "Container/RBTree.h"

namespace gx::container {

/// @brief 順序付きセット — 一意な値をソート順に格納するコンテナ
///
/// 赤黒木による O(log N) 検索・挿入・削除。重複不可。
///
/// @tparam Key     値型 (キーと値が同一)
/// @tparam Compare 比較関数 (デフォルト: Less<Key>)
/// @tparam Alloc   アロケータ (デフォルト: Allocator)
template <typename Key,
          typename Compare = Less<Key>,
          typename Alloc   = Allocator>
class Set : public RBTree<Key, Key,
                           IdentityExtractKey<Key>,
                           Compare, Alloc, false>
{
    using base_type = RBTree<Key, Key,
                              IdentityExtractKey<Key>,
                              Compare, Alloc, false>;

public:
    using typename base_type::key_type;
    using typename base_type::value_type;
    using typename base_type::size_type;
    using typename base_type::iterator;
    using typename base_type::const_iterator;

    // ====================================================================
    // コンストラクタ
    // ====================================================================

    Set() = default;

    explicit Set(const Compare& comp, const Alloc& alloc = Alloc())
        : base_type(comp, alloc) {}

    Set(std::initializer_list<value_type> ilist)
    {
        for (auto& v : ilist)
            this->Insert(v);
    }

    // ====================================================================
    // ユーティリティ
    // ====================================================================

    /// @brief 区間 [lower, upper) に含まれる要素数を返す
    size_type CountRange(const key_type& lower, const key_type& upper) const
    {
        size_type n = 0;
        for (auto it = this->LowerBound(lower); it != this->UpperBound(upper); ++it)
            ++n;
        return n;
    }
};

} // namespace gx::container
