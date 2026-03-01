/// @file SDFFont.cpp
/// @brief SDFフォントの実装
#include "pch_graphics.h"
#include "Graphics/Text/SDFFont.h"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace gx
{

/// @brief BMFont形式テキスト行から key=value ペアのintを読み取るヘルパー
static int ParseInt(const std::string& token)
{
    auto pos = token.find('=');
    if (pos == std::string::npos)
        return 0;
    return std::atoi(token.c_str() + pos + 1);
}

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

bool SDFFont::LoadFromFnt(const std::string& fntPath, int textureHandle, int texWidth, int texHeight)
{
    std::ifstream file(fntPath);
    if (!file.is_open())
        return false;

    m_textureHandle = textureHandle;
    m_texWidth = texWidth;
    m_texHeight = texHeight;
    m_glyphs.clear();
    m_kernings.clear();

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

        if (tag == "info")
        {
            // info face="FontName" size=32 ...
            m_baseSize = FindTokenInt(tokens, "size");
            if (m_baseSize <= 0)
                m_baseSize = 32;
        }
        else if (tag == "common")
        {
            // common lineHeight=40 base=32 scaleW=512 scaleH=512 ...
            m_lineHeight = FindTokenInt(tokens, "lineHeight");
            m_base = FindTokenInt(tokens, "base");
            if (m_lineHeight <= 0)
                m_lineHeight = m_baseSize;
        }
        else if (tag == "char")
        {
            // char id=65 x=10 y=20 width=18 height=22 xoffset=1 yoffset=5 xadvance=20 page=0
            SDFGlyph glyph;
            glyph.id       = static_cast<uint32_t>(FindTokenInt(tokens, "id"));
            glyph.x        = FindTokenInt(tokens, "x");
            glyph.y        = FindTokenInt(tokens, "y");
            glyph.width    = FindTokenInt(tokens, "width");
            glyph.height   = FindTokenInt(tokens, "height");
            glyph.xOffset  = FindTokenInt(tokens, "xoffset");
            glyph.yOffset  = FindTokenInt(tokens, "yoffset");
            glyph.xAdvance = FindTokenInt(tokens, "xadvance");
            glyph.page     = FindTokenInt(tokens, "page");

            // Calculate UV coordinates
            glyph.u0 = static_cast<float>(glyph.x) * invW;
            glyph.v0 = static_cast<float>(glyph.y) * invH;
            glyph.u1 = static_cast<float>(glyph.x + glyph.width) * invW;
            glyph.v1 = static_cast<float>(glyph.y + glyph.height) * invH;

            m_glyphs[glyph.id] = glyph;
        }
        else if (tag == "kerning")
        {
            // kerning first=65 second=66 amount=-2
            KerningPair kp;
            kp.first  = static_cast<uint32_t>(FindTokenInt(tokens, "first"));
            kp.second = static_cast<uint32_t>(FindTokenInt(tokens, "second"));
            kp.amount = FindTokenInt(tokens, "amount");
            m_kernings.push_back(kp);
        }
        // "page", "chars", "kernings" lines are informational and can be skipped
    }

    return !m_glyphs.empty();
}

float SDFFont::MeasureWidth(const std::wstring& text, float fontSize) const
{
    if (text.empty() || m_baseSize <= 0)
        return 0.0f;

    float scale = fontSize / static_cast<float>(m_baseSize);
    float totalWidth = 0.0f;

    for (size_t i = 0; i < text.size(); ++i)
    {
        uint32_t charCode = static_cast<uint32_t>(text[i]);
        const SDFGlyph* glyph = GetGlyph(charCode);
        if (!glyph)
            continue;

        totalWidth += static_cast<float>(glyph->xAdvance);

        // Add kerning with the next character
        if (i + 1 < text.size())
        {
            uint32_t nextChar = static_cast<uint32_t>(text[i + 1]);
            totalWidth += static_cast<float>(GetKerning(charCode, nextChar));
        }
    }

    return totalWidth * scale;
}

float SDFFont::GetLineHeight(float fontSize) const
{
    if (m_baseSize <= 0)
        return fontSize;
    return static_cast<float>(m_lineHeight) * (fontSize / static_cast<float>(m_baseSize));
}

const SDFGlyph* SDFFont::GetGlyph(uint32_t charCode) const
{
    auto it = m_glyphs.find(charCode);
    if (it != m_glyphs.end())
        return &it->second;
    return nullptr;
}

int SDFFont::GetKerning(uint32_t first, uint32_t second) const
{
    for (const auto& kp : m_kernings)
    {
        if (kp.first == first && kp.second == second)
            return kp.amount;
    }
    return 0;
}

} // namespace gx
