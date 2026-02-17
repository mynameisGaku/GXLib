#include "pch.h"
#include "GUI/Widgets/Canvas.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Canvas::RenderSelf(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景描画
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f)
    {
        UIRectEffect effect;
        const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
        renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);
    }

    // カスタム描画コールバック
    if (onDraw)
        onDraw(renderer, globalRect);
}

}} // namespace GX::GUI
