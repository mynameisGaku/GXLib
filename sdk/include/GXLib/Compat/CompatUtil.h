#pragma once
/// @file CompatUtil.h
/// @brief 簡易API用内部ユーティリティ

#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace gx_internal
{
/// @addtogroup grp_compat
/// @{

/// @brief マルチバイト文字列をワイド文字列に変換する
/// @param str 変換元のchar文字列
/// @return 変換後のgx::WString
inline gx::WString ToWString(const char* str)
{
    if (!str || str[0] == '\0') return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    gx::WString result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str, -1, result.data(), len);
    return result;
}

/// @}
} // namespace gx_internal
