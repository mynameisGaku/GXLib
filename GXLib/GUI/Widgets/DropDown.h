#pragma once
/// @file DropDown.h
/// @brief ドロップダウンウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief ドロップダウン選択ウィジェット
class DropDown : public Widget
{
public:
    DropDown() = default;
    ~DropDown() override = default;

    WidgetType GetType() const override { return WidgetType::DropDown; }

    void SetItems(const std::vector<std::string>& items);
    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return m_selectedIndex; }
    bool IsOpen() const { return m_open; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override { return 150.0f; }
    float GetIntrinsicHeight() const override { return 30.0f; }

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

private:
    static constexpr float k_ItemHeight = 28.0f;
    static constexpr float k_ArrowWidth = 20.0f;
    static constexpr float k_DropPadding = 4.0f;

    std::vector<std::string> m_items;
    std::vector<std::wstring> m_wideItems;
    int m_selectedIndex = 0;
    int m_hoveredItem = -1;
    bool m_open = false;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
