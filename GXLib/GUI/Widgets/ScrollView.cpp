#include "pch.h"
#include "GUI/Widgets/ScrollView.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void ScrollView::ComputeContentHeight()
{
    float maxBottom = 0.0f;
    for (auto& child : m_children)
    {
        if (!child->visible) continue;
        // 子の layoutRect.y + height でコンテンツ下端を求める
        float bottom = child->layoutRect.y + child->layoutRect.height +
                       child->computedStyle.margin.bottom;
        maxBottom = (std::max)(maxBottom, bottom);
    }
    m_contentHeight = maxBottom;
}

void ScrollView::ClampScroll()
{
    float viewH = globalRect.height - computedStyle.padding.VerticalTotal() -
                  computedStyle.borderWidth * 2.0f;
    if (viewH <= 0.0f) return;
    float maxScroll = m_contentHeight - viewH;
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    scrollOffsetY = (std::max)(0.0f, (std::min)(scrollOffsetY, maxScroll));
}

bool ScrollView::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (event.type == UIEventType::MouseWheel && enabled)
    {
        scrollOffsetY -= event.wheelDelta * m_scrollSpeed;
        ClampScroll();
        return true;
    }
    return false;
}

void ScrollView::Update(float deltaTime)
{
    Widget::Update(deltaTime);
    ComputeContentHeight();
    ClampScroll();
}

void ScrollView::RenderSelf(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景描画
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f ||
        drawStyle.shadowBlur > 0.0f)
    {
        UIRectEffect effect;
        const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
        renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);
    }

    // クリッピング + 子描画
    renderer.PushScissor(globalRect);
    Widget::RenderChildren(renderer);

    // スクロールバー描画
    float viewH = globalRect.height - computedStyle.padding.VerticalTotal() -
                  computedStyle.borderWidth * 2.0f;
    if (m_contentHeight > viewH && viewH > 0.0f)
    {
        float barH = (viewH / m_contentHeight) * viewH;
        barH = (std::max)(barH, 16.0f);
        float barY = globalRect.y + (scrollOffsetY / m_contentHeight) * viewH;

        float barX = globalRect.x + globalRect.width - k_ScrollbarWidth - 2.0f;
        StyleColor barColor = { 0.5f, 0.5f, 0.6f, 0.5f };
        renderer.DrawSolidRect(barX, barY, k_ScrollbarWidth, barH, barColor);
    }

    renderer.PopScissor();
}

void ScrollView::RenderChildren(UIRenderer& /*renderer*/)
{
    // Children are rendered inside RenderSelf with clipping.
}

}} // namespace GX::GUI
