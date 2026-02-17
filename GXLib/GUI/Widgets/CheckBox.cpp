#include "pch.h"
#include "GUI/Widgets/CheckBox.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void CheckBox::SetChecked(bool checked)
{
    if (m_checked == checked) return;
    m_checked = checked;
    if (onValueChanged)
        onValueChanged(m_checked ? "true" : "false");
}

float CheckBox::GetIntrinsicWidth() const
{
    float textW = 0.0f;
    if (!m_text.empty() && m_fontHandle >= 0 && m_renderer)
        textW = static_cast<float>(m_renderer->GetTextWidth(m_fontHandle, m_text));

    return k_BoxSize + (textW > 0 ? k_Gap + textW : 0.0f);
}

float CheckBox::GetIntrinsicHeight() const
{
    float lineH = k_BoxSize;
    if (m_fontHandle >= 0 && m_renderer)
        lineH = (std::max)(k_BoxSize, static_cast<float>(m_renderer->GetLineHeight(m_fontHandle)));
    return lineH;
}

bool CheckBox::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (event.type == UIEventType::Click && enabled)
    {
        SetChecked(!m_checked);
        return true;
    }
    return false;
}

void CheckBox::RenderSelf(UIRenderer& renderer)
{
    float boxX = globalRect.x;
    float boxY = globalRect.y + (globalRect.height - k_BoxSize) * 0.5f;

    // チェックボックス枠
    LayoutRect boxRect = { boxX, boxY, k_BoxSize, k_BoxSize };
    Style boxStyle;
    boxStyle.backgroundColor = { 0.15f, 0.15f, 0.2f, 1.0f };
    boxStyle.borderWidth = 1.5f;
    boxStyle.borderColor = hovered
        ? StyleColor(0.5f, 0.6f, 0.9f, 1.0f)
        : StyleColor(0.4f, 0.4f, 0.55f, 1.0f);
    boxStyle.cornerRadius = 3.0f;
    renderer.DrawRect(boxRect, boxStyle, 1.0f);

    // チェックマーク（塗りつぶし矩形）
    if (m_checked)
    {
        float inset = 4.0f;
        StyleColor checkColor = { 0.3f, 0.6f, 1.0f, 1.0f };
        renderer.DrawSolidRect(boxX + inset, boxY + inset,
                               k_BoxSize - inset * 2, k_BoxSize - inset * 2, checkColor);
    }

    // ラベルテキスト
    if (!m_text.empty() && m_fontHandle >= 0)
    {
        float textX = boxX + k_BoxSize + k_Gap;
        float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));
        float textY = globalRect.y + (globalRect.height - textH) * 0.5f;
        const Style& drawStyle = GetRenderStyle();
        renderer.DrawText(textX, textY, m_fontHandle, m_text, drawStyle.color, 1.0f);
    }
}

}} // namespace GX::GUI
