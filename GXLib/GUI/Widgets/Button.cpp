#include "pch.h"
#include "GUI/Widgets/Button.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

float Button::GetIntrinsicWidth() const
{
    if (m_text.empty() || m_fontHandle < 0 || !m_renderer) return 100.0f;
    return static_cast<float>(m_renderer->GetTextWidth(m_fontHandle, m_text))
           + computedStyle.padding.HorizontalTotal();
}

float Button::GetIntrinsicHeight() const
{
    if (m_fontHandle < 0 || !m_renderer) return 40.0f;
    return static_cast<float>(m_renderer->GetLineHeight(m_fontHandle))
           + computedStyle.padding.VerticalTotal();
}

bool Button::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);
    return false;
}

void Button::Render(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();

    // 背景を描画
    renderer.DrawRect(globalRect, drawStyle, opacity);

    // テキスト描画
    if (!m_text.empty() && m_fontHandle >= 0)
    {
        float textW = static_cast<float>(renderer.GetTextWidth(m_fontHandle, m_text));
        float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));

        float textX = globalRect.x + (globalRect.width - textW) * 0.5f;
        float textY = globalRect.y + (globalRect.height - textH) * 0.5f;

        renderer.DrawText(textX, textY, m_fontHandle, m_text, drawStyle.color, opacity);
    }

    // ボタンは子を持たないのが一般的
}

}} // namespace GX::GUI
