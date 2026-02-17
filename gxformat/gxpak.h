#pragma once
/// @file gxpak.h
/// @brief GXPAK asset bundle format (multi-asset archive with optional LZ4 compression)

#include "types.h"
#include <cstdint>

namespace gxfmt
{

// ============================================================
// Constants
// ============================================================

static constexpr uint32_t k_GxpakMagic   = 0x4B505847; // 'GXPK'
static constexpr uint32_t k_GxpakVersion = 1;

// ============================================================
// Asset type
// ============================================================

enum class GxpakAssetType : uint8_t
{
    Model     = 0,   // .gxmd
    Animation = 1,   // .gxan
    Texture   = 2,   // .png, .jpg, .dds, etc.
    Other     = 255,
};

// ============================================================
// Header (32 bytes)
// ============================================================

struct GxpakHeader
{
    uint32_t magic;           // 0x4B505847
    uint32_t version;         // 1
    uint32_t entryCount;
    uint32_t flags;           // bit0: has LZ4 compressed entries
    uint64_t tocOffset;       // TOC is at end of file
    uint64_t tocSize;         // in bytes
};

static_assert(sizeof(GxpakHeader) == 32, "GxpakHeader must be 32 bytes");

// ============================================================
// TOC entry (stored at end of file)
// ============================================================

struct GxpakTocEntry
{
    uint32_t       pathLength;       // UTF-8 path string length (not including null)
    // followed by pathLength bytes of UTF-8 path string (null-terminated)
    GxpakAssetType assetType;
    uint8_t        compressed;       // 1 = LZ4 compressed
    uint8_t        _pad[2];
    uint64_t       dataOffset;       // from file start
    uint32_t       compressedSize;   // size on disk
    uint32_t       originalSize;     // uncompressed size
};

// Note: TOC entries are variable-length due to path string.
// In-memory representation after loading:

struct GxpakEntry
{
    char           path[260];        // UTF-8 path within bundle
    GxpakAssetType assetType;
    bool           compressed;
    uint64_t       dataOffset;
    uint32_t       compressedSize;
    uint32_t       originalSize;
};

// ============================================================
// Binary layout:
//   [GxpakHeader 32B]
//   [Entry data blocks (contiguous, per-entry)]
//   [TOC at tocOffset: serialized GxpakTocEntry array]
// ============================================================

// Asset type detection from file extension
inline GxpakAssetType DetectAssetType(const char* path)
{
    if (!path) return GxpakAssetType::Other;
    const char* dot = nullptr;
    for (const char* p = path; *p; ++p)
        if (*p == '.') dot = p;
    if (!dot) return GxpakAssetType::Other;

    auto eq = [](const char* a, const char* b) -> bool {
        while (*a && *b) {
            char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
            char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
            if (ca != cb) return false;
            ++a; ++b;
        }
        return *a == *b;
    };

    if (eq(dot, ".gxmd")) return GxpakAssetType::Model;
    if (eq(dot, ".gxan")) return GxpakAssetType::Animation;
    if (eq(dot, ".png") || eq(dot, ".jpg") || eq(dot, ".jpeg") ||
        eq(dot, ".dds") || eq(dot, ".tga") || eq(dot, ".bmp"))
        return GxpakAssetType::Texture;

    return GxpakAssetType::Other;
}

} // namespace gxfmt
