#pragma once
/// @file types.h
/// @brief gxformatで共通利用する基本型とユーティリティ関数

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace gxfmt
{

/// @brief 値を指定アライメントの倍数に切り上げる (32bit版)
/// @param value 切り上げ対象の値
/// @param alignment アライメント (2の累乗であること)
/// @return アライメント済みの値
inline constexpr uint32_t AlignUp(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/// @brief 値を指定アライメントの倍数に切り上げる (64bit版)
/// @param value 切り上げ対象の値
/// @param alignment アライメント (2の累乗であること)
/// @return アライメント済みの値
inline constexpr uint64_t AlignUp64(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace gxfmt
