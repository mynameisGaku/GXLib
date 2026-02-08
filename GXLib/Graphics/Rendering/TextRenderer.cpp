/// @file TextRenderer.cpp
/// @brief テキストレンダラーの実装
#include "pch.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include <cstdarg>

namespace GX
{

void TextRenderer::Initialize(SpriteBatch* spriteBatch, FontManager* fontManager)
{
    m_spriteBatch = spriteBatch;
    m_fontManager = fontManager;
}

void TextRenderer::DrawString(int fontHandle, float x, float y, const std::wstring& text, uint32_t color)
{
    if (!m_spriteBatch || !m_fontManager)
        return;

    int atlasHandle = m_fontManager->GetAtlasTextureHandle(fontHandle);
    if (atlasHandle < 0)
        return;

    // 色をfloat4に変換してSpriteBatchに設定
    float a = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    float r = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>((color) & 0xFF) / 255.0f;

    // 描画色を保存・設定
    m_spriteBatch->SetDrawColor(r, g, b, a);

    float cursorX = x;
    float cursorY = y;

    for (wchar_t ch : text)
    {
        // 改行処理
        if (ch == L'\n')
        {
            cursorX = x;
            cursorY += static_cast<float>(m_fontManager->GetLineHeight(fontHandle));
            continue;
        }

        const GlyphInfo* glyph = m_fontManager->GetGlyphInfo(fontHandle, ch);
        if (!glyph)
            continue;

        // スペースはadvanceだけ進める
        if (ch == L' ')
        {
            cursorX += glyph->advance;
            continue;
        }

        // グリフを描画
        float drawX = cursorX + glyph->offsetX;
        float drawY = cursorY + glyph->offsetY;

        // SpriteBatch::DrawRectGraphでアトラスの一部を描画
        // srcX, srcY, w, h はアトラス上のピクセル座標
        int srcX = static_cast<int>(glyph->u0 * FontManager::k_AtlasSize);
        int srcY = static_cast<int>(glyph->v0 * FontManager::k_AtlasSize);

        m_spriteBatch->DrawRectGraph(drawX, drawY, srcX, srcY,
                                     glyph->width, glyph->height,
                                     atlasHandle, true);

        cursorX += glyph->advance;
    }

    // 描画色をリセット
    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void TextRenderer::DrawFormatString(int fontHandle, float x, float y, uint32_t color, const wchar_t* format, ...)
{
    wchar_t buffer[1024];

    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, format, args);
    va_end(args);

    DrawString(fontHandle, x, y, buffer, color);
}

int TextRenderer::GetStringWidth(int fontHandle, const std::wstring& text)
{
    if (!m_fontManager)
        return 0;

    float width = 0.0f;

    for (wchar_t ch : text)
    {
        if (ch == L'\n')
            break;  // 1行目の幅のみ

        const GlyphInfo* glyph = m_fontManager->GetGlyphInfo(fontHandle, ch);
        if (glyph)
        {
            width += glyph->advance;
        }
    }

    return static_cast<int>(ceilf(width));
}

} // namespace GX
