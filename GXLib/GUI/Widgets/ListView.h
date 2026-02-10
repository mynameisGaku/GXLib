#pragma once
/// @file ListView.h
/// @brief リストビューウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief リストビュー（スクロール可能なアイテム選択リスト）
class ListView : public Widget
{
public:
    ListView() = default;
    ~ListView() override = default;

    WidgetType GetType() const override { return WidgetType::ListView; }

    void SetItems(const std::vector<std::string>& items);
    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return m_selectedIndex; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 150.0f; }

    bool OnEvent(const UIEvent& event) override;
    void Render(UIRenderer& renderer) override;

private:
    void ClampScroll();

    static constexpr float k_ItemHeight = 28.0f;
    static constexpr float k_ScrollbarWidth = 4.0f;

    std::vector<std::string> m_items;
    std::vector<std::wstring> m_wideItems;
    int m_selectedIndex = -1;
    int m_hoveredItem = -1;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
