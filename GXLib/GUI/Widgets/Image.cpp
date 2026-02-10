#include "pch.h"
#include "GUI/Widgets/Image.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Image::Render(UIRenderer& renderer)
{
    if (m_textureHandle < 0) return;
    const Style& drawStyle = GetRenderStyle();

    // 背景
    if (!drawStyle.backgroundColor.IsTransparent())
        renderer.DrawRect(globalRect, drawStyle, opacity);

    float rectW = globalRect.width;
    float rectH = globalRect.height;
    float natW = m_naturalWidth > 0 ? m_naturalWidth : rectW;
    float natH = m_naturalHeight > 0 ? m_naturalHeight : rectH;

    float drawX = globalRect.x;
    float drawY = globalRect.y;
    float drawW = rectW;
    float drawH = rectH;

    if (m_fit == ImageFit::Contain && natW > 0 && natH > 0)
    {
        float scale = (std::min)(rectW / natW, rectH / natH);
        drawW = natW * scale;
        drawH = natH * scale;
        drawX += (rectW - drawW) * 0.5f;
        drawY += (rectH - drawH) * 0.5f;
    }
    else if (m_fit == ImageFit::Cover && natW > 0 && natH > 0)
    {
        float scale = (std::max)(rectW / natW, rectH / natH);
        drawW = natW * scale;
        drawH = natH * scale;
        drawX += (rectW - drawW) * 0.5f;
        drawY += (rectH - drawH) * 0.5f;

        // クリップ
        renderer.PushScissor(globalRect);
        renderer.DrawImage(drawX, drawY, drawW, drawH, m_textureHandle, opacity);
        renderer.PopScissor();
        return;
    }

    renderer.DrawImage(drawX, drawY, drawW, drawH, m_textureHandle, opacity);
}

}} // namespace GX::GUI
