#pragma once
/// @file TextRenderer.h
/// @brief テキスト描画（SpriteBatch + FontManager 連携）
///
/// DxLibの DrawStringToHandle / DrawFormatStringToHandle に相当するテキスト描画を提供する。
/// FontManager のアトラステクスチャと SpriteBatch の矩形描画を組み合わせ、
/// 1文字ずつグリフをアトラスから切り出して並べる仕組み。

#include "pch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Math/Transform2D.h"

namespace GX
{

class SpriteBatch;

/// @brief SpriteBatchベースのテキスト描画クラス（DxLibの DrawStringToHandle に相当）
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

    /// テキストの水平揃え
    enum class TextAlign { Left, Center, Right };

    /// テキストレイアウトオプション
    struct TextLayoutOptions
    {
        float maxWidth = 0.0f;       ///< 最大幅（0=無制限）
        float lineSpacing = 1.2f;    ///< 行間倍率（1.0=フォントサイズ分）
        TextAlign align = TextAlign::Left;
        bool wordWrap = true;        ///< 自動改行
    };

    /// レイアウト付きテキスト描画（自動改行・揃え対応）
    void DrawStringLayout(int fontHandle, float x, float y,
                           const std::wstring& text, uint32_t color,
                           const TextLayoutOptions& options);

    /// テキストの描画高さを計算する（改行含む）
    int GetStringHeight(int fontHandle, const std::wstring& text,
                         const TextLayoutOptions& options);

    /// 矩形領域内にテキストを描画する
    void DrawStringInRect(int fontHandle, float x, float y, float width, float height,
                           const std::wstring& text, uint32_t color,
                           const TextLayoutOptions& options);

private:
    /// テキストを行に分割する（改行・ワードラップ対応）
    std::vector<std::wstring> BreakLines(int fontHandle, const std::wstring& text,
                                          const TextLayoutOptions& options);

    /// 単一行の描画幅を計算する（内部ヘルパー）
    float MeasureLineWidth(int fontHandle, const std::wstring& line);

    SpriteBatch* m_spriteBatch = nullptr;
    FontManager* m_fontManager = nullptr;
};

} // namespace GX
