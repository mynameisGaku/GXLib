#include "pch.h"
#include "GUI/Widgets/Image.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Image::Update(float deltaTime)
{
    Widget::Update(deltaTime);
    const Style& style = GetRenderStyle();
    if (style.imageUVSpeedX != 0.0f || style.imageUVSpeedY != 0.0f)
    {
        m_uvOffsetX += style.imageUVSpeedX * deltaTime;
        m_uvOffsetY += style.imageUVSpeedY * deltaTime;

        m_uvOffsetX = std::fmod(m_uvOffsetX, 1.0f);
        m_uvOffsetY = std::fmod(m_uvOffsetY, 1.0f);
    }
}

void Image::RenderSelf(UIRenderer& renderer)
{
    if (m_textureHandle < 0) return;
    const Style& drawStyle = GetRenderStyle();

    // 背景
    if (!drawStyle.backgroundColor.IsTransparent())
    {
        UIRectEffect effect;
        const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
        renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);
    }

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
        float u0 = m_uvOffsetX;
        float v0 = m_uvOffsetY;
        float u1 = u0 + drawStyle.imageUVScaleX;
        float v1 = v0 + drawStyle.imageUVScaleY;
        renderer.PushScissor(globalRect);
        renderer.DrawImageUV(drawX, drawY, drawW, drawH, m_textureHandle, u0, v0, u1, v1, 1.0f);
        renderer.PopScissor();
        return;
    }
    {
        float u0 = m_uvOffsetX;
        float v0 = m_uvOffsetY;
        float u1 = u0 + drawStyle.imageUVScaleX;
        float v1 = v0 + drawStyle.imageUVScaleY;
        renderer.DrawImageUV(drawX, drawY, drawW, drawH, m_textureHandle, u0, v0, u1, v1, 1.0f);
    }
}

}} // namespace GX::GUI
