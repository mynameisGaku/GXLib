#include "pch.h"
#include "GUI/Widgets/RadioButton.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void RadioButton::SetSelected(bool selected)
{
    if (m_selected == selected) return;
    m_selected = selected;

    if (m_selected)
    {
        DeselectSiblings();

        // 親の onValueChanged を呼ぶ
        Widget* parent = GetParent();
        if (parent && parent->onValueChanged)
            parent->onValueChanged(m_value);
    }
}

void RadioButton::DeselectSiblings()
{
    Widget* parent = GetParent();
    if (!parent) return;

    for (auto& sibling : parent->GetChildren())
    {
        if (sibling.get() == this) continue;
        if (sibling->GetType() == WidgetType::RadioButton)
        {
            auto* rb = static_cast<RadioButton*>(sibling.get());
            rb->m_selected = false;
        }
    }
}

float RadioButton::GetIntrinsicWidth() const
{
    float textW = 0.0f;
    if (!m_text.empty() && m_fontHandle >= 0 && m_renderer)
        textW = static_cast<float>(m_renderer->GetTextWidth(m_fontHandle, m_text));

    return k_CircleSize + (textW > 0 ? k_Gap + textW : 0.0f);
}

float RadioButton::GetIntrinsicHeight() const
{
    float lineH = k_CircleSize;
    if (m_fontHandle >= 0 && m_renderer)
        lineH = (std::max)(k_CircleSize, static_cast<float>(m_renderer->GetLineHeight(m_fontHandle)));
    return lineH;
}

bool RadioButton::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (event.type == UIEventType::Click && enabled)
    {
        SetSelected(true);
        return true;
    }
    return false;
}

void RadioButton::Render(UIRenderer& renderer)
{
    float circleX = globalRect.x;
    float circleY = globalRect.y + (globalRect.height - k_CircleSize) * 0.5f;

    // 外側の円（角丸=半径でフル円に）
    LayoutRect circleRect = { circleX, circleY, k_CircleSize, k_CircleSize };
    Style circleStyle;
    circleStyle.backgroundColor = { 0.15f, 0.15f, 0.2f, 1.0f };
    circleStyle.borderWidth = 1.5f;
    circleStyle.borderColor = hovered
        ? StyleColor(0.5f, 0.6f, 0.9f, 1.0f)
        : StyleColor(0.4f, 0.4f, 0.55f, 1.0f);
    circleStyle.cornerRadius = k_CircleSize * 0.5f;
    renderer.DrawRect(circleRect, circleStyle, opacity);

    // 選択時の内側ドット
    if (m_selected)
    {
        float dotSize = 8.0f;
        float dotX = circleX + (k_CircleSize - dotSize) * 0.5f;
        float dotY = circleY + (k_CircleSize - dotSize) * 0.5f;
        LayoutRect dotRect = { dotX, dotY, dotSize, dotSize };
        Style dotStyle;
        dotStyle.backgroundColor = { 0.3f, 0.6f, 1.0f, 1.0f };
        dotStyle.cornerRadius = dotSize * 0.5f;
        renderer.DrawRect(dotRect, dotStyle, opacity);
    }

    // ラベルテキスト
    if (!m_text.empty() && m_fontHandle >= 0)
    {
        float textX = circleX + k_CircleSize + k_Gap;
        float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));
        float textY = globalRect.y + (globalRect.height - textH) * 0.5f;
        const Style& drawStyle = GetRenderStyle();
        renderer.DrawText(textX, textY, m_fontHandle, m_text, drawStyle.color, opacity);
    }
}

}} // namespace GX::GUI
