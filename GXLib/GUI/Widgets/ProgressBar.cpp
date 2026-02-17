#include "pch.h"
#include "GUI/Widgets/ProgressBar.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

void ProgressBar::RenderSelf(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // トラック背景（描画用スタイル）
    UIRectEffect effect;
    const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
    renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);

    // フィルバー
    if (m_value > 0.0f)
    {
        float pad = drawStyle.borderWidth;
        float trackW = globalRect.width - pad * 2.0f;
        float trackH = globalRect.height - pad * 2.0f;
        float fillW = trackW * m_value;

        float fillRadius = (std::max)(drawStyle.cornerRadius - pad, 0.0f);

        // フィルバーを SDF rect として描画
        LayoutRect fillRect = {
            globalRect.x + pad,
            globalRect.y + pad,
            fillW,
            trackH
        };
        Style fillStyle;
        fillStyle.backgroundColor = m_barColor;
        fillStyle.cornerRadius = fillRadius;
        renderer.DrawRect(fillRect, fillStyle, 1.0f);
    }
}

}} // namespace GX::GUI
