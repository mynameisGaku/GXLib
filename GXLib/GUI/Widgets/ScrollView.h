#pragma once
/// @file ScrollView.h
/// @brief スクロール可能コンテナウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief スクロール可能なコンテナ（MouseWheel でスクロール）
class ScrollView : public Widget
{
public:
    ScrollView() = default;
    ~ScrollView() override = default;

    WidgetType GetType() const override { return WidgetType::ScrollView; }

    float GetIntrinsicWidth() const override { return 200.0f; }
    float GetIntrinsicHeight() const override { return 150.0f; }

    void SetScrollSpeed(float speed) { m_scrollSpeed = speed; }

    bool OnEvent(const UIEvent& event) override;
    void Update(float deltaTime) override;
    void RenderSelf(UIRenderer& renderer) override;
    void RenderChildren(UIRenderer& renderer) override;

private:
    void ComputeContentHeight();
    void ClampScroll();

    float m_contentHeight = 0.0f;
    float m_scrollSpeed = 30.0f;

    static constexpr float k_ScrollbarWidth = 4.0f;
};

}} // namespace GX::GUI
