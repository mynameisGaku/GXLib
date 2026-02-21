#include "pch.h"
#include "GUI/Widgets/Slider.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Slider::SetValue(float v)
{
    // ステップにスナップ（step=0.1 なら 0.0, 0.1, 0.2 ... に丸まる）
    if (m_step > 0.0f)
        v = std::round(v / m_step) * m_step;

    v = (std::max)(m_min, (std::min)(v, m_max));

    if (v == m_value) return;
    m_value = v;

    if (onValueChanged)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.4f", m_value);
        onValueChanged(buf);
    }
}

float Slider::ScreenXToValue(float screenX) const
{
    float trackLeft = globalRect.x + k_ThumbSize * 0.5f;
    float trackRight = globalRect.x + globalRect.width - k_ThumbSize * 0.5f;
    float trackW = trackRight - trackLeft;
    if (trackW <= 0.0f) return m_min;

    float t = (screenX - trackLeft) / trackW;
    t = (std::max)(0.0f, (std::min)(t, 1.0f));
    return m_min + t * (m_max - m_min);
}

bool Slider::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (!enabled) return false;

    if (event.type == UIEventType::MouseDown)
    {
        m_dragging = true;
        SetValue(ScreenXToValue(event.localX));
        return true;
    }
    else if (event.type == UIEventType::MouseMove && m_dragging)
    {
        SetValue(ScreenXToValue(event.localX));
        return true;
    }
    else if (event.type == UIEventType::MouseUp)
    {
        if (m_dragging)
        {
            m_dragging = false;
            return true;
        }
    }
    return false;
}

void Slider::RenderSelf(UIRenderer& renderer)
{
    float range = m_max - m_min;
    float t = (range > 0.0f) ? (m_value - m_min) / range : 0.0f;

    float trackLeft = globalRect.x + k_ThumbSize * 0.5f;
    float trackRight = globalRect.x + globalRect.width - k_ThumbSize * 0.5f;
    float trackW = trackRight - trackLeft;
    float trackY = globalRect.y + (globalRect.height - k_TrackHeight) * 0.5f;

    // トラック背景
    StyleColor trackBg = { 0.25f, 0.25f, 0.3f, 1.0f };
    renderer.DrawSolidRect(trackLeft, trackY, trackW, k_TrackHeight, trackBg);

    // フィル
    float fillW = trackW * t;
    if (fillW > 0.0f)
    {
        StyleColor fillColor = { 0.3f, 0.6f, 1.0f, 1.0f };
        renderer.DrawSolidRect(trackLeft, trackY, fillW, k_TrackHeight, fillColor);
    }

    // つまみ（SDF角丸矩形）
    float thumbX = trackLeft + trackW * t - k_ThumbSize * 0.5f;
    float thumbY = globalRect.y + (globalRect.height - k_ThumbSize) * 0.5f;

    LayoutRect thumbRect = { thumbX, thumbY, k_ThumbSize, k_ThumbSize };
    Style thumbStyle;
    thumbStyle.backgroundColor = m_dragging
        ? StyleColor(0.5f, 0.75f, 1.0f, 1.0f)
        : (hovered ? StyleColor(0.4f, 0.65f, 1.0f, 1.0f)
                   : StyleColor(0.35f, 0.6f, 0.95f, 1.0f));
    thumbStyle.cornerRadius = k_ThumbSize * 0.5f;
    thumbStyle.borderWidth = 1.0f;
    thumbStyle.borderColor = { 0.2f, 0.4f, 0.7f, 1.0f };
    renderer.DrawRect(thumbRect, thumbStyle, 1.0f);
}

}} // namespace GX::GUI
