#pragma once
/// @file RadioButton.h
/// @brief ラジオボタンウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief ラジオボタン（同一親 Panel 内で相互排他選択）
class RadioButton : public Widget
{
public:
    RadioButton() = default;
    ~RadioButton() override = default;

    WidgetType GetType() const override { return WidgetType::RadioButton; }

    void SetSelected(bool selected);
    bool IsSelected() const { return m_selected; }

    void SetText(const std::wstring& text) { m_text = text; layoutDirty = true; }
    const std::wstring& GetText() const { return m_text; }

    void SetValue(const std::string& value) { m_value = value; }
    const std::string& GetValue() const { return m_value; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;

    bool OnEvent(const UIEvent& event) override;
    void Render(UIRenderer& renderer) override;

private:
    void DeselectSiblings();

    static constexpr float k_CircleSize = 18.0f;
    static constexpr float k_Gap = 8.0f;

    bool m_selected = false;
    std::wstring m_text;
    std::string m_value;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
