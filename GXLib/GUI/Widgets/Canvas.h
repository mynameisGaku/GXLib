#pragma once
/// @file Canvas.h
/// @brief カスタム描画エリアウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief カスタム描画エリア
/// onDrawコールバックにUIRendererとレイアウト矩形が渡されるので、自由に描画できる。
/// グラフやカスタムUIなど、既存ウィジェットでは表現できない描画に使う。
class Canvas : public Widget
{
public:
    Canvas() = default;
    ~Canvas() override = default;

    WidgetType GetType() const override { return WidgetType::Canvas; }

    float GetIntrinsicWidth() const override { return 100.0f; }
    float GetIntrinsicHeight() const override { return 100.0f; }

    void RenderSelf(UIRenderer& renderer) override;

    /// カスタム描画コールバック
    std::function<void(UIRenderer&, const LayoutRect&)> onDraw;
};

}} // namespace GX::GUI
