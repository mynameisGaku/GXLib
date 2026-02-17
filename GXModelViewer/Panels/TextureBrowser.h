#pragma once
/// @file TextureBrowser.h
/// @brief Texture browser panel with grid thumbnails

#include "Graphics/Resource/TextureManager.h"

/// @brief ImGui panel for browsing and inspecting loaded textures
class TextureBrowser
{
public:
    /// Draw the texture browser panel.
    /// @param texManager The texture manager to browse.
    void Draw(GX::TextureManager& texManager);

private:
    int m_selectedHandle = -1;      ///< Currently selected texture handle
    float m_thumbnailSize = 64.0f;  ///< Thumbnail display size
};
