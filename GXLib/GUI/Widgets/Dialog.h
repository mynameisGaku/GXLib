#pragma once
/// @file Dialog.h
/// @brief モーダルダイアログウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief モーダルダイアログ（オーバーレイ + 中央コンテンツ）
class Dialog : public Widget
{
public:
    Dialog() = default;
    ~Dialog() override = default;

    WidgetType GetType() const override { return WidgetType::Dialog; }

    void Show() { visible = true; }
    void Hide() { visible = false; }

    void SetTitle(const std::wstring& title) { m_title = title; }
    const std::wstring& GetTitle() const { return m_title; }

    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }

    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    void SetOverlayColor(float r, float g, float b, float a)
    {
        m_overlayColor = { r, g, b, a };
    }

    float GetIntrinsicWidth() const override { return 0.0f; }
    float GetIntrinsicHeight() const override { return 0.0f; }

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

    /// オーバーレイクリック時のコールバック
    std::function<void()> onClose;

private:
    std::wstring m_title;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
    StyleColor m_overlayColor = { 0.0f, 0.0f, 0.0f, 0.5f };
};

}} // namespace GX::GUI
