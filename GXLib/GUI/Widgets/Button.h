#pragma once
/// @file Button.h
/// @brief ボタンウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

/// @brief ボタンウィジェット
/// クリック可能なUI部品。テキストを中央配置で描画し、onClickコールバックに対応する。
/// ホバー/プレス/無効時の見た目はStyleSheet（擬似クラス）またはコード側のスタイルで制御できる。
class Button : public Widget
{
public:
    Button() = default;
    ~Button() override = default;

    WidgetType GetType() const override { return WidgetType::Button; }

    /// @brief ボタンに表示するテキストを設定する
    /// @param text 表示テキスト（Unicode）
    void SetText(const std::wstring& text) { m_text = text; }

    /// @brief 現在のテキストを取得する
    /// @return テキスト文字列
    const std::wstring& GetText() const { return m_text; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    Style hoverStyle;     ///< ホバー時のスタイル（コード指定用）
    Style pressedStyle;   ///< プレス時のスタイル（コード指定用）
    Style disabledStyle;  ///< 無効時のスタイル（コード指定用）

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

    /// UIRendererを設定（intrinsic size 計算に必要）
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

private:
    std::wstring m_text;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
