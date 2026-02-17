#pragma once
/// @file ProgressBar.h
/// @brief プログレスバーウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief プログレスバー（表示専用、0.0〜1.0）
class ProgressBar : public Widget
{
public:
    ProgressBar() = default;
    ~ProgressBar() override = default;

    WidgetType GetType() const override { return WidgetType::ProgressBar; }

    void SetValue(float v) { m_value = (std::max)(0.0f, (std::min)(v, 1.0f)); }
    float GetValue() const { return m_value; }

    void SetBarColor(const StyleColor& c) { m_barColor = c; }
    const StyleColor& GetBarColor() const { return m_barColor; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 20.0f; }

    void RenderSelf(UIRenderer& renderer) override;

private:
    float m_value = 0.0f;
    StyleColor m_barColor = { 0.3f, 0.6f, 1.0f, 1.0f };
};

}} // namespace GX::GUI
