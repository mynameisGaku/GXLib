#include "pch.h"
#include "GUI/Widgets/ListView.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

static std::wstring Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring result(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                         static_cast<int>(utf8.size()), &result[0], wlen);
    return result;
}

void ListView::SetItems(const std::vector<std::string>& items)
{
    m_items = items;
    m_wideItems.clear();
    m_wideItems.reserve(items.size());
    for (auto& item : items)
        m_wideItems.push_back(Utf8ToWide(item));
    m_selectedIndex = -1;
    scrollOffsetY = 0.0f;
}

void ListView::SetSelectedIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        index = -1;
    m_selectedIndex = index;
}

void ListView::ClampScroll()
{
    float totalH = k_ItemHeight * static_cast<float>(m_items.size());
    float viewH = globalRect.height;
    float maxScroll = totalH - viewH;
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    scrollOffsetY = (std::max)(0.0f, (std::min)(scrollOffsetY, maxScroll));
}

bool ListView::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (!enabled) return false;

    if (event.type == UIEventType::MouseWheel)
    {
        scrollOffsetY -= event.wheelDelta * 30.0f;
        ClampScroll();
        return true;
    }

    if (event.type == UIEventType::Click)
    {
        float relY = event.mouseY - globalRect.y + scrollOffsetY;
        int idx = static_cast<int>(relY / k_ItemHeight);
        if (idx >= 0 && idx < static_cast<int>(m_items.size()))
        {
            m_selectedIndex = idx;
            if (onValueChanged)
                onValueChanged(m_items[idx]);
        }
        return true;
    }

    if (event.type == UIEventType::MouseMove)
    {
        float relY = event.mouseY - globalRect.y + scrollOffsetY;
        int idx = static_cast<int>(relY / k_ItemHeight);
        if (idx >= 0 && idx < static_cast<int>(m_items.size()))
            m_hoveredItem = idx;
        else
            m_hoveredItem = -1;
        return true;
    }

    if (event.type == UIEventType::MouseLeave)
    {
        m_hoveredItem = -1;
    }

    return false;
}

void ListView::Render(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f)
    {
        renderer.DrawRect(globalRect, drawStyle, opacity);
    }

    if (m_items.empty() || m_fontHandle < 0) return;

    renderer.PushScissor(globalRect);

    float viewH = globalRect.height;
    int firstVisible = static_cast<int>(scrollOffsetY / k_ItemHeight);
    int lastVisible = static_cast<int>((scrollOffsetY + viewH) / k_ItemHeight);
    int itemCount = static_cast<int>(m_items.size());

    for (int i = firstVisible; i <= lastVisible && i < itemCount; ++i)
    {
        if (i < 0) continue;

        float itemY = globalRect.y + i * k_ItemHeight - scrollOffsetY;

        // 選択ハイライト
        if (i == m_selectedIndex)
        {
            StyleColor selColor = { 0.3f, 0.5f, 0.8f, 0.6f };
            renderer.DrawSolidRect(globalRect.x, itemY, globalRect.width, k_ItemHeight, selColor);
        }
        else if (i == m_hoveredItem)
        {
            StyleColor hoverColor = { 0.3f, 0.3f, 0.4f, 0.4f };
            renderer.DrawSolidRect(globalRect.x, itemY, globalRect.width, k_ItemHeight, hoverColor);
        }

        // テキスト
        float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));
        float textY = itemY + (k_ItemHeight - textH) * 0.5f;
        renderer.DrawText(globalRect.x + 8.0f, textY, m_fontHandle,
                          m_wideItems[i], drawStyle.color, opacity);
    }

    // スクロールバー
    float totalH = k_ItemHeight * static_cast<float>(itemCount);
    if (totalH > viewH && viewH > 0.0f)
    {
        float barH = (viewH / totalH) * viewH;
        barH = (std::max)(barH, 16.0f);
        float barY = globalRect.y + (scrollOffsetY / totalH) * viewH;
        float barX = globalRect.x + globalRect.width - k_ScrollbarWidth - 2.0f;
        StyleColor barColor = { 0.5f, 0.5f, 0.6f, 0.5f };
        renderer.DrawSolidRect(barX, barY, k_ScrollbarWidth, barH, barColor);
    }

    renderer.PopScissor();
}

}} // namespace GX::GUI
