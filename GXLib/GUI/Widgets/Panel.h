#pragma once
/// @file Panel.h
/// @brief パネルウィジェット — レイアウトコンテナ
///
/// Panel は背景描画 + 子ウィジェットのFlexboxコンテナ。
/// 他のウィジェットを配置するための基本的なコンテナとして機能する。

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief パネルウィジェット（レイアウトコンテナ）
class Panel : public Widget
{
public:
    Panel() = default;
    ~Panel() override = default;

    WidgetType GetType() const override { return WidgetType::Panel; }

    void RenderSelf(UIRenderer& renderer) override;
    void RenderChildren(UIRenderer& renderer) override;
};

}} // namespace GX::GUI
