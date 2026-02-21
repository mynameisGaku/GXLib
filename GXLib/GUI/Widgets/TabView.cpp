#include "pch.h"
#include "GUI/Widgets/TabView.h"
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

void TabView::SetTabNames(const std::vector<std::string>& names)
{
    m_tabNames = names;
    m_wideTabNames.clear();
    m_wideTabNames.reserve(names.size());
    for (auto& name : names)
        m_wideTabNames.push_back(Utf8ToWide(name));
}

void TabView::SetActiveTab(int index)
{
    if (index < 0) index = 0;
    if (index >= (int)m_wideTabNames.size() && !m_wideTabNames.empty())
        index = (int)m_wideTabNames.size() - 1;
    m_activeTab = index;
}

void TabView::Update(float deltaTime)
{
    // アクティブタブに対応する子だけをvisibleにして、非アクティブタブの子は非表示にする
    auto& children = GetChildren();
    for (int i = 0; i < static_cast<int>(children.size()); ++i)
    {
        children[i]->visible = (i == m_activeTab);
    }

    Widget::Update(deltaTime);
}

bool TabView::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (!enabled) return false;

    if (event.type == UIEventType::Click)
    {
        // タブヘッダー領域のクリック判定
        float headerBottom = globalRect.y + k_TabHeaderHeight;
        if (event.localY < headerBottom && event.localY >= globalRect.y)
        {
            // どのタブがクリックされたか判定
            int numTabs = static_cast<int>(m_tabNames.size());
            if (numTabs > 0)
            {
                float tabWidth = globalRect.width / static_cast<float>(numTabs);
                int clickedTab = static_cast<int>((event.localX - globalRect.x) / tabWidth);
                if (clickedTab >= 0 && clickedTab < numTabs)
                {
                    SetActiveTab(clickedTab);
                    return true;
                }
            }
        }
    }

    if (event.type == UIEventType::MouseMove)
    {
        float headerBottom = globalRect.y + k_TabHeaderHeight;
        if (event.localY < headerBottom && event.localY >= globalRect.y)
        {
            int numTabs = static_cast<int>(m_tabNames.size());
            if (numTabs > 0)
            {
                float tabWidth = globalRect.width / static_cast<float>(numTabs);
                int hovTab = static_cast<int>((event.localX - globalRect.x) / tabWidth);
                m_hoveredTab = (hovTab >= 0 && hovTab < numTabs) ? hovTab : -1;
            }
        }
        else
        {
            m_hoveredTab = -1;
        }
    }

    if (event.type == UIEventType::MouseLeave)
    {
        m_hoveredTab = -1;
    }

    return false;
}

void TabView::RenderSelf(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景
    if (!drawStyle.backgroundColor.IsTransparent() ||
        drawStyle.borderWidth > 0.0f)
    {
        UIRectEffect effect;
        const UIRectEffect* eff = GetActiveEffect(drawStyle, effect);
        renderer.DrawRect(globalRect, drawStyle, 1.0f, eff);
    }

    // タブヘッダー描画
    int numTabs = static_cast<int>(m_tabNames.size());
    if (numTabs > 0 && m_fontHandle >= 0)
    {
        float tabWidth = globalRect.width / static_cast<float>(numTabs);

        for (int i = 0; i < numTabs; ++i)
        {
            float tabX = globalRect.x + tabWidth * i;
            LayoutRect tabRect = { tabX, globalRect.y, tabWidth, k_TabHeaderHeight };

            StyleColor tabBg;
            if (i == m_activeTab)
                tabBg = { 0.25f, 0.25f, 0.35f, 1.0f };
            else if (i == m_hoveredTab)
                tabBg = { 0.2f, 0.2f, 0.28f, 1.0f };
            else
                tabBg = { 0.15f, 0.15f, 0.2f, 1.0f };

            renderer.DrawSolidRect(tabX, globalRect.y, tabWidth, k_TabHeaderHeight, tabBg);

            // アクティブタブ下線
            if (i == m_activeTab)
            {
                StyleColor lineColor = { 0.3f, 0.6f, 1.0f, 1.0f };
                renderer.DrawSolidRect(tabX, globalRect.y + k_TabHeaderHeight - 2.0f,
                                       tabWidth, 2.0f, lineColor);
            }

            // タブテキスト
            if (i < static_cast<int>(m_wideTabNames.size()))
            {
                int textW = renderer.GetTextWidth(m_fontHandle, m_wideTabNames[i]);
                float textH = static_cast<float>(renderer.GetLineHeight(m_fontHandle));
                float textX = tabX + (tabWidth - textW) * 0.5f;
                float textY = globalRect.y + (k_TabHeaderHeight - textH) * 0.5f;
                StyleColor textColor = (i == m_activeTab)
                    ? StyleColor(1.0f, 1.0f, 1.0f, 1.0f)
                    : StyleColor(0.7f, 0.7f, 0.8f, 1.0f);
                renderer.DrawText(textX, textY, m_fontHandle, m_wideTabNames[i], textColor, 1.0f);
            }
        }
    }

    // アクティブタブの子コンテンツのみ描画
}

void TabView::RenderChildren(UIRenderer& renderer)
{
    auto& children = GetChildren();
    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(children.size()))
    {
        if (children[m_activeTab]->visible)
            children[m_activeTab]->Render(renderer);
    }
}

}} // namespace GX::GUI
