#pragma once
/// @file TextRenderer.h
/// @brief テキストレンダラー — SpriteBatchを利用したテキスト描画
///
/// 【初学者向け解説】
/// TextRendererは、FontManagerで作成したフォントアトラスと
/// SpriteBatchの矩形描画機能を組み合わせてテキストを描画します。
///
/// 文字列の各文字について：
/// 1. FontManagerからグリフ情報（アトラス上の位置）を取得
/// 2. SpriteBatch::DrawRectGraphで該当部分を描画
/// 3. 次の文字の位置に移動（advance幅分）
///
/// DXLibのDrawString/DrawFormatStringに相当する機能です。

#include "pch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Math/Transform2D.h"

namespace GX
{

class SpriteBatch;

/// @brief SpriteBatchベースのテキスト描画クラス
class TextRenderer
{
public:
    TextRenderer() = default;
    ~TextRenderer() = default;

    /// @brief テキストレンダラーを初期化する
    /// @param spriteBatch 描画に使用するSpriteBatchへのポインタ
    /// @param fontManager フォント管理に使用するFontManagerへのポインタ
    void Initialize(SpriteBatch* spriteBatch, FontManager* fontManager);

    /// @brief 文字列を描画する（SpriteBatchのBegin/End間で呼ぶこと）
    /// @param fontHandle フォントハンドル（FontManager::CreateFontで取得）
    /// @param x 描画開始X座標
    /// @param y 描画開始Y座標
    /// @param text 描画する文字列
    /// @param color 文字色（0xAARRGGBB形式）
    void DrawString(int fontHandle, float x, float y, const std::wstring& text, uint32_t color = 0xFFFFFFFF);

    /// @brief 2D変換付きでテキストを描画（回転・拡縮・移動など）
    void DrawStringTransformed(int fontHandle, float x, float y,
                               const std::wstring& text, uint32_t color,
                               const Transform2D& transform);

    /// @brief printf形式のフォーマット文字列を描画する
    /// @param fontHandle フォントハンドル（FontManager::CreateFontで取得）
    /// @param x 描画開始X座標
    /// @param y 描画開始Y座標
    /// @param color 文字色（0xAARRGGBB形式）
    /// @param format printf形式のフォーマット文字列
    void DrawFormatString(int fontHandle, float x, float y, uint32_t color, const wchar_t* format, ...);

    /// @brief 文字列の描画幅を計算する
    /// @param fontHandle フォントハンドル（FontManager::CreateFontで取得）
    /// @param text 幅を計算する文字列
    /// @return 文字列の描画幅（ピクセル）
    int GetStringWidth(int fontHandle, const std::wstring& text);

private:
    SpriteBatch* m_spriteBatch = nullptr;
    FontManager* m_fontManager = nullptr;
};

} // namespace GX
