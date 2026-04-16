#pragma once
/// @file PPtr.h
/// @brief PPtr — GUID + LFID のシリアライズ可能な永続参照
///
/// Unity の PPtr に相当する軽量参照型。
/// アセットファイルの GUID と、ファイル内のオブジェクト識別子 (LFID) で
/// アセットへの永続的な参照を保持する。
/// @addtogroup grp_core
/// @{

#include "Core/GUID.h"

namespace gx
{

/// @brief アセットへの永続参照 (Persistent Pointer)
/// @tparam T 参照先の型（型安全のための識別用、実行時には使用しない）
template<typename T>
struct PPtr
{
    GUID guid;           ///< どのアセットファイルか
    uint32_t lfid = 0;   ///< そのファイル内のどのオブジェクトか (0=ルート)

    /// @brief 参照が有効か判定する
    /// @return GUID が有効なら true
    bool IsValid() const { return guid.IsValid(); }

    /// @brief bool 変換演算子
    explicit operator bool() const { return IsValid(); }

    /// @brief 等値比較
    bool operator==(const PPtr& other) const { return guid == other.guid && lfid == other.lfid; }

    /// @brief 非等値比較
    bool operator!=(const PPtr& other) const { return !(*this == other); }

    /// @brief 文字列変換 ("guid:lfid" 形式)
    /// @return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx:0" 形式の文字列
    gx::String ToString() const
    {
        char buf[8];
        snprintf(buf, sizeof(buf), ":%u", lfid);
        return guid.ToString() + gx::String(buf);
    }

    /// @brief 文字列からPPtrを生成する
    /// @param str "guid:lfid" 形式の文字列
    /// @return パースされた PPtr
    static PPtr FromString(const gx::String& str)
    {
        PPtr result;
        auto colonPos = str.rfind(':');
        if (colonPos == gx::String::npos || colonPos == 0)
            return result;

        result.guid = GUID::FromString(str.substr(0, colonPos));
        gx::String lfidStr = str.substr(colonPos + 1);
        result.lfid = static_cast<uint32_t>(std::strtoul(lfidStr.c_str(), nullptr, 10));
        return result;
    }
};

} // namespace gx
/// @}
