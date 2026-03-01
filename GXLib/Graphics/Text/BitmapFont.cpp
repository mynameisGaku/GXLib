/// @file BitmapFont.cpp
/// @brief ビットマップフォントの実装
#include "pch_graphics.h"
#include "Graphics/Text/BitmapFont.h"
#include <fstream>
#include <sstream>

namespace gx
{

/// @brief BMFont形式テキスト行からトークンを分割するヘルパー
static std::vector<std::string> Tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

/// @brief トークン列から指定キーの値を取得する
static int FindTokenInt(const std::vector<std::string>& tokens, const std::string& key)
{
    for (const auto& t : tokens)
    {
        if (t.size() > key.size() && t.compare(0, key.size(), key) == 0 && t[key.size()] == '=')
        {
            return std::atoi(t.c_str() + key.size() + 1);
        }
    }
    return 0;
}

void BitmapFont::BuildFromGrid(int textureHandle, int texWidth, int texHeight,
                                int charWidth, int charHeight, int charsPerRow, int startChar)
{
    m_textureHandle = textureHandle;
    m_texWidth = texWidth;
    m_texHeight = texHeight;
    m_lineHeight = charHeight;
    m_glyphs.clear();

    if (m_texWidth <= 0 || m_texHeight <= 0 || charWidth <= 0 || charHeight <= 0 || charsPerRow <= 0)
        return;

    float invW = 1.0f / static_cast<float>(m_texWidth);
    float invH = 1.0f / static_cast<float>(m_texHeight);

    // Generate glyphs for ASCII range startChar through 126
    for (int charCode = startChar; charCode <= 126; ++charCode)
    {
        int index = charCode - startChar;
        int col = index % charsPerRow;
        int row = index / charsPerRow;

        int pixelX = col * charWidth;
        int pixelY = row * charHeight;

        // Bounds check: skip if character would be outside the texture
        if (pixelX + charWidth > m_texWidth || pixelY + charHeight > m_texHeight)
            break;

        BitmapGlyph glyph;
        glyph.charCode = static_cast<uint32_t>(charCode);
        glyph.width    = charWidth;
        glyph.height   = charHeight;
        glyph.xOffset  = 0;
        glyph.yOffset  = 0;
        glyph.xAdvance = charWidth;

        glyph.u0 = static_cast<float>(pixelX) * invW;
        glyph.v0 = static_cast<float>(pixelY) * invH;
        glyph.u1 = static_cast<float>(pixelX + charWidth) * invW;
        glyph.v1 = static_cast<float>(pixelY + charHeight) * invH;

        m_glyphs[glyph.charCode] = glyph;
    }
}

bool BitmapFont::LoadFromFnt(const std::string& fntPath, int textureHandle, int texWidth, int texHeight)
{
    std::ifstream file(fntPath);
    if (!file.is_open())
        return false;

    m_textureHandle = textureHandle;
    m_texWidth = texWidth;
    m_texHeight = texHeight;
    m_glyphs.clear();

    if (m_texWidth <= 0 || m_texHeight <= 0)
        return false;

    float invW = 1.0f / static_cast<float>(m_texWidth);
    float invH = 1.0f / static_cast<float>(m_texHeight);

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        auto tokens = Tokenize(line);
        if (tokens.empty())
            continue;

        const std::string& tag = tokens[0];

        if (tag == "common")
        {
            // common lineHeight=16 base=13 ...
            m_lineHeight = FindTokenInt(tokens, "lineHeight");
            if (m_lineHeight <= 0)
                m_lineHeight = 16;
        }
        else if (tag == "char")
        {
            // char id=65 x=10 y=20 width=8 height=12 xoffset=0 yoffset=1 xadvance=9 page=0
            BitmapGlyph glyph;
            glyph.charCode = static_cast<uint32_t>(FindTokenInt(tokens, "id"));

            int px = FindTokenInt(tokens, "x");
            int py = FindTokenInt(tokens, "y");
            glyph.width    = FindTokenInt(tokens, "width");
            glyph.height   = FindTokenInt(tokens, "height");
            glyph.xOffset  = FindTokenInt(tokens, "xoffset");
            glyph.yOffset  = FindTokenInt(tokens, "yoffset");
            glyph.xAdvance = FindTokenInt(tokens, "xadvance");

            glyph.u0 = static_cast<float>(px) * invW;
            glyph.v0 = static_cast<float>(py) * invH;
            glyph.u1 = static_cast<float>(px + glyph.width) * invW;
            glyph.v1 = static_cast<float>(py + glyph.height) * invH;

            m_glyphs[glyph.charCode] = glyph;
        }
        // Skip other tags (info, page, chars, kernings)
    }

    return !m_glyphs.empty();
}

float BitmapFont::MeasureWidth(const std::wstring& text) const
{
    if (text.empty())
        return 0.0f;

    float totalWidth = 0.0f;

    for (size_t i = 0; i < text.size(); ++i)
    {
        uint32_t charCode = static_cast<uint32_t>(text[i]);
        const BitmapGlyph* glyph = GetGlyph(charCode);
        if (!glyph)
            continue;

        totalWidth += static_cast<float>(glyph->xAdvance);

        // Add letter spacing (except after the last character)
        if (i + 1 < text.size())
        {
            totalWidth += static_cast<float>(m_letterSpacing);
        }
    }

    return totalWidth;
}

const BitmapGlyph* BitmapFont::GetGlyph(uint32_t charCode) const
{
    auto it = m_glyphs.find(charCode);
    if (it != m_glyphs.end())
        return &it->second;
    return nullptr;
}

} // namespace gx
