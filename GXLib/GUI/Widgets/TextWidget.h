#pragma once
/// @file TextWidget.h
/// @brief テキスト表示ウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief テキスト表示ウィジェット
class TextWidget : public Widget
{
public:
    TextWidget() = default;
    ~TextWidget() override = default;

    WidgetType GetType() const override { return WidgetType::Text; }

    void SetText(const std::wstring& text) { m_text = text; layoutDirty = true; }
    const std::wstring& GetText() const { return m_text; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;

    void Render(UIRenderer& renderer) override;

    /// UIRendererを設定（intrinsic size 計算に必要）
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

private:
    std::wstring m_text;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;  ///< intrinsic size 計算用
};

}} // namespace GX::GUI
