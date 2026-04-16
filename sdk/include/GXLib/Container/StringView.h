#pragma once
/// @file StringView.h
/// @brief 文字列ビュー (std::string_view エイリアス + Hash特殊化)

#include "pch_common.h"
#include "Container/Hash.h"
#include <string_view>

namespace gx::container {

/// @brief 文字列ビュー (std::string_view のエイリアス)
using StringView = std::string_view;

/// @brief ワイド文字列ビュー (std::wstring_view のエイリアス)
using WStringView = std::wstring_view;

// Hash specializations already provided in Hash.h for:
//   std::string_view   -> Hash<std::string_view>
//   std::wstring_view  -> Hash<std::wstring_view>

} // namespace gx::container
