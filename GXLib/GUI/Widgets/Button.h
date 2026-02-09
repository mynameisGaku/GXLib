#pragma once
/// @file Button.h
/// @brief ボタンウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief ボタンウィジェット
class Button : public Widget
{
public:
    Button() = default;
    ~Button() override = default;

    WidgetType GetType() const override { return WidgetType::Button; }

    void SetText(const std::wstring& text) { m_text = text; }
    const std::wstring& GetText() const { return m_text; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    /// ホバー時のスタイル
    Style hoverStyle;
    /// プレス時のスタイル
    Style pressedStyle;
    /// 無効時のスタイル
    Style disabledStyle;

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;

    bool OnEvent(const UIEvent& event) override;
    void Render(UIRenderer& renderer) override;

    /// UIRendererを設定（intrinsic size 計算に必要）
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

private:
    std::wstring m_text;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
