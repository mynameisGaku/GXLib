#pragma once
/// @file HashMap.h
/// @brief ハッシュマップ (Robin Hood, 重複キー不可)

#include "pch_common.h"
#include "Container/HashTable.h"

namespace gx::container {

/// @brief ハッシュマップ — キーと値のペアを格納する非順序連想コンテナ
///
/// Robin Hood open-addressing による高速検索。重複キー不可。
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
class HashMap : public HashTable<std::pair<const Key, Value>, Key,
                                  PairExtractKey<std::pair<const Key, Value>>,
                                  HashFunc, EqualFunc, Alloc, false>
{
    using base_type = HashTable<std::pair<const Key, Value>, Key,
                                 PairExtractKey<std::pair<const Key, Value>>,
                                 HashFunc, EqualFunc, Alloc, false>;

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

    HashMap() = default;

    explicit HashMap(size_type initialCapacity,
                     const HashFunc& hash = HashFunc(),
                     const EqualFunc& eq  = EqualFunc(),
                     const Alloc& alloc   = Alloc())
        : base_type(initialCapacity, hash, eq, alloc) {}

    HashMap(std::initializer_list<value_type> ilist)
    {
        this->Reserve(ilist.size());
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

    /// @brief キーに対応する値への参照を返す (assert版 — 見つからなければアサート)
    __forceinline mapped_type& At(const key_type& key)
    {
        iterator it = this->Find(key);
        assert(it != this->end() && "HashMap::At — key not found");
        return it->second;
    }

    __forceinline const mapped_type& At(const key_type& key) const
    {
        const_iterator it = this->Find(key);
        assert(it != this->end() && "HashMap::At — key not found");
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

    /// @brief キーが存在しなければ in-place 構築して挿入 (既存なら何もしない)
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
