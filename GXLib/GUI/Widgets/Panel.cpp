#include "pch.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Panel::RenderSelf(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景描画（色が設定されている場合のみ）
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f ||
        drawStyle.shadowBlur > 0.0f)
    {
        UIRectEffect effect;
        const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
        renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);
    }

    // 子ウィジェットを描画
}

void Panel::RenderChildren(UIRenderer& renderer)
{
    if (computedStyle.overflow == OverflowMode::Hidden ||
        computedStyle.overflow == OverflowMode::Scroll)
    {
        renderer.PushScissor(globalRect);
        Widget::RenderChildren(renderer);
        renderer.PopScissor();
    }
    else
    {
        Widget::RenderChildren(renderer);
    }
}

}} // namespace GX::GUI
