#pragma once
/// @file GUID.h
/// @brief 128-bit GUID (Globally Unique Identifier)
///
/// UUID v4 互換の永続アセットIDを提供する。
/// CoCreateGuid で生成し、文字列変換・ハッシュ・比較をサポート。
/// @addtogroup grp_core
/// @{

#include "pch_common.h"

namespace gx
{

/// @brief 128-bit GUID 構造体
struct GUID
{
    uint64_t hi = 0; ///< 上位64ビット
    uint64_t lo = 0; ///< 下位64ビット

    /// @brief GUIDが有効（非ゼロ）か判定する
    /// @return hi または lo が非ゼロなら true
    bool IsValid() const { return hi != 0 || lo != 0; }

    /// @brief 等値比較
    bool operator==(const GUID& other) const { return hi == other.hi && lo == other.lo; }

    /// @brief 非等値比較
    bool operator!=(const GUID& other) const { return !(*this == other); }

    /// @brief 順序比較（HashMap以外のソートなどに使用）
    bool operator<(const GUID& other) const
    {
        return hi < other.hi || (hi == other.hi && lo < other.lo);
    }

    /// @brief GUID を "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx" 形式の文字列に変換する
    /// @return UUID v4 文字列
    gx::String ToString() const;

    /// @brief UUID v4 文字列からGUIDを生成する
    /// @param str "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx" 形式の文字列
    /// @return パースされたGUID（不正文字列の場合は無効なGUID）
    static GUID FromString(const gx::String& str);

    /// @brief 新しいランダムGUIDを生成する (CoCreateGuid)
    /// @return 新規GUID
    static GUID Generate();
};

} // namespace gx

// ============================================================================
// gx::container::Hash 特殊化
// ============================================================================
namespace gx { namespace container {

template<>
struct Hash<gx::GUID>
{
    size_t operator()(const gx::GUID& guid) const noexcept
    {
        // MurmurHash3Mix で hi/lo を合成
        size_t h = MurmurHash3Mix(static_cast<size_t>(guid.hi));
        h ^= MurmurHash3Mix(static_cast<size_t>(guid.lo)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

}} // namespace gx::container

/// @}
