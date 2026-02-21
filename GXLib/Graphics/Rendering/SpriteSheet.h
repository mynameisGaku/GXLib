#pragma once
/// @file SpriteSheet.h
/// @brief スプライトシート分割読み込み
///
/// 1枚のテクスチャをグリッド状に分割し、各セルを個別のハンドルで扱えるようにする。
/// DxLibの LoadDivGraph / DerivationGraph に相当する機能。
///
/// 典型例: 64x64 のキャラ画像を 4列x3行 で並べた画像を12分割し、
/// 各コマを DrawGraph で個別に描画できる。

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief スプライトシート分割読み込みユーティリティ（DxLibの LoadDivGraph に相当）
class SpriteSheet
{
public:
    SpriteSheet() = default;
    ~SpriteSheet() = default;

    /// @brief 画像を等間隔グリッドで分割し、各セルのハンドルを返す
    ///
    /// 内部では1枚のテクスチャをロードし、UV矩形ベースのリージョンハンドルを生成する。
    /// 得られたハンドルは SpriteBatch::DrawGraph() でそのまま使える。
    ///
    /// @param textureManager テクスチャマネージャへの参照
    /// @param filePath 画像ファイルパス
    /// @param allNum 分割総数（xNum * yNum 以下であること）
    /// @param xNum 横方向の分割数
    /// @param yNum 縦方向の分割数
    /// @param xSize 各セルの幅（ピクセル）
    /// @param ySize 各セルの高さ（ピクセル）
    /// @param handleArray 出力先のハンドル配列（allNum 個以上の領域が必要）
    /// @return 成功した場合 true
    static bool LoadDivGraph(TextureManager& textureManager,
                              const std::wstring& filePath,
                              int allNum, int xNum, int yNum,
                              int xSize, int ySize,
                              int* handleArray);
};

} // namespace GX
