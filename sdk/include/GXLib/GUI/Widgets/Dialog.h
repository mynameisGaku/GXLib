#pragma once
/// @file Dialog.h
/// @brief モーダルダイアログウィジェット

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui_widgets
/// @{

class UIRenderer;

/// @brief モーダルダイアログウィジェット
/// 半透明オーバーレイの上にコンテンツを表示する。
/// オーバーレイ部分をクリックするとonCloseが呼ばれるか、自動で非表示になる。
/// 子ウィジェットをCSSのcenter配置で中央に表示する想定。
class Dialog : public Widget
{
public:
    Dialog() = default;
    ~Dialog() override = default;

    WidgetType GetType() const override { return WidgetType::Dialog; }

    /// @brief ダイアログを表示する
    void Show() { visible = true; }

    /// @brief ダイアログを非表示にする
    void Hide() { visible = false; }

    /// @brief タイトルを設定する（表示用）
    /// @param title タイトル文字列
    void SetTitle(const gx::WString& title) { m_title = title; }

    /// @brief タイトルを取得する
    /// @return タイトル文字列
    const gx::WString& GetTitle() const { return m_title; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    /// @brief UIRendererを設定する
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    /// @brief オーバーレイ（背景の半透明幕）の色を設定する
    /// @param r 赤 (0.0~1.0)
    /// @param g 緑 (0.0~1.0)
    /// @param b 青 (0.0~1.0)
    /// @param a 不透明度 (0.0~1.0)
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
    gx::WString m_title;               ///< ダイアログタイトル
    int m_fontHandle = -1;              ///< フォントハンドル
    UIRenderer* m_renderer = nullptr;   ///< GUI描画レンダラー
    StyleColor m_overlayColor = { 0.0f, 0.0f, 0.0f, 0.5f }; ///< オーバーレイ色（RGBA）
};

/// @}
}} // namespace gx::GUI
