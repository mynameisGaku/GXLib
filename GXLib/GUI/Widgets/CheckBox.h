#pragma once
/// @file CheckBox.h
/// @brief チェックボックスウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief チェックボックスウィジェット
/// クリックでON/OFFを切り替える。左にボックス、右にラベルテキストを表示する。
/// onValueChangedコールバックで値の変化（"true"/"false"）を受け取れる。
class CheckBox : public Widget
{
public:
    CheckBox() = default;
    ~CheckBox() override = default;

    WidgetType GetType() const override { return WidgetType::CheckBox; }

    /// @brief チェック状態を設定する。変更時にonValueChangedが呼ばれる
    /// @param checked true=チェックON
    void SetChecked(bool checked);

    /// @brief チェック状態を取得する
    /// @return true ならチェックON
    bool IsChecked() const { return m_checked; }

    /// @brief チェック状態をboolで取得する（IsCheckedと同等）
    /// @return チェック状態
    bool GetValue() const { return IsChecked(); }

    /// @brief ラベルテキストを設定する
    /// @param text ラベル文字列
    void SetText(const std::wstring& text) { m_text = text; layoutDirty = true; }

    /// @brief ラベルテキストを取得する
    /// @return ラベル文字列
    const std::wstring& GetText() const { return m_text; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    /// @brief UIRendererを設定する（テキスト幅の計測に必要）
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;

    bool OnEvent(const UIEvent& event) override;
    void RenderSelf(UIRenderer& renderer) override;

private:
    static constexpr float k_BoxSize = 18.0f;
    static constexpr float k_Gap = 8.0f;

    bool m_checked = false;
    std::wstring m_text;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
