/// @file test_FontSystem.cpp
/// @brief SDFFontとBitmapFontのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/Text/SDFFont.h"
#include "Graphics/Text/BitmapFont.h"

using namespace gx;

// =========================================================================
// SDFFontテスト
// =========================================================================

TEST(SDFFont, DefaultState)
{
    SDFFont font;
    EXPECT_EQ(font.GetTextureHandle(), -1);
    EXPECT_EQ(font.GetGlyphCount(), 0u);
    EXPECT_EQ(font.GetBaseSize(), 32);
    EXPECT_EQ(font.GetLineHeightPx(), 32);
}

TEST(SDFFont, GetGlyph_UnknownReturnsNull)
{
    SDFFont font;
    const SDFGlyph* glyph = font.GetGlyph('A');
    EXPECT_EQ(glyph, nullptr);
}

TEST(SDFFont, SetEdgeThreshold_GetEdgeThreshold)
{
    SDFFont font;
    EXPECT_FLOAT_EQ(font.GetEdgeThreshold(), 0.5f); // デフォルト値
    font.SetEdgeThreshold(0.3f);
    EXPECT_FLOAT_EQ(font.GetEdgeThreshold(), 0.3f);
    font.SetEdgeThreshold(0.8f);
    EXPECT_FLOAT_EQ(font.GetEdgeThreshold(), 0.8f);
}

TEST(SDFFont, SetSmoothWidth_GetSmoothWidth)
{
    SDFFont font;
    EXPECT_FLOAT_EQ(font.GetSmoothWidth(), 0.1f); // デフォルト値
    font.SetSmoothWidth(0.05f);
    EXPECT_FLOAT_EQ(font.GetSmoothWidth(), 0.05f);
    font.SetSmoothWidth(0.2f);
    EXPECT_FLOAT_EQ(font.GetSmoothWidth(), 0.2f);
}

TEST(SDFFont, OutlineSettings)
{
    SDFFont font;
    EXPECT_FALSE(font.IsOutlineEnabled());

    font.SetOutlineEnabled(true);
    EXPECT_TRUE(font.IsOutlineEnabled());

    font.SetOutlineEnabled(false);
    EXPECT_FALSE(font.IsOutlineEnabled());
}

TEST(SDFFont, OutlineThickness)
{
    SDFFont font;
    EXPECT_FLOAT_EQ(font.GetOutlineThickness(), 0.1f); // デフォルト値
    font.SetOutlineThickness(0.25f);
    EXPECT_FLOAT_EQ(font.GetOutlineThickness(), 0.25f);
}

TEST(SDFFont, OutlineColor)
{
    SDFFont font;
    // クラッシュしないことを検証 -- 色は内部に保存される
    font.SetOutlineColor(1.0f, 0.0f, 0.0f, 1.0f);
    font.SetOutlineEnabled(true);
    EXPECT_TRUE(font.IsOutlineEnabled());
}

TEST(SDFFont, ShadowSettings)
{
    SDFFont font;
    EXPECT_FALSE(font.IsShadowEnabled());

    font.SetShadowEnabled(true);
    EXPECT_TRUE(font.IsShadowEnabled());

    font.SetShadowEnabled(false);
    EXPECT_FALSE(font.IsShadowEnabled());
}

TEST(SDFFont, ShadowOffset)
{
    SDFFont font;
    // 影オフセットの設定がクラッシュしないことを検証
    font.SetShadowOffset(3.0f, 3.0f);
    font.SetShadowEnabled(true);
    EXPECT_TRUE(font.IsShadowEnabled());
}

TEST(SDFFont, ShadowColor)
{
    SDFFont font;
    font.SetShadowColor(0.0f, 0.0f, 0.0f, 0.8f);
    font.SetShadowEnabled(true);
    EXPECT_TRUE(font.IsShadowEnabled());
}

TEST(SDFFont, GlyphCount_ZeroInitially)
{
    SDFFont font;
    EXPECT_EQ(font.GetGlyphCount(), 0u);
}

TEST(SDFFont, MeasureWidth_EmptyString)
{
    SDFFont font;
    float width = font.MeasureWidth(L"", 32.0f);
    EXPECT_FLOAT_EQ(width, 0.0f);
}

TEST(SDFFont, GetLineHeight)
{
    SDFFont font;
    // デフォルトbaseSize=32の場合、fontSize=32での行の高さはlineHeightピクセルになるはず
    float lh = font.GetLineHeight(32.0f);
    EXPECT_GT(lh, 0.0f);
}

