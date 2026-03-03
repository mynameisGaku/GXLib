#pragma once
/// @file anim_loader.h
/// @brief GXANアニメーションランタイムローダー
///
/// .gxanファイルを読み込み、ボーン名ベースのアニメーションデータを
/// LoadedGxan構造体に展開する。ボーン名→インデックスの解決には
/// bone_matcherを使用する。

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "gxan.h"
#include "gxmd.h"

namespace gxloader
{

struct LoadedJoint; // model_loader.hから前方宣言

/// @brief GXANのアニメーションチャネル (ボーン名ベース)
/// @details GXMDのLoadedAnimChannelとは異なり、jointIndex代わりにboneNameを持つ。
struct LoadedAnimChannelGxan
{
    std::string boneName;       ///< ボーン名 (リターゲティング用)
    uint8_t     target = 0;     ///< ターゲット (0=Translation, 1=Rotation, 2=Scale)
    uint8_t     interpolation = 0; ///< 補間方法 (0=Linear, 1=Step, 2=CubicSpline)
    std::vector<gxfmt::VectorKey> vecKeys;  ///< float3キーフレーム (T/S用)
    std::vector<gxfmt::QuatKey>   quatKeys; ///< クォータニオンキーフレーム (R用)
};

/// @brief GXANから読み込んだアニメーションデータ
struct LoadedGxan
{
    float duration = 0.0f;  ///< 再生時間 (秒)
    std::vector<LoadedAnimChannelGxan> channels; ///< チャネル一覧
};

/// @brief ディスクからGXANファイルを読み込む
/// @param filePath .gxanファイルパス
/// @return 読み込み済みアニメーション。失敗時nullptr
std::unique_ptr<LoadedGxan> LoadGxan(const std::string& filePath);

/// @brief メモリバッファからGXANを読み込む
/// @param data バッファ先頭ポインタ
/// @param size バッファサイズ
/// @return 読み込み済みアニメーション。失敗時nullptr
std::unique_ptr<LoadedGxan> LoadGxanFromMemory(const uint8_t* data, size_t size);

#ifdef _WIN32
/// @brief ワイド文字パスでGXANファイルを読み込む (Windows非ASCIIパス対応)
/// @param filePath ワイド文字パス
/// @return 読み込み済みアニメーション。失敗時nullptr
std::unique_ptr<LoadedGxan> LoadGxanW(const std::wstring& filePath);
#endif

} // namespace gxloader
