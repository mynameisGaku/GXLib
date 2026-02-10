#pragma once
/// @file Slider.h
/// @brief スライダーウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief スライダーウィジェット（ドラッグ操作による値入力）
class Slider : public Widget
{
public:
    Slider() = default;
    ~Slider() override = default;

    WidgetType GetType() const override { return WidgetType::Slider; }

    void SetValue(float v);
    float GetValue() const { return m_value; }

    void SetRange(float minVal, float maxVal) { m_min = minVal; m_max = maxVal; }
    float GetMin() const { return m_min; }
    float GetMax() const { return m_max; }

    void SetStep(float step) { m_step = step; }
    float GetStep() const { return m_step; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 24.0f; }

    bool OnEvent(const UIEvent& event) override;
    void Render(UIRenderer& renderer) override;

private:
    float ScreenXToValue(float screenX) const;

    static constexpr float k_TrackHeight = 4.0f;
    static constexpr float k_ThumbSize = 14.0f;

    float m_value = 0.0f;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_step = 0.0f;
    bool m_dragging = false;
};

}} // namespace GX::GUI