TEST(SDFFont, GetKerning_NoPairs)
{
    SDFFont font;
    // カーニングペアが読み込まれていない場合は0を返すはず
    int kern = font.GetKerning('A', 'V');
    EXPECT_EQ(kern, 0);
}

// =========================================================================
// BitmapFontテスト
// =========================================================================

TEST(BitmapFont, DefaultState)
{
    BitmapFont font;
    EXPECT_EQ(font.GetTextureHandle(), -1);
    EXPECT_EQ(font.GetGlyphCount(), 0u);
    EXPECT_EQ(font.GetLineHeight(), 16);
    EXPECT_EQ(font.GetLetterSpacing(), 0);
}

TEST(BitmapFont, BuildFromGrid_GlyphCount)
{
    BitmapFont font;
    // 1行16文字、ASCII 32-126 = 95文字
    // テクスチャ: 256x96、文字16x16 => 16列 x 6行 = 96セル、ただしグリフは95個
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);
    EXPECT_EQ(font.GetGlyphCount(), 95u); // ASCII 32..126（両端を含む）
}

TEST(BitmapFont, BuildFromGrid_TextureHandle)
{
    BitmapFont font;
    font.BuildFromGrid(42, 256, 96, 16, 16, 16, 32);
    EXPECT_EQ(font.GetTextureHandle(), 42);
}

TEST(BitmapFont, GetGlyph_AfterBuildFromGrid)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);

    // 'A' = 文字コード65
    const BitmapGlyph* glyph = font.GetGlyph('A');
    ASSERT_NE(glyph, nullptr);
    EXPECT_EQ(glyph->charCode, static_cast<uint32_t>('A'));
    EXPECT_EQ(glyph->width, 16);
    EXPECT_EQ(glyph->height, 16);
}

TEST(BitmapFont, GetGlyph_SpaceCharacter)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);

    // スペース = 文字コード32（最初の文字）
    const BitmapGlyph* glyph = font.GetGlyph(' ');
    ASSERT_NE(glyph, nullptr);
    EXPECT_EQ(glyph->charCode, 32u);
}

TEST(BitmapFont, GetGlyph_TildeCharacter)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);

    // '~' = 文字コード126（最後の文字）
    const BitmapGlyph* glyph = font.GetGlyph('~');
    ASSERT_NE(glyph, nullptr);
    EXPECT_EQ(glyph->charCode, 126u);
}

TEST(BitmapFont, GetGlyph_UnknownCharReturnsNull)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);

    // 文字コード200はASCII範囲外で、グリッドに含まれない
    const BitmapGlyph* glyph = font.GetGlyph(200);
    EXPECT_EQ(glyph, nullptr);
}

TEST(BitmapFont, MeasureWidth_EmptyString)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);
    float width = font.MeasureWidth(L"");
    EXPECT_FLOAT_EQ(width, 0.0f);
}

TEST(BitmapFont, MeasureWidth_SingleCharacter)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);
    float width = font.MeasureWidth(L"A");
    // 幅は正の値であるべき（少なくとも'A'のxAdvance分）
    EXPECT_GT(width, 0.0f);
}

TEST(BitmapFont, MeasureWidth_MultipleCharacters)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);
    float widthOne = font.MeasureWidth(L"A");
    float widthTwo = font.MeasureWidth(L"AB");
    // 2文字は1文字より幅が広いはず
    EXPECT_GT(widthTwo, widthOne);
}

TEST(BitmapFont, SetLetterSpacing_GetLetterSpacing)
{
    BitmapFont font;
    EXPECT_EQ(font.GetLetterSpacing(), 0);

    font.SetLetterSpacing(2);
    EXPECT_EQ(font.GetLetterSpacing(), 2);

    font.SetLetterSpacing(-1);
    EXPECT_EQ(font.GetLetterSpacing(), -1);
}

TEST(BitmapFont, LetterSpacing_AffectsMeasureWidth)
{
    BitmapFont font;
    font.BuildFromGrid(1, 256, 96, 16, 16, 16, 32);

    float widthDefault = font.MeasureWidth(L"ABC");
    font.SetLetterSpacing(4);
    float widthSpaced = font.MeasureWidth(L"ABC");
    // 文字間隔を追加すると、テキストの幅が広くなるはず
    EXPECT_GT(widthSpaced, widthDefault);
}

TEST(BitmapFont, GetTextureHandle)
{
    BitmapFont font;
    EXPECT_EQ(font.GetTextureHandle(), -1);

    font.BuildFromGrid(99, 128, 128, 8, 8, 16, 32);
    EXPECT_EQ(font.GetTextureHandle(), 99);
}
