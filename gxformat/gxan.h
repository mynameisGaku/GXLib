#pragma once
/// @file gxan.h
/// @brief GXAN standalone animation format (bone-name based, skeleton-independent)

#include "types.h"
#include <cstdint>

namespace gxfmt
{

// ============================================================
// Constants
// ============================================================

static constexpr uint32_t k_GxanMagic   = 0x4E415847; // 'GXAN'
static constexpr uint32_t k_GxanVersion = 1;

// ============================================================
// Header (64 bytes)
// ============================================================

struct GxanHeader
{
    uint32_t magic;              // 0x4E415847
    uint32_t version;            // 1
    uint32_t channelCount;
    float    duration;           // seconds
    uint64_t stringTableOffset;  // from file start
    uint32_t stringTableSize;
    uint32_t _pad0;              // explicit padding for uint64_t alignment
    uint64_t channelDescOffset;  // from file start
    uint64_t keyDataOffset;      // from file start
    uint32_t keyDataSize;
    uint8_t  _reserved[12];     // pad to 64 bytes
};

static_assert(sizeof(GxanHeader) == 64, "GxanHeader must be 64 bytes");

// ============================================================
// Channel descriptor
// ============================================================

struct GxanChannelDesc
{
    uint32_t boneNameIndex;     // StringTable byte offset (bone name, not index)
    uint8_t  target;            // 0=Translation, 1=Rotation, 2=Scale
    uint8_t  interpolation;     // 0=Linear, 1=Step, 2=CubicSpline
    uint8_t  _pad[2];
    uint32_t keyCount;
    uint32_t dataOffset;        // byte offset from keyDataOffset
};

// Key data reuses VectorKey/QuatKey from gxmd.h

// ============================================================
// Binary layout:
//   [GxanHeader 64B]
//   [StringTable: uint32_t byteCount + UTF-8 bone name strings]
//   [GxanChannelDesc x channelCount]
//   [Key data (VectorKey / QuatKey arrays)]
// ============================================================

} // namespace gxfmt
