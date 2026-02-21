#include "pch.h"
#include "GUI/Widgets/DropDown.h"
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

void DropDown::SetItems(const std::vector<std::string>& items)
{
    m_items = items;
    m_wideItems.clear();
    m_wideItems.reserve(items.size());
    for (auto& item : items)
        m_wideItems.push_back(Utf8ToWide(item));
    if (m_items.empty())
        m_selectedIndex = -1;
    else if (m_selectedIndex >= static_cast<int>(m_items.size()))
        m_selectedIndex = 0;
}

void DropDown::SetSelectedIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        index = 0;
    m_selectedIndex = index;
}

bool DropDown::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (!enabled) return false;

    if (event.type == UIEventType::Click)
    {
        float headerH = globalRect.height;
        float headerBottom = globalRect.y + headerH;

        if (m_open)
        {
            // ポップアップ内のクリック判定
            float popupTop = headerBottom;
            float relY = event.localY - popupTop;
            if (relY >= 0.0f)
            {
                int idx = static_cast<int>(relY / k_ItemHeight);
                if (idx >= 0 && idx < static_cast<int>(m_items.size()))
                {
                    m_selectedIndex = idx;
                    m_open = false;
                    if (onValueChanged && !m_items.empty() &&
                        m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size())
                        onValueChanged(m_items[m_selectedIndex]);
                    return true;
                }
            }
            // ヘッダークリック → 閉じる
            m_open = false;
            return true;
        }
        else
        {
            // ヘッダークリック → 開く
            m_open = true;
            return true;
        }
    }

    if (event.type == UIEventType::MouseMove && m_open)
    {
        float headerBottom = globalRect.y + globalRect.height;
        float relY = event.localY - headerBottom;
        if (relY >= 0.0f)
        {
            int idx = static_cast<int>(relY / k_ItemHeight);
            m_hoveredItem = (idx >= 0 && idx < static_cast<int>(m_items.size())) ? idx : -1;
        }
        else
        {
            m_hoveredItem = -1;
        }
    }

    if (event.type == UIEventType::FocusLost)
    {
        m_open = false;
    }

    return false;
}

void DropDown::RenderSelf(UIRenderer& renderer)
{
    float headerH = globalRect.height;
    const Style& drawStyle = GetRenderStyle();

    // ヘッダー背景
    UIRectEffect effect;
    const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
    renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);

    // 選択中テキスト
    if (m_fontHandle >= 0 && m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_wideItems.size()))
    {
        float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));
        float textY = globalRect.y + (headerH - textH) * 0.5f;
        renderer.DrawText(globalRect.x + k_DropPadding + 4.0f, textY,
                          m_fontHandle, m_wideItems[m_selectedIndex],
                          drawStyle.color, 1.0f);
    }

    // 矢印（DrawSolidRect で三角形を近似 — Unicode文字は動的ラスタライズを避ける）
    {
        StyleColor arrowColor = { 0.7f, 0.7f, 0.8f, 1.0f };
        float cx = globalRect.x + globalRect.width - k_ArrowWidth * 0.5f;
        float cy = globalRect.y + headerH * 0.5f - 3.0f;
        renderer.DrawSolidRect(cx - 5.0f, cy,      10.0f, 2.0f, arrowColor);
        renderer.DrawSolidRect(cx - 3.0f, cy + 2.0f, 6.0f, 2.0f, arrowColor);
        renderer.DrawSolidRect(cx - 1.0f, cy + 4.0f, 2.0f, 2.0f, arrowColor);
    }

    // ポップアップ（DeferDrawで全ウィジェットの描画後にオーバーレイとして描画する。
    //  ラムダキャプチャのためローカルコピーが必要）
    if (m_open && !m_items.empty())
    {
        float popupX = globalRect.x;
        float popupTop = globalRect.y + headerH;
        float popupH = k_ItemHeight * static_cast<float>(m_items.size());
        float popupW = globalRect.width;
        int hoveredItem = m_hoveredItem;
        int selectedIndex = m_selectedIndex;
        int fontHandle = m_fontHandle;
        Transform2D popupTransform = renderer.GetTransform();
        float popupOpacity = renderer.GetOpacity();
        auto wideItems = m_wideItems;

        renderer.DeferDraw([&renderer, popupX, popupTop, popupW, popupH,
                            hoveredItem, selectedIndex, fontHandle,
                            popupTransform, popupOpacity,
                            wideItems = std::move(wideItems)]()
        {
            // ポップアップ背景
            renderer.PushTransform(popupTransform);
            renderer.PushOpacity(popupOpacity);
            StyleColor popupBg = { 0.12f, 0.12f, 0.18f, 0.95f };
            renderer.DrawSolidRect(popupX, popupTop, popupW, popupH, popupBg);

            // アイテム描画
            for (int i = 0; i < static_cast<int>(wideItems.size()); ++i)
            {
                float itemY = popupTop + i * k_ItemHeight;

                // ホバーハイライト
                if (i == hoveredItem)
                {
                    StyleColor hoverColor = { 0.25f, 0.35f, 0.6f, 0.7f };
                    renderer.DrawSolidRect(popupX, itemY, popupW, k_ItemHeight, hoverColor);
                }

                // テキスト
                if (fontHandle >= 0)
                {
                    float textH = static_cast<float>(renderer.GetLineHeight(fontHandle));
                    float textY = itemY + (k_ItemHeight - textH) * 0.5f;
                    StyleColor textCol = (i == selectedIndex)
                        ? StyleColor(0.4f, 0.7f, 1.0f, 1.0f)
                        : StyleColor(0.9f, 0.9f, 0.95f, 1.0f);
                    renderer.DrawText(popupX + k_DropPadding + 4.0f, textY,
                                      fontHandle, wideItems[i], textCol, 1.0f);
                }
            }

            // ポップアップ枠
            Style borderStyle;
            borderStyle.borderWidth = 1.0f;
            borderStyle.borderColor = { 0.3f, 0.3f, 0.45f, 0.8f };
            borderStyle.cornerRadius = 2.0f;
            LayoutRect popupRect = { popupX, popupTop, popupW, popupH };
            renderer.DrawRect(popupRect, borderStyle, 1.0f);
            renderer.PopOpacity();
            renderer.PopTransform();
        });
    }
}

}} // namespace GX::GUI
