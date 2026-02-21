#pragma once
/// @file Slider.h
/// @brief スライダーウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief スライダーウィジェット（ドラッグ操作で数値を入力する）
/// 指定した範囲(min~max)の値をマウスドラッグで調整する。
/// stepを設定するとスナップ（離散値）になる。onValueChangedで値変更を受け取れる。
class Slider : public Widget
{
public:
    Slider() = default;
    ~Slider() override = default;

    WidgetType GetType() const override { return WidgetType::Slider; }

    /// @brief 値を設定する（step設定時はスナップされ、min/maxでクランプされる）
    /// @param v 設定する値
    void SetValue(float v);

    /// @brief 現在の値を取得する
    /// @return 現在値
    float GetValue() const { return m_value; }

    /// @brief 値の範囲を設定する
    /// @param minVal 最小値
    /// @param maxVal 最大値
    void SetRange(float minVal, float maxVal) { m_min = minVal; m_max = maxVal; }

    /// @brief 最小値を取得する
    float GetMin() const { return m_min; }
    /// @brief 最大値を取得する
    float GetMax() const { return m_max; }

    /// @brief ステップ値を設定する（0でスナップ無効）
    /// @param step ステップ値（例: 0.1なら0.1刻み）
    void SetStep(float step) { m_step = step; }

    /// @brief ステップ値を取得する
    float GetStep() const { return m_step; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 24.0f; }

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

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
