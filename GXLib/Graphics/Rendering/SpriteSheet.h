#pragma once
/// @file SpriteSheet.h
/// @brief スプライトシート（分割画像）管理クラス
///
/// 【初学者向け解説】
/// スプライトシートとは、複数のキャラクター画像やアニメーションフレームを
/// 1枚の大きな画像にまとめたものです。DXLibのLoadDivGraph()に相当します。
///
/// 例：64x64のキャラクター画像を4列3行で並べた192x256の画像
/// LoadDivGraph で12分割すると、各セルを個別のハンドルで描画できます。

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"

namespace GX
{

/// @brief スプライトシートからの分割読み込み
class SpriteSheet
{
public:
    SpriteSheet() = default;
    ~SpriteSheet() = default;

    /// 画像を分割して読み込み（DXLib LoadDivGraph互換）
    /// @param textureManager テクスチャマネージャー
    /// @param filePath 画像ファイルパス
    /// @param allNum 分割総数
    /// @param xNum 横方向の分割数
    /// @param yNum 縦方向の分割数
    /// @param xSize 各分割画像の幅
    /// @param ySize 各分割画像の高さ
    /// @param handleArray 出力先のハンドル配列（allNum個以上の領域が必要）
    /// @return 成功でtrue
    static bool LoadDivGraph(TextureManager& textureManager,
                              const std::wstring& filePath,
                              int allNum, int xNum, int yNum,
                              int xSize, int ySize,
                              int* handleArray);
};

} // namespace GX
