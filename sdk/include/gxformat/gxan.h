#pragma once
/// @file gxan.h
/// @brief GXANスタンドアロンアニメーション形式の定義
///
/// .gxanファイルはボーン名ベースのアニメーションデータ。
/// スケルトンに依存しないため、異なるモデル間でアニメーションを共有できる。
/// 読み込み時はbone_matcherでボーン名→インデックスの対応を解決する。

#include "types.h"
#include <cstdint>

namespace gxfmt
{

// ============================================================
// 定数
// ============================================================

static constexpr uint32_t k_GxanMagic   = 0x4E415847; ///< ファイル識別子 'GXAN'
static constexpr uint32_t k_GxanVersion = 1;           ///< 現在のフォーマットバージョン

// ============================================================
// ヘッダ (64B)
// ============================================================

/// @brief GXANファイルの先頭に配置されるヘッダ (64B固定)
struct GxanHeader
{
    uint32_t magic;              ///< ファイル識別子 0x4E415847 ('GXAN')
    uint32_t version;            ///< フォーマットバージョン (現在1)
    uint32_t channelCount;       ///< チャネル数
    float    duration;           ///< 再生時間 (秒)
    uint64_t stringTableOffset;  ///< 文字列テーブルのファイル先頭からのオフセット
    uint32_t stringTableSize;    ///< 文字列テーブルのバイト数
    uint32_t _pad0;              ///< uint64_tアライメント用の明示パディング
    uint64_t channelDescOffset;  ///< GxanChannelDesc配列のファイル先頭からのオフセット
    uint64_t keyDataOffset;      ///< キーフレームデータのファイル先頭からのオフセット
    uint32_t keyDataSize;        ///< キーフレームデータのバイト数
    uint8_t  _reserved[12];     ///< 64Bパディング用予約
};

static_assert(sizeof(GxanHeader) == 64, "GxanHeader must be 64 bytes");

// ============================================================
// チャネル記述子
// ============================================================

/// @brief アニメーションチャネルの記述子 (ボーン名ベース)
/// @details GXMDのAnimationChannelDescとは異なり、boneIndexではなく
///          boneNameIndex (文字列テーブルのオフセット) でボーンを指定する。
struct GxanChannelDesc
{
    uint32_t boneNameIndex;     ///< ボーン名 (文字列テーブルのバイトオフセット)
    uint8_t  target;            ///< ターゲット (0=Translation, 1=Rotation, 2=Scale)
    uint8_t  interpolation;     ///< 補間方法 (0=Linear, 1=Step, 2=CubicSpline)
    uint8_t  _pad[2];
    uint32_t keyCount;          ///< キーフレーム数
    uint32_t dataOffset;        ///< keyDataOffset からのバイトオフセット
};

// キーフレームデータはgxmd.hのVectorKey/QuatKeyを再利用

// ============================================================
// Binary layout:
//   [GxanHeader 64B]
//   [StringTable: uint32_t byteCount + UTF-8 bone name strings]
//   [GxanChannelDesc x channelCount]
//   [Key data (VectorKey / QuatKey arrays)]
// ============================================================

} // namespace gxfmt
