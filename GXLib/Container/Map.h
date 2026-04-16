#pragma once
/// @file Map.h
/// @brief 順序付きマップ (赤黒木, 重複キー不可)

#include "pch_common.h"
#include "Container/RBTree.h"

namespace gx::container {

/// @brief 順序付きマップ — キーと値のペアを格納するソート済み連想コンテナ
///
/// 赤黒木による O(log N) 検索・挿入・削除。重複キー不可。
///
/// @tparam Key     キー型
/// @tparam Value   値型
/// @tparam Compare 比較関数 (デフォルト: Less<Key>)
/// @tparam Alloc   アロケータ (デフォルト: Allocator)
template <typename Key, typename Value,
          typename Compare = Less<Key>,
          typename Alloc   = Allocator>
class Map : public RBTree<std::pair<const Key, Value>, Key,
                           PairExtractKey<std::pair<const Key, Value>>,
                           Compare, Alloc, false>
{
    using base_type = RBTree<std::pair<const Key, Value>, Key,
                              PairExtractKey<std::pair<const Key, Value>>,
                              Compare, Alloc, false>;

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

    Map() = default;

    explicit Map(const Compare& comp, const Alloc& alloc = Alloc())
        : base_type(comp, alloc) {}

    Map(std::initializer_list<value_type> ilist)
    {
        for (auto& v : ilist)
            this->Insert(v);
    }

    // ====================================================================
    // 要素アクセス
    // ====================================================================

    /// @brief キーに対応する値への参照を返す (無ければデフォルト構築して挿入)
    __forceinline mapped_type& operator[](const key_type& key)
    {
        auto result = this->Emplace(std::piecewise_construct,
                                    std::forward_as_tuple(key),
                                    std::forward_as_tuple());
        return result.first->second;
    }

    /// @brief キーに対応する値への参照を返す (ムーブキー版)
    __forceinline mapped_type& operator[](key_type&& key)
    {
        auto result = this->Emplace(std::piecewise_construct,
                                    std::forward_as_tuple(std::move(key)),
                                    std::forward_as_tuple());
        return result.first->second;
    }

    /// @brief キーに対応する値への参照を返す (assert版)
    __forceinline mapped_type& At(const key_type& key)
    {
        iterator it = this->Find(key);
        assert(it != this->end() && "Map::At — key not found");
        return it->second;
    }

    __forceinline const mapped_type& At(const key_type& key) const
    {
        const_iterator it = this->Find(key);
        assert(it != this->end() && "Map::At — key not found");
        return it->second;
    }

    // ====================================================================
    // std compatibility aliases
    // ====================================================================

    __forceinline mapped_type& at(const key_type& key) { return At(key); }
    __forceinline const mapped_type& at(const key_type& key) const { return At(key); }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const key_type& key, M&& value)
    {
        return InsertOrAssign(key, std::forward<M>(value));
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args)
    {
        return TryEmplace(key, std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type&& key, Args&&... args)
    {
        return TryEmplace(std::move(key), std::forward<Args>(args)...);
    }

    // ====================================================================
    // 挿入
    // ====================================================================

    /// @brief キーが存在すれば上書き、なければ挿入
    template <typename M>
    std::pair<iterator, bool> InsertOrAssign(const key_type& key, M&& value)
    {
        auto result = this->Emplace(std::piecewise_construct,
                                    std::forward_as_tuple(key),
                                    std::forward_as_tuple(std::forward<M>(value)));
        if (!result.second)
        {
            result.first->second = std::forward<M>(value);
        }
        return { result.first, result.second };
    }

    /// @brief キーが存在しなければ in-place 構築して挿入
    template <typename... Args>
    std::pair<iterator, bool> TryEmplace(const key_type& key, Args&&... args)
    {
        iterator it = this->Find(key);
        if (it != this->end())
            return { it, false };

        auto result = this->Emplace(std::piecewise_construct,
                                    std::forward_as_tuple(key),
                                    std::forward_as_tuple(std::forward<Args>(args)...));
        return { result.first, result.second };
    }

    /// @brief ムーブキー版 TryEmplace
    template <typename... Args>
    std::pair<iterator, bool> TryEmplace(key_type&& key, Args&&... args)
    {
        iterator it = this->Find(key);
        if (it != this->end())
            return { it, false };

        auto result = this->Emplace(std::piecewise_construct,
                                    std::forward_as_tuple(std::move(key)),
                                    std::forward_as_tuple(std::forward<Args>(args)...));
        return { result.first, result.second };
    }
};

} // namespace gx::container
