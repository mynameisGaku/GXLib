#pragma once
/// @file Image.h
/// @brief 画像表示ウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief 画像フィットモード
enum class ImageFit { Stretch, Contain, Cover };

/// @brief 画像表示ウィジェット
class Image : public Widget
{
public:
    Image() = default;
    ~Image() override = default;

    WidgetType GetType() const override { return WidgetType::Image; }

    void SetTextureHandle(int handle) { m_textureHandle = handle; }
    int GetTextureHandle() const { return m_textureHandle; }

    void SetFit(ImageFit fit) { m_fit = fit; }
    ImageFit GetFit() const { return m_fit; }

    void SetNaturalSize(float w, float h) { m_naturalWidth = w; m_naturalHeight = h; }
    float GetNaturalWidth() const { return m_naturalWidth; }
    float GetNaturalHeight() const { return m_naturalHeight; }

    float GetIntrinsicWidth() const override { return m_naturalWidth > 0 ? m_naturalWidth : 64.0f; }
    float GetIntrinsicHeight() const override { return m_naturalHeight > 0 ? m_naturalHeight : 64.0f; }

    void Render(UIRenderer& renderer) override;

private:
    int m_textureHandle = -1;
    ImageFit m_fit = ImageFit::Stretch;
    float m_naturalWidth = 0.0f;
    float m_naturalHeight = 0.0f;
};

}} // namespace GX::GUI
