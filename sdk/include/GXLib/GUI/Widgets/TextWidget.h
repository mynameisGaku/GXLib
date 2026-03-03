#pragma once
/// @file TextWidget.h
/// @brief テキスト表示ウィジェット

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui_widgets
/// @{

/// @brief テキスト表示ウィジェット
/// textAlign/verticalAlignでテキスト位置を制御できる。DxLibのDrawStringに相当する描画を行う。
class TextWidget : public Widget
{
public:
    TextWidget() = default;
    ~TextWidget() override = default;

    WidgetType GetType() const override { return WidgetType::Text; }

    /// @brief 表示するテキストを設定する
    /// @param text 表示テキスト（Unicode）
    void SetText(const gx::WString& text) { m_text = text; layoutDirty = true; }

    /// @brief 現在のテキストを取得する
    /// @return テキスト文字列
    const gx::WString& GetText() const { return m_text; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;
    void RenderSelf(UIRenderer& renderer) override;

    /// @brief UIRendererを設定する（テキスト幅の計測に必要）
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

private:
    gx::WString m_text;               ///< 表示テキスト（Unicode）
    int m_fontHandle = -1;             ///< フォントハンドル
    UIRenderer* m_renderer = nullptr;  ///< GUI描画レンダラー（intrinsic size 計算用）
};

/// @}
}} // namespace gx::GUI
