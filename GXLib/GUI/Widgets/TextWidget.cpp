#include "pch.h"
#include "GUI/Widgets/TextWidget.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

float TextWidget::GetIntrinsicWidth() const
{
    if (m_text.empty() || m_fontHandle < 0 || !m_renderer) return 0.0f;
    return static_cast<float>(m_renderer->GetTextWidth(m_fontHandle, m_text));
}

float TextWidget::GetIntrinsicHeight() const
{
    if (m_fontHandle < 0 || !m_renderer) return computedStyle.fontSize;
    return static_cast<float>(m_renderer->GetLineHeight(m_fontHandle));
}

void TextWidget::Render(UIRenderer& renderer)
{
    if (m_text.empty() || m_fontHandle < 0) return;

    // 背景（設定されている場合）
    if (!computedStyle.backgroundColor.IsTransparent())
    {
        renderer.DrawRect(globalRect, computedStyle, opacity);
    }

    // テキスト位置計算（textAlign）
    float textX = globalRect.x + computedStyle.padding.left;
    float textY = globalRect.y + computedStyle.padding.top;

    float contentW = globalRect.width - computedStyle.padding.HorizontalTotal();
    float textW = static_cast<float>(renderer.GetTextWidth(m_fontHandle, m_text));
    float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));

    switch (computedStyle.textAlign)
    {
    case TextAlign::Center:
        textX += (contentW - textW) * 0.5f;
        break;
    case TextAlign::Right:
        textX += contentW - textW;
        break;
    default:
        break;
    }

    // verticalAlign
    float contentH = globalRect.height - computedStyle.padding.VerticalTotal();
    switch (computedStyle.verticalAlign)
    {
    case VAlign::Center:
        textY += (contentH - textH) * 0.5f;
        break;
    case VAlign::Bottom:
        textY += contentH - textH;
        break;
    default:
        break;
    }

    renderer.DrawText(textX, textY, m_fontHandle, m_text, computedStyle.color, opacity);

    // 子は通常描画しない（TextWidgetは末端）
}

}} // namespace GX::GUI
