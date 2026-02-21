#pragma once
/// @file RadioButton.h
/// @brief ラジオボタンウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief ラジオボタンウィジェット
/// 同じ親Panel内のRadioButton同士で相互排他選択を行う。
/// 選択時に親のonValueChangedコールバックが呼ばれ、選択されたvalueが渡される。
class RadioButton : public Widget
{
public:
    RadioButton() = default;
    ~RadioButton() override = default;

    WidgetType GetType() const override { return WidgetType::RadioButton; }

    /// @brief 選択状態を設定する。trueにすると同じ親の他のRadioButtonは非選択になる
    /// @param selected true=選択
    void SetSelected(bool selected);

    /// @brief 選択状態を取得する
    /// @return true なら選択中
    bool IsSelected() const { return m_selected; }

    /// @brief ラベルテキストを設定する
    /// @param text ラベル文字列
    void SetText(const std::wstring& text) { m_text = text; layoutDirty = true; }

    /// @brief ラベルテキストを取得する
    /// @return ラベル文字列
    const std::wstring& GetText() const { return m_text; }

    /// @brief onValueChangedに渡される値文字列を設定する
    /// @param value 値文字列
    void SetValue(const std::string& value) { m_value = value; }

    /// @brief 値文字列を取得する
    /// @return 値文字列
    const std::string& GetValue() const { return m_value; }

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
    void DeselectSiblings();

    static constexpr float k_CircleSize = 18.0f;
    static constexpr float k_Gap = 8.0f;

    bool m_selected = false;
    std::wstring m_text;
    std::string m_value;
    int m_fontHandle = -1;
    UIRenderer* m_renderer = nullptr;
};

}} // namespace GX::GUI
