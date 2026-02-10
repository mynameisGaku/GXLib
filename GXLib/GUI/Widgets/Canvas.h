#pragma once
/// @file Canvas.h
/// @brief カスタム描画エリアウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief カスタム描画エリア（onDraw コールバックで自由描画）
class Canvas : public Widget
{
public:
    Canvas() = default;
    ~Canvas() override = default;

    WidgetType GetType() const override { return WidgetType::Canvas; }

    float GetIntrinsicWidth() const override { return 100.0f; }
    float GetIntrinsicHeight() const override { return 100.0f; }

    void Render(UIRenderer& renderer) override;

    /// カスタム描画コールバック
    std::function<void(UIRenderer&, const LayoutRect&)> onDraw;
};

}} // namespace GX::GUI
