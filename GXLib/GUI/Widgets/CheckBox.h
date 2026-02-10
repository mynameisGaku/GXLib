#pragma once
/// @file CheckBox.h
/// @brief チェックボックスウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief チェックボックスウィジェット
class CheckBox : public Widget
{
public:
    CheckBox() = default;
    ~CheckBox() override = default;

    WidgetType GetType() const override { return WidgetType::CheckBox; }

    void SetChecked(bool checked);
    bool IsChecked() const { return m_checked; }

    void SetText(const std::wstring& text) { m_text = text; layoutDirty = true; }
    const std::wstring& GetText() const { return m_text; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;

    bool OnEvent(const UIEvent& event) override;
    void Render(UIRenderer& renderer) override;

private:
    static constexpr float k_BoxSize = 18.0f;
    static constexpr float k_Gap = 8.0f;

    bool m_checked = false;
    std::wstring m_text;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
