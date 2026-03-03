#pragma once
/// @file SDFFont.h
/// @brief SDF (Signed Distance Field) フォントレンダリング
///
/// 距離場ベースのフォント描画により、拡大・縮小してもエッジが滑らかに保たれる。
/// BMFont形式のfntファイル + SDF生成済みテクスチャを読み込む。
/// @addtogroup grp_gfx_text/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief SDFフォントのグリフ情報
struct SDFGlyph
{
    uint32_t id = 0;         ///< 文字コード (Unicode)
    float u0 = 0, v0 = 0;   ///< テクスチャUV左上
    float u1 = 0, v1 = 0;   ///< テクスチャUV右下
    int   x = 0, y = 0;     ///< テクスチャ上のピクセル座標
    int   width = 0;         ///< グリフ幅(ピクセル)
    int   height = 0;        ///< グリフ高(ピクセル)
    int   xOffset = 0;       ///< 描画時Xオフセット
    int   yOffset = 0;       ///< 描画時Yオフセット
    int   xAdvance = 0;      ///< 次の文字へのXアドバンス
    int   page = 0;          ///< テクスチャページ番号
};

/// @brief カーニングペア
struct KerningPair
{
    uint32_t first = 0;   ///< 1文字目の文字コード
    uint32_t second = 0;  ///< 2文字目の文字コード
    int amount = 0;       ///< カーニング量（ピクセル、負で詰め）
};

/// @brief SDFフォントクラス
class SDFFont
{
public:
    SDFFont() = default;
    ~SDFFont() = default;

    /// @brief BMFont形式の.fntファイルからフォント情報を読み込む
    /// @param fntPath .fntファイルパス
    /// @param textureHandle SDF生成済みテクスチャのハンドル
    /// @param texWidth テクスチャ幅
    /// @param texHeight テクスチャ高さ
    /// @return 成功時true
    bool LoadFromFnt(const gx::String& fntPath, int textureHandle, int texWidth, int texHeight);

    /// @brief テキストの描画幅を計算する
    float MeasureWidth(const gx::WString& text, float fontSize) const;

    /// @brief テキストの描画高さを返す
    float GetLineHeight(float fontSize) const;

    /// @brief グリフ情報を取得する
    const SDFGlyph* GetGlyph(uint32_t charCode) const;

    /// @brief カーニング量を取得する
    int GetKerning(uint32_t first, uint32_t second) const;

    /// @brief テクスチャハンドルを取得
    int GetTextureHandle() const { return m_textureHandle; }
    /// @brief フォント基本サイズ（ピクセル）を取得
    int GetBaseSize() const { return m_baseSize; }
    /// @brief 行の高さ（ピクセル）を取得
    int GetLineHeightPx() const { return m_lineHeight; }
    /// @brief ベースライン位置（ピクセル）を取得
    int GetBase() const { return m_base; }
    /// @brief 登録済みグリフ数を取得
    uint32_t GetGlyphCount() const { return static_cast<uint32_t>(m_glyphs.size()); }

    /// @brief SDF描画パラメータ
    float GetEdgeThreshold() const { return m_edgeThreshold; }
    void SetEdgeThreshold(float v) { m_edgeThreshold = v; }
    float GetSmoothWidth() const { return m_smoothWidth; }
    void SetSmoothWidth(float v) { m_smoothWidth = v; }

    /// @brief アウトライン設定
    void SetOutlineEnabled(bool enabled) { m_outlineEnabled = enabled; }
    bool IsOutlineEnabled() const { return m_outlineEnabled; }
    void SetOutlineColor(float r, float g, float b, float a) { m_outlineR = r; m_outlineG = g; m_outlineB = b; m_outlineA = a; }
    void SetOutlineThickness(float t) { m_outlineThickness = t; }
    float GetOutlineThickness() const { return m_outlineThickness; }

    /// @brief シャドウ設定
    void SetShadowEnabled(bool enabled) { m_shadowEnabled = enabled; }
    bool IsShadowEnabled() const { return m_shadowEnabled; }
    void SetShadowOffset(float x, float y) { m_shadowOffsetX = x; m_shadowOffsetY = y; }
    void SetShadowColor(float r, float g, float b, float a) { m_shadowR = r; m_shadowG = g; m_shadowB = b; m_shadowA = a; }

private:
    int m_textureHandle = -1;                          ///< SDFテクスチャハンドル
    int m_texWidth = 0, m_texHeight = 0;               ///< テクスチャサイズ（ピクセル）
    int m_baseSize = 32;                               ///< フォント基本サイズ（ピクセル）
    int m_lineHeight = 32;                             ///< 行の高さ（ピクセル）
    int m_base = 26;                                   ///< ベースライン位置（ピクセル）

    float m_edgeThreshold = 0.5f;                      ///< SDF エッジ閾値（0.5=中心）
    float m_smoothWidth = 0.1f;                        ///< SDF スムージング幅

    bool m_outlineEnabled = false;                     ///< アウトライン有効フラグ
    float m_outlineR = 0, m_outlineG = 0, m_outlineB = 0, m_outlineA = 1; ///< アウトライン色（RGBA）
    float m_outlineThickness = 0.1f;                   ///< アウトライン太さ

    bool m_shadowEnabled = false;                      ///< シャドウ有効フラグ
    float m_shadowOffsetX = 2.0f, m_shadowOffsetY = 2.0f; ///< シャドウオフセット（ピクセル）
    float m_shadowR = 0, m_shadowG = 0, m_shadowB = 0, m_shadowA = 0.5f; ///< シャドウ色（RGBA）

    gx::HashMap<uint32_t, SDFGlyph> m_glyphs;   ///< 文字コード→グリフ情報マップ
    gx::Vector<KerningPair> m_kernings;               ///< カーニングペア配列
};

} // namespace gx
/// @}
