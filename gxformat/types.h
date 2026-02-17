#pragma once
/// @file types.h
/// @brief gxformat common types and utilities

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace gxfmt
{

inline constexpr uint32_t AlignUp(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

inline constexpr uint64_t AlignUp64(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace gxfmt
