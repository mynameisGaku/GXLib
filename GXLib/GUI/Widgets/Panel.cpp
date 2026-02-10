#include "pch.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Panel::Render(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景描画（色が設定されている場合のみ）
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f ||
        drawStyle.shadowBlur > 0.0f)
    {
        renderer.DrawRect(globalRect, drawStyle, opacity);
    }

    // 子ウィジェットを描画
    if (computedStyle.overflow == OverflowMode::Hidden ||
        computedStyle.overflow == OverflowMode::Scroll)
    {
        renderer.PushScissor(globalRect);
        Widget::Render(renderer);
        renderer.PopScissor();
    }
    else
    {
        Widget::Render(renderer);
    }
}

}} // namespace GX::GUI
