#pragma once
/// @file TabView.h
/// @brief タブビューウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief タブビュー（タブヘッダー + 子ウィジェットをタブコンテンツとして表示）
class TabView : public Widget
{
public:
    TabView() = default;
    ~TabView() override = default;

    WidgetType GetType() const override { return WidgetType::TabView; }

    void SetTabNames(const std::vector<std::string>& names);
    void SetActiveTab(int index);
    int GetActiveTab() const { return m_activeTab; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override { return 300.0f; }
    float GetIntrinsicHeight() const override { return 200.0f; }

    bool OnEvent(const UIEvent& event) override;
    void Update(float deltaTime) override;
    void Render(UIRenderer& renderer) override;

private:
    static constexpr float k_TabHeaderHeight = 32.0f;
    static constexpr float k_TabPadding = 12.0f;

    std::vector<std::string> m_tabNames;
    std::vector<std::wstring> m_wideTabNames;
    int m_activeTab = 0;
    int m_hoveredTab = -1;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
