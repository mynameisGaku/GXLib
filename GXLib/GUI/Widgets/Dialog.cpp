#include "pch.h"
#include "GUI/Widgets/Dialog.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

bool Dialog::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (event.type == UIEventType::Click && enabled)
    {
        // オーバーレイ部分（子以外）のクリックで閉じる
        if (event.target == this)
        {
            if (onClose)
                onClose();
            else
                Hide();
            return true;
        }
    }
    return false;
}

void Dialog::Render(UIRenderer& renderer)
{
    // 全画面オーバーレイ
    renderer.DrawSolidRect(globalRect.x, globalRect.y,
                           globalRect.width, globalRect.height,
                           m_overlayColor);

    // 子ウィジェットを描画（CSSでcenter配置される想定）
    Widget::Render(renderer);
}

}} // namespace GX::GUI
