/// @file SoftImage.cpp
/// @brief CPU側での画像ピクセル操作の実装
#include "pch.h"
#include "Graphics/Resource/SoftImage.h"
#include "Core/Logger.h"

// stb_imageの実装定義(STB_IMAGE_IMPLEMENTATION)はTexture.cppにあるため、ここではヘッダのみ
#include "ThirdParty/stb_image.h"

namespace GX
{

bool SoftImage::Create(uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_pixels.resize(width * height * 4, 0); // RGBA 4バイト/ピクセル
    return true;
}

bool SoftImage::LoadFromFile(const std::wstring& filePath)
{
    // stb_imageはchar*パスのみ対応
    int pathLen = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string pathUtf8(pathLen - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, pathUtf8.data(), pathLen, nullptr, nullptr);

    int width, height, channels;
    unsigned char* pixels = stbi_load(pathUtf8.c_str(), &width, &height, &channels, 4);
    if (!pixels)
    {
        GX_LOG_ERROR("SoftImage: Failed to load %s", pathUtf8.c_str());
        return false;
    }

    m_width  = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);
    m_pixels.assign(pixels, pixels + width * height * 4);

    stbi_image_free(pixels);
    return true;
}

uint32_t SoftImage::GetPixel(int x, int y) const
{
    if (x < 0 || x >= static_cast<int>(m_width) ||
        y < 0 || y >= static_cast<int>(m_height))
        return 0;

    // 内部はRGBA順だが、戻り値は0xAARRGGBB形式にパックする
    size_t offset = (y * m_width + x) * 4;
    uint8_t r = m_pixels[offset + 0];
    uint8_t g = m_pixels[offset + 1];
    uint8_t b = m_pixels[offset + 2];
    uint8_t a = m_pixels[offset + 3];
    return (a << 24) | (r << 16) | (g << 8) | b;
}

void SoftImage::DrawPixel(int x, int y, uint32_t color)
{
    if (x < 0 || x >= static_cast<int>(m_width) ||
        y < 0 || y >= static_cast<int>(m_height))
        return;

    // 0xAARRGGBB形式からRGBA各バイトに分解して格納
    size_t offset = (y * m_width + x) * 4;
    m_pixels[offset + 0] = (color >> 16) & 0xFF; // R
    m_pixels[offset + 1] = (color >> 8)  & 0xFF; // G
    m_pixels[offset + 2] = (color >> 0)  & 0xFF; // B
    m_pixels[offset + 3] = (color >> 24) & 0xFF; // A
}

int SoftImage::CreateTexture(TextureManager& textureManager)
{
    if (m_pixels.empty() || m_width == 0 || m_height == 0)
        return -1;

    return textureManager.CreateTextureFromMemory(m_pixels.data(), m_width, m_height);
}

void SoftImage::Clear(uint32_t color)
{
    // 0xAARRGGBB形式を分解してRGBA順で全ピクセルに書き込む
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b = (color >> 0)  & 0xFF;
    uint8_t a = (color >> 24) & 0xFF;

    for (size_t i = 0; i < m_pixels.size(); i += 4)
    {
        m_pixels[i + 0] = r;
        m_pixels[i + 1] = g;
        m_pixels[i + 2] = b;
        m_pixels[i + 3] = a;
    }
}

} // namespace GX
