#pragma once
/// @file gxpak.h
/// @brief GXPAKアセットバンドル形式の定義
///
/// .gxpakファイルは複数のアセット(.gxmd, .gxan, テクスチャ等)を
/// 単一アーカイブにまとめる形式。エントリ単位でLZ4圧縮に対応する。
/// gxpakツールで生成し、gxloader::PakLoaderまたはPakFileProviderで読み込む。

#include "types.h"
#include <cstdint>

namespace gxfmt
{

// ============================================================
// 定数
// ============================================================

static constexpr uint32_t k_GxpakMagic   = 0x4B505847; ///< ファイル識別子 'GXPK'
static constexpr uint32_t k_GxpakVersion = 1;           ///< 現在のフォーマットバージョン

// ============================================================
// アセット種別
// ============================================================

/// @brief バンドル内アセットの種別 (拡張子から自動判定)
enum class GxpakAssetType : uint8_t
{
    Model     = 0,   ///< .gxmd モデル
    Animation = 1,   ///< .gxan アニメーション
    Texture   = 2,   ///< .png, .jpg, .dds 等のテクスチャ
    Other     = 255, ///< その他
};

// ============================================================
// ヘッダ (32B)
// ============================================================

/// @brief GXPAKファイルの先頭に配置されるヘッダ (32B固定)
struct GxpakHeader
{
    uint32_t magic;           ///< ファイル識別子 0x4B505847 ('GXPK')
    uint32_t version;         ///< フォーマットバージョン (現在1)
    uint32_t entryCount;      ///< エントリ数
    uint32_t flags;           ///< フラグ (bit0: LZ4圧縮エントリあり)
    uint64_t tocOffset;       ///< TOCのファイル先頭からのオフセット (ファイル末尾に配置)
    uint64_t tocSize;         ///< TOCのバイト数
};

static_assert(sizeof(GxpakHeader) == 32, "GxpakHeader must be 32 bytes");

// ============================================================
// TOCエントリ (ファイル末尾に配置)
// ============================================================

/// @brief TOCのディスク上シリアライズ形式 (可変長)
/// @details pathLengthの直後にpathLength バイトのUTF-8パス文字列が続く。
struct GxpakTocEntry
{
    uint32_t       pathLength;       ///< パス文字列のバイト長 (null終端含まず)
    // ↓ pathLengthバイトのUTF-8パス文字列 (null終端)
    GxpakAssetType assetType;        ///< アセット種別
    uint8_t        compressed;       ///< LZ4圧縮フラグ (1=圧縮済み)
    uint8_t        _pad[2];
    uint64_t       dataOffset;       ///< データのファイル先頭からのオフセット
    uint32_t       compressedSize;   ///< ディスク上のサイズ (圧縮後)
    uint32_t       originalSize;     ///< 非圧縮時のサイズ
};

/// @brief TOCエントリのメモリ上表現 (固定長)
/// @details PakLoaderが読み込み後に保持する形式。パスは260文字まで。
struct GxpakEntry
{
    char           path[260];        ///< バンドル内のUTF-8パス
    GxpakAssetType assetType;        ///< アセット種別
    bool           compressed;       ///< LZ4圧縮フラグ
    uint64_t       dataOffset;       ///< データのファイル先頭からのオフセット
    uint32_t       compressedSize;   ///< ディスク上のサイズ
    uint32_t       originalSize;     ///< 非圧縮時のサイズ
};

// ============================================================
// Binary layout:
//   [GxpakHeader 32B]
//   [Entry data blocks (contiguous, per-entry)]
//   [TOC at tocOffset: serialized GxpakTocEntry array]
// ============================================================

/// @brief ファイル拡張子からアセット種別を判定する
/// @param path ファイルパス (拡張子部分のみ使用)
/// @return 判定されたアセット種別。不明な場合はOther
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
