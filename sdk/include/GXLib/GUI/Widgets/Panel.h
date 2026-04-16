#pragma once
/// @file Panel.h
/// @brief パネルウィジェット — レイアウトコンテナ
///
/// Panel は背景描画 + 子ウィジェットのFlexboxコンテナ。
/// 他のウィジェットを配置するための基本的なコンテナとして機能する。

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui_widgets
/// @{

/// @brief パネルウィジェット（レイアウトコンテナ）
class Panel : public Widget
{
public:
    Panel() = default;
    ~Panel() override = default;

    WidgetType GetType() const override { return WidgetType::Panel; }

    /// @brief 背景（角丸矩形 + ボーダー + 影）を描画する
    /// @param renderer GUI描画用レンダラー
    void RenderSelf(UIRenderer& renderer) override;

    /// @brief 子ウィジェットを描画する。overflow=Hidden/Scrollの場合はシザークリップを適用
    /// @param renderer GUI描画用レンダラー
    void RenderChildren(UIRenderer& renderer) override;
};

/// @}
}} // namespace gx::GUI
