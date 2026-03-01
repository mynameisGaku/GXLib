#pragma once
/// @file BitmapFont.h
/// @brief ビットマップフォント（固定幅/可変幅テクスチャフォント）
///
/// 等幅グリッドまたはBMFont形式のテクスチャフォントを管理する。
/// SDFとは異なり、ピクセルパーフェクトなドット絵フォントに適する。
/// @addtogroup grp_gfx_text/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief ビットマップフォントのグリフ
struct BitmapGlyph
{
    uint32_t charCode = 0;       ///< 文字コード (Unicode)
    float u0 = 0, v0 = 0;       ///< テクスチャUV左上
    float u1 = 0, v1 = 0;       ///< テクスチャUV右下
    int width = 0, height = 0;  ///< グリフサイズ（ピクセル）
    int xOffset = 0, yOffset = 0; ///< 描画時オフセット（ピクセル）
    int xAdvance = 0;            ///< 次の文字へのXアドバンス（ピクセル）
};

/// @brief ビットマップフォントクラス
class BitmapFont
{
public:
    BitmapFont() = default;
    ~BitmapFont() = default;

    /// @brief 等幅グリッドから構築 (ASCII 32-126)
    /// @param textureHandle テクスチャハンドル
    /// @param texWidth テクスチャ幅
    /// @param texHeight テクスチャ高さ
    /// @param charWidth 1文字の幅
    /// @param charHeight 1文字の高さ
    /// @param charsPerRow 1行あたりの文字数
    /// @param startChar 最初の文字コード(通常32=スペース)
    void BuildFromGrid(int textureHandle, int texWidth, int texHeight,
                       int charWidth, int charHeight, int charsPerRow = 16, int startChar = 32);

    /// @brief BMFont形式(.fnt)から読み込み
    bool LoadFromFnt(const std::string& fntPath, int textureHandle, int texWidth, int texHeight);

    /// @brief テキスト幅を計算
    float MeasureWidth(const std::wstring& text) const;

    /// @brief 行の高さ
    int GetLineHeight() const { return m_lineHeight; }

    /// @brief グリフ情報取得
    const BitmapGlyph* GetGlyph(uint32_t charCode) const;

    /// @brief テクスチャハンドルを取得
    /// @return テクスチャハンドル (-1で未設定)
    int GetTextureHandle() const { return m_textureHandle; }
    /// @brief 登録済みグリフ数を取得
    /// @return グリフ数
    uint32_t GetGlyphCount() const { return static_cast<uint32_t>(m_glyphs.size()); }

    /// @brief 文字間スペーシング
    void SetLetterSpacing(int spacing) { m_letterSpacing = spacing; }
    int GetLetterSpacing() const { return m_letterSpacing; }

private:
    int m_textureHandle = -1;                          ///< テクスチャハンドル
    int m_texWidth = 0, m_texHeight = 0;               ///< テクスチャサイズ（ピクセル）
    int m_lineHeight = 16;                             ///< 行の高さ（ピクセル）
    int m_letterSpacing = 0;                           ///< 文字間スペーシング（ピクセル）
    std::unordered_map<uint32_t, BitmapGlyph> m_glyphs; ///< 文字コード→グリフ情報マップ
};

} // namespace gx
/// @}
