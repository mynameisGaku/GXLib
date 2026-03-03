#pragma once
/// @file MultiSet.h
/// @brief 順序付きマルチセット (赤黒木, 重複キー許可)

#include "pch_common.h"
#include "Container/RBTree.h"

namespace gx::container {

/// @brief 順序付きマルチセット — 重複値を許可するソート済みコンテナ
///
/// 赤黒木による O(log N) 検索・挿入・削除。同一値を複数格納可能。
///
/// @tparam Key     値型 (キーと値が同一)
/// @tparam Compare 比較関数 (デフォルト: Less<Key>)
/// @tparam Alloc   アロケータ (デフォルト: Allocator)
template <typename Key,
          typename Compare = Less<Key>,
          typename Alloc   = Allocator>
class MultiSet : public RBTree<Key, Key,
                                IdentityExtractKey<Key>,
                                Compare, Alloc, true>
{
    using base_type = RBTree<Key, Key,
                              IdentityExtractKey<Key>,
                              Compare, Alloc, true>;

public:
    using typename base_type::key_type;
    using typename base_type::value_type;
    using typename base_type::size_type;
    using typename base_type::iterator;
    using typename base_type::const_iterator;

    // ====================================================================
    // コンストラクタ
    // ====================================================================

    MultiSet() = default;

    explicit MultiSet(const Compare& comp, const Alloc& alloc = Alloc())
        : base_type(comp, alloc) {}

    MultiSet(std::initializer_list<value_type> ilist)
    {
        for (auto& v : ilist)
            this->Insert(v);
    }

    // ====================================================================
    // 等範囲
    // ====================================================================

    /// @brief 指定値に一致する全要素の範囲を返す
    using base_type::EqualRange;

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
