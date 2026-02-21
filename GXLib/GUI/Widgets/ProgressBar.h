#pragma once
/// @file ProgressBar.h
/// @brief プログレスバーウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief プログレスバー（表示専用、0.0~1.0の進捗率を棒グラフで表示する）
/// SetValueで進捗を設定し、SetBarColorでフィルバーの色を変更できる。
class ProgressBar : public Widget
{
public:
    ProgressBar() = default;
    ~ProgressBar() override = default;

    WidgetType GetType() const override { return WidgetType::ProgressBar; }

    /// @brief 進捗値を設定する（0.0~1.0にクランプされる）
    /// @param v 進捗値
    void SetValue(float v) { m_value = (std::max)(0.0f, (std::min)(v, 1.0f)); }

    /// @brief 現在の進捗値を取得する
    /// @return 進捗値 (0.0~1.0)
    float GetValue() const { return m_value; }

    /// @brief フィルバーの色を設定する
    /// @param c バーの色
    void SetBarColor(const StyleColor& c) { m_barColor = c; }

    /// @brief フィルバーの色を取得する
    /// @return バーの色
    const StyleColor& GetBarColor() const { return m_barColor; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 20.0f; }

    void RenderSelf(UIRenderer& renderer) override;

private:
    float m_value = 0.0f;
    StyleColor m_barColor = { 0.3f, 0.6f, 1.0f, 1.0f };
};

}} // namespace GX::GUI
