#include "pch.h"
#include "GUI/Widgets/Canvas.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void Canvas::Render(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景描画
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f)
    {
        renderer.DrawRect(globalRect, drawStyle, opacity);
    }

    // カスタム描画コールバック
    if (onDraw)
        onDraw(renderer, globalRect);
}

}} // namespace GX::GUI
